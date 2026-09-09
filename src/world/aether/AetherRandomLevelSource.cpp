#include "AetherRandomLevelSource.h"
#include "Aether.h"
#include "AetherFeatures.h"
#include "AetherDungeons.h"
#include "../level/Level.h"
#include "../level/chunk/LevelChunk.h"
#include "../level/levelgen/feature/OreFeature.h"
#include "../level/material/Material.h"
#include "../level/tile/Tile.h"
#include "../level/MobSpawner.h"
#include "../entity/EntityTypes.h"
#include "../entity/MobCategory.h"
#include "../../util/Mth.h"
#include "../../platform/log.h"
#include <cstring>

static const int MAX_BUFFER_SIZE = 1024;

// Island shaping constants (tuned against the PerlinNoise implementation:
// a 6-octave 2D region scaled by 0.6 spans roughly -0.9..0.75 after /40).
static const float ISLAND_SCALE = 0.6f;
static const float ISLAND_NORM = 40.0f;
static const float ISLAND_THRESHOLD = 0.15f;
static const int ISLAND_CENTER_Y = 74;

AetherRandomLevelSource::AetherRandomLevelSource(Level* level, long seed)
:	random(seed),
	lperlinNoise1(&random, 16),
	lperlinNoise2(&random, 16),
	selectorNoise(&random, 8),
	islandNoise(&random, 6),
	surfaceNoise(&random, 4),
	depthNoise(&random, 16),
	level(level),
	pnr(NULL), ar(NULL), br(NULL), sr(NULL), dr(NULL)
{
	buffer = new float[MAX_BUFFER_SIZE];
	_aetherCreatures.push_back(Biome::MobSpawnerData(MobTypes::Moa, 10, 2, 4));
	_aetherMonsters.push_back(Biome::MobSpawnerData(MobTypes::Sentry, 10, 1, 2));
}

AetherRandomLevelSource::~AetherRandomLevelSource() {
	delete[] buffer;
	delete[] pnr;
	delete[] ar;
	delete[] br;
	delete[] sr;
	delete[] dr;
}

bool AetherRandomLevelSource::hasChunk(int x, int z) {
	int64_t key = ((int64_t)x << 32) | ((int64_t)(uint32_t)z);
	return chunkMap.find(key) != chunkMap.end();
}

LevelChunk* AetherRandomLevelSource::create(int x, int z) {
	return getChunk(x, z);
}

LevelChunk* AetherRandomLevelSource::getChunk(int xOffs, int zOffs) {
	int64_t key = ((int64_t)xOffs << 32) | ((int64_t)(uint32_t)zOffs);
	ChunkMap::iterator it = chunkMap.find(key);
	if (it != chunkMap.end()) return it->second;

	random.setSeed((long)(xOffs * 341872712l + zOffs * 132899541l));

	unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
	memset(blocks, 0, LevelChunk::ChunkBlockCount);

	LevelChunk* levelChunk = new LevelChunk(level, blocks, xOffs, zOffs);
	chunkMap.insert(std::make_pair(key, levelChunk));

	prepareHeights(xOffs, zOffs, blocks);
	buildSurfaces(xOffs, zOffs, blocks);

	levelChunk->recalcHeightmap();
	return levelChunk;
}

// 3D density used to carve caves / overhangs into the island mass.
float* AetherRandomLevelSource::getHeights(float* buffer, int x, int y, int z, int xSize, int ySize, int zSize) {
	float s = 684.412f;
	float hs = 684.412f * 2.0f;
	pnr = selectorNoise.getRegion(pnr, (float)x, (float)y, (float)z, xSize, ySize, zSize, s / 80.0f, hs / 60.0f, s / 80.0f);
	ar = lperlinNoise1.getRegion(ar, (float)x, (float)y, (float)z, xSize, ySize, zSize, s, hs, s);
	br = lperlinNoise2.getRegion(br, (float)x, (float)y, (float)z, xSize, ySize, zSize, s, hs, s);
	int p = 0;
	for (int xx = 0; xx < xSize; xx++) {
		for (int zz = 0; zz < zSize; zz++) {
			for (int yy = 0; yy < ySize; yy++) {
				float bb = ar[p] / 512.0f;
				float cc = br[p] / 512.0f;
				float v = (pnr[p] / 10.0f + 1.0f) / 2.0f;
				float val;
				if (v < 0.0f) val = bb;
				else if (v > 1.0f) val = cc;
				else val = bb + (cc - bb) * v;
				buffer[p] = val;
				p++;
			}
		}
	}
	return buffer;
}

void AetherRandomLevelSource::prepareHeights(int xOffs, int zOffs, unsigned char* blocks) {
	// 2D island mask and large-scale height offset
	sr = islandNoise.getRegion(sr, xOffs * 16, zOffs * 16, 16, 16, ISLAND_SCALE, ISLAND_SCALE, 0.5f);
	dr = depthNoise.getRegion(dr, xOffs * 16, zOffs * 16, 16, 16, 0.08f, 0.08f, 0.5f);
	for (int i = 0; i < 256; ++i) {
		islandBuffer[i] = sr[i] / ISLAND_NORM - ISLAND_THRESHOLD;
	}

	// 3D density (4x8x4 cells, trilinearly interpolated like the nether generator)
	const int xChunks = 16 / CHUNK_WIDTH;
	const int xSize = xChunks + 1, ySize = 128 / CHUNK_HEIGHT + 1, zSize = xChunks + 1;
	buffer = getHeights(buffer, xOffs * xChunks, 0, zOffs * xChunks, xSize, ySize, zSize);

	int holyId = Aether::holystone->id;

	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			float e = islandBuffer[x * 16 + z];
			if (e <= 0.0f) continue;

			// Island profile: flat-ish top, deep rounded underside.
			float depthOffs = dr[x * 16 + z] / 900.0f; // roughly -6..6
			float center = ISLAND_CENTER_Y + depthOffs;
			float thick = Mth::Min(e, 0.7f) / 0.7f; // 0..1
			int top = (int)(center + 4.0f + thick * 14.0f);
			int bottom = (int)(center - 4.0f - thick * thick * 42.0f);
			if (top > ISLAND_MAX_Y) top = ISLAND_MAX_Y;
			if (bottom < ISLAND_MIN_Y) bottom = ISLAND_MIN_Y;

			// 3D density lookup (cell + trilinear weights)
			int xc = x / CHUNK_WIDTH, zc = z / CHUNK_WIDTH;
			float fx = (float)(x % CHUNK_WIDTH) / CHUNK_WIDTH;
			float fz = (float)(z % CHUNK_WIDTH) / CHUNK_WIDTH;

			for (int y = bottom; y <= top; y++) {
				int yc = y / CHUNK_HEIGHT;
				float fy = (float)(y % CHUNK_HEIGHT) / CHUNK_HEIGHT;
				if (yc + 1 >= ySize) break;
				#define D(ix, iy, iz) buffer[((xc + (ix)) * zSize + (zc + (iz))) * ySize + (yc + (iy))]
				float d00 = D(0, 0, 0) + (D(0, 1, 0) - D(0, 0, 0)) * fy;
				float d01 = D(0, 0, 1) + (D(0, 1, 1) - D(0, 0, 1)) * fy;
				float d10 = D(1, 0, 0) + (D(1, 1, 0) - D(1, 0, 0)) * fy;
				float d11 = D(1, 0, 1) + (D(1, 1, 1) - D(1, 0, 1)) * fy;
				#undef D
				float d0 = d00 + (d01 - d00) * fz;
				float d1 = d10 + (d11 - d10) * fz;
				float density = d0 + (d1 - d0) * fx;

				// Near the island edges / underside the density carves holes,
				// in the core it only creates small caves.
				float edge = 1.0f - Mth::Min(e / 0.35f, 1.0f); // 1 at the rim, 0 inside
				float depthFrac = (float)(top - y) / (float)Mth::Max(top - bottom, 1); // 0 at top, 1 at bottom
				float carve = density / 20.0f - 1.6f + edge * 2.2f + depthFrac * depthFrac * 1.4f;
				if (carve > 0.9f) continue; // air

				int offs = (x << 11) | (z << 7) | y;
				blocks[offs] = (unsigned char)holyId;
			}
		}
	}
}

void AetherRandomLevelSource::buildSurfaces(int xOffs, int zOffs, unsigned char* blocks) {
	float s = 1.0f / 16.0f;
	surfaceNoise.getRegion(surfaceBuffer, (float)(xOffs * 16), (float)(zOffs * 16), 0, 16, 16, 1, s * 1.5f, s * 1.5f, 1.0f);

	int holyId = Aether::holystone->id;
	int grassId = Aether::aetherGrass->id;
	int dirtId = Aether::aetherDirt->id;
	int quickId = Aether::quicksoil->id;
	int waterId = Tile::calmWater->id;
	int mossyId = Aether::mossyHolystone->id;
	int iceId = Aether::icestone->id;

	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			float e = islandBuffer[x * 16 + z];
			if (e <= 0.0f) continue;
			float sn = surfaceBuffer[x + z * 16];
			bool quick = sn > 1.9f;
			bool lake = sn < -2.1f && e > 0.45f;

			int run = -1;
			int topY = -1;
			for (int y = 127; y >= 0; y--) {
				int offs = (x << 11) | (z << 7) | y;
				int old = blocks[offs];
				if (old == 0) {
					run = -1;
					continue;
				}
				if (old != holyId) continue;
				if (run == -1) {
					// first solid block from above: the surface
					run = 3 + (int)(random.nextFloat() * 2);
					if (topY < 0) topY = y;
					if (lake && topY == y) {
						// hollow out a lake basin: top 2 blocks become water
						blocks[offs] = (unsigned char)waterId;
						if (y - 1 >= 0 && blocks[(x << 11) | (z << 7) | (y - 1)] == holyId)
							blocks[(x << 11) | (z << 7) | (y - 1)] = (unsigned char)waterId;
						run = 2;
						continue;
					}
					if (quick) blocks[offs] = (unsigned char)quickId;
					else blocks[offs] = (unsigned char)grassId;
				} else if (run > 0) {
					run--;
					if (quick) blocks[offs] = (unsigned char)(run > 1 ? quickId : dirtId);
					else blocks[offs] = (unsigned char)dirtId;
				} else {
					// deep interior: occasional mossy holystone / icestone flecks
					if (random.nextInt(60) == 0) blocks[offs] = (unsigned char)mossyId;
					else if (random.nextInt(140) == 0) blocks[offs] = (unsigned char)iceId;
				}
			}
			// The underside of an island gets a mossy fringe
			for (int y = 1; y < 127; y++) {
				int offs = (x << 11) | (z << 7) | y;
				if (blocks[offs] == holyId && blocks[(x << 11) | (z << 7) | (y - 1)] == 0 && random.nextInt(3) == 0) {
					blocks[offs] = (unsigned char)mossyId;
				}
			}
		}
	}
}

void AetherRandomLevelSource::placeCloudLayer(int xo, int zo) {
	// Aerclouds float in the open air between and below the islands.
	int tries = 2 + random.nextInt(3);
	for (int i = 0; i < tries; i++) {
		int x = xo + random.nextInt(16);
		int z = zo + random.nextInt(16);
		int y = 30 + random.nextInt(80);
		if (!level->isEmptyTile(x, y, z)) continue;
		// don't spawn clouds inside islands: need air above and below
		if (!level->isEmptyTile(x, y + 2, z) || !level->isEmptyTile(x, y - 2, z)) continue;

		int r = random.nextInt(100);
		Tile* cloud = Aether::aercloudCold;
		if (r < 8) cloud = Aether::aercloudGolden;
		else if (r < 22) cloud = Aether::aercloudBlue;
		else if (r < 27) cloud = Aether::aercloudPurple;
		else if (r < 31) cloud = Aether::aercloudGreen;
		else if (r < 34) cloud = Aether::aercloudStorm;

		int len = 3 + random.nextInt(6);
		int wid = 2 + random.nextInt(3);
		bool alongX = random.nextInt(2) == 0;
		for (int a = 0; a < len; a++) {
			for (int b = 0; b < wid; b++) {
				int cx = alongX ? x + a : x + b;
				int cz = alongX ? z + b : z + a;
				if ((a == 0 || a == len - 1) && (b == 0 || b == wid - 1) && random.nextInt(2) == 0) continue;
				if (level->isEmptyTile(cx, y, cz)) level->setTileNoUpdate(cx, y, cz, cloud->id);
			}
		}
	}
}

void AetherRandomLevelSource::postProcess(ChunkSource* parent, int xt, int zt) {
	int xo = xt * 16;
	int zo = zt * 16;

	random.setSeed(level->getSeed());
	int xScale = random.nextInt() / 2 * 2 + 1;
	int zScale = random.nextInt() / 2 * 2 + 1;
	random.setSeed(((xt * xScale) + (zt * zScale)) ^ level->getSeed());

	int holyId = Aether::holystone->id;

	// 1. Ores inside holystone. Islands only fill a fraction of the 40..112 band, so
	//    pick the y inside the island body under a random column instead of blindly.
	// NB: OreFeature places nothing for sizes below ~8 (the blob radius rounds away), so
	//     rarity is controlled with the try count rather than the vein size.
	struct OreSpec { Tile* tile; int size; int tries; };
	OreSpec ores[] = {
		{ Aether::ambrosiumOre, 12, 14 },
		{ Aether::zaniteOre,    10, 10 },
		{ Aether::gravititeOre,  8,  5 },
		{ Aether::icestone,     10,  6 },
		{ Aether::aetherDirt,   16,  4 },
	};
	for (size_t o = 0; o < sizeof(ores) / sizeof(ores[0]); o++) {
		for (int i = 0; i < ores[o].tries; i++) {
			int x = xo + random.nextInt(16), z = zo + random.nextInt(16);
			// Find island top and bottom in this column
			int top = -1, bottom = -1;
			for (int yy = ISLAND_MAX_Y + 8; yy >= ISLAND_MIN_Y - 4; yy--) {
				int t = level->getTile(x, yy, z);
				if (t == holyId) { if (top < 0) top = yy; bottom = yy; }
			}
			if (top < 0) continue;
			int span = top - bottom;
			int y = bottom + (span > 0 ? random.nextInt(span + 1) : 0);
			// gravitite prefers the lower half of islands (but not the very underside where the blob would miss)
			if (ores[o].tile == Aether::gravititeOre) y = bottom + 2 + (span > 4 ? random.nextInt(span / 2) : 0);
			// OreFeature centres on (x+8, y+2..4, z+8): shift back so the blob lands on the sampled column
			OreFeature(ores[o].tile->id, ores[o].size, holyId).place(level, &random, x - 8, y - 3, z - 8);
		}
	}

	// 2. Trees on the island surface
	int trees = random.nextInt(4);
	if (random.nextInt(6) == 0) trees += 3;
	for (int i = 0; i < trees; i++) {
		int x = xo + random.nextInt(16) + 8;
		int z = zo + random.nextInt(16) + 8;
		int y = level->getTopSolidBlock(x, z);
		if (y < ISLAND_MIN_Y || y > ISLAND_MAX_Y + 2) continue;
		bool golden = random.nextInt(9) == 0;
		AetherTreeFeature(false, golden).place(level, &random, x, y, z);
	}

	// 3. Flowers, berry bushes
	{
		int x = xo + random.nextInt(16) + 8, z = zo + random.nextInt(16) + 8;
		AetherPlantPatchFeature(Aether::whiteFlower->id, 8).place(level, &random, x, 0, z);
	}
	if (random.nextInt(2) == 0) {
		int x = xo + random.nextInt(16) + 8, z = zo + random.nextInt(16) + 8;
		AetherPlantPatchFeature(Aether::purpleFlower->id, 6).place(level, &random, x, 0, z);
	}
	{
		int x = xo + random.nextInt(16) + 8, z = zo + random.nextInt(16) + 8;
		AetherPlantPatchFeature(Aether::berryBush->id, 6).place(level, &random, x, 0, z);
	}

	// 4. Aerclouds
	placeCloudLayer(xo + 8, zo + 8);

	// 5. Dungeons (one of each kind per world, positions derived from the seed)
	placeDungeons(xt, zt);

	// 6. Moa flocks
	if (!level->isClientSide && level->getLevelData()->getSpawnMobs() && random.nextInt(3) == 0) {
		int count = 2 + random.nextInt(3);
		int cx = xo + 8 + random.nextInt(8), cz = zo + 8 + random.nextInt(8);
		for (int i = 0; i < count; i++) {
			int x = cx + random.nextInt(5) - 2, z = cz + random.nextInt(5) - 2;
			int y = level->getTopSolidBlock(x, z);
			if (y < ISLAND_MIN_Y || y > ISLAND_MAX_Y + 2) continue;
			int below = level->getTile(x, y - 1, z);
			if (below != Aether::aetherGrass->id && below != Aether::enchantedGrass->id) continue;
			Aether::spawnMob(level, MobTypes::Moa, x + 0.5f, (float)y, z + 0.5f);
		}
	}
}

void AetherRandomLevelSource::placeDungeons(int xt, int zt) {
	AetherDungeons::placeForChunk(level, xt, zt);
}

bool AetherRandomLevelSource::tick() {
	return false;
}

Biome::MobList AetherRandomLevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
	if (&mobCategory == &MobCategory::monster) return _aetherMonsters;
	if (&mobCategory == &MobCategory::creature) return _aetherCreatures;
	return _emptyMobList;
}

bool AetherRandomLevelSource::shouldSave() {
	return true;
}

std::string AetherRandomLevelSource::gatherStats() {
	return "AetherRandomLevelSource";
}
