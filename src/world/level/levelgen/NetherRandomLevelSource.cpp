#include "NetherRandomLevelSource.h"
#include "feature/SpringFeature.h"
#include "feature/OreFeature.h"
#include "../Level.h"
#include "../ChunkPos.h"
#include "../MobSpawner.h"
#include "../chunk/LevelChunk.h"
#include "../material/Material.h"
#include "../tile/Tile.h"
#include "../tile/FireTile.h"
#include "../../entity/EntityTypes.h"
#include "../../entity/MobCategory.h"
#include "../../rpg/Rpg.h"

static const int MAX_BUFFER_SIZE = 1024;

NetherRandomLevelSource::NetherRandomLevelSource(Level* level, long seed)
:	random(seed),
	level(level),
	lperlinNoise1(&random, 16),
	lperlinNoise2(&random, 16),
	perlinNoise1(&random, 8),
	perlinNoise2(&random, 4),
	perlinNoise3(&random, 4),
	scaleNoise(&random, 10),
	depthNoise(&random, 16),
	pnr(NULL), ar(NULL), br(NULL), sr(NULL), dr(NULL)
{
	buffer = new float[MAX_BUFFER_SIZE];
	_netherEnemies.push_back(Biome::MobSpawnerData(MobTypes::PigZombie, 10, 4, 4));
}

NetherRandomLevelSource::~NetherRandomLevelSource() {
	delete[] buffer;
	delete[] pnr;
	delete[] ar;
	delete[] br;
	delete[] sr;
	delete[] dr;
}

bool NetherRandomLevelSource::hasChunk(int x, int y) {
	int64_t key = ((int64_t)x << 32) | ((int64_t)(uint32_t)y);
	return chunkMap.find(key) != chunkMap.end();
}

LevelChunk* NetherRandomLevelSource::create(int x, int z) {
	return getChunk(x, z);
}

LevelChunk* NetherRandomLevelSource::getChunk(int xOffs, int zOffs) {
	int64_t key = ((int64_t)xOffs << 32) | ((int64_t)(uint32_t)zOffs);

	ChunkMap::iterator it = chunkMap.find(key);
	if (it != chunkMap.end())
		return it->second;

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

void NetherRandomLevelSource::prepareHeights(int xOffs, int zOffs, unsigned char* blocks) {
	int xChunks = 16 / CHUNK_WIDTH; // 4
	int lavaHeight = 32;

	int xSize = xChunks + 1; // 5
	int ySize = 128 / CHUNK_HEIGHT + 1; // 17
	int zSize = xChunks + 1; // 5
	buffer = getHeights(buffer, xOffs * xChunks, 0, zOffs * xChunks, xSize, ySize, zSize);

	for (int xc = 0; xc < xChunks; xc++) {
		for (int zc = 0; zc < xChunks; zc++) {
			for (int yc = 0; yc < 128 / CHUNK_HEIGHT; yc++) {
				float yStep = 1.0f / (float)CHUNK_HEIGHT;
				float s0 = buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 0)];
				float s1 = buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 0)];
				float s2 = buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 0)];
				float s3 = buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 0)];

				float s0a = (buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 1)] - s0) * yStep;
				float s1a = (buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 1)] - s1) * yStep;
				float s2a = (buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 1)] - s2) * yStep;
				float s3a = (buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 1)] - s3) * yStep;

				for (int y = 0; y < CHUNK_HEIGHT; y++) {
					float xStep = 1.0f / (float)CHUNK_WIDTH;

					float _s0 = s0;
					float _s1 = s1;
					float _s0a = (s2 - s0) * xStep;
					float _s1a = (s3 - s1) * xStep;

					for (int x = 0; x < CHUNK_WIDTH; x++) {
						int offs = (x + xc * CHUNK_WIDTH) << 11 | (0 + zc * CHUNK_WIDTH) << 7 | (yc * CHUNK_HEIGHT + y);
						int step = 1 << 7;
						float zStep = 1.0f / (float)CHUNK_WIDTH;

						float val = _s0;
						float vala = (_s1 - _s0) * zStep;
						for (int z = 0; z < CHUNK_WIDTH; z++) {
							int worldY = yc * CHUNK_HEIGHT + y;
							int tileId = 0;

							if (val > 0.0f) {
								tileId = Tile::netherrack->id;
							} else if (worldY < lavaHeight) {
								tileId = Tile::calmLava->id;
							}

							blocks[offs] = (unsigned char)tileId;
							offs += step;
							val += vala;
						}
						_s0 += _s0a;
						_s1 += _s1a;
					}

					s0 += s0a;
					s1 += s1a;
					s2 += s2a;
					s3 += s3a;
				}
			}
		}
	}
}

void NetherRandomLevelSource::buildSurfaces(int xOffs, int zOffs, unsigned char* blocks) {
	int lavaHeight = 32;

	float s = 1.0f / 16.0f;
	perlinNoise2.getRegion(soulSandBuffer, (float)(xOffs * 16), (float)(zOffs * 16), 0, 16, 16, 1, s, s, 1.0f);
	perlinNoise2.getRegion(gravelBuffer, (float)(xOffs * 16), 109.0134f, (float)(zOffs * 16), 16, 1, 16, s, 1.0f, s);
	perlinNoise3.getRegion(depthBuffer, (float)(xOffs * 16), (float)(zOffs * 16), 0, 16, 16, 1, s * 2.0f, s * 2.0f, s * 2.0f);

	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			bool soulSand = (soulSandBuffer[x + z * 16] + random.nextFloat() * 0.2f) > 0.0f;
			bool gravel = (gravelBuffer[x + z * 16] + random.nextFloat() * 0.2f) > 2.0f;
			int runDepth = (int)(depthBuffer[x + z * 16] / 3.0f + 3.0f + random.nextFloat() * 0.25f);

			int run = -1;
			int topMat = Tile::netherrack->id;
			int bodyMat = Tile::netherrack->id;

			for (int y = 127; y >= 0; y--) {
				int offs = (z * 16 + x) * 128 + y;

				// Bedrock floor (y <= 0..4) and ceiling (y >= 123..127)
				if (y >= 127 - random.nextInt(5) || y <= random.nextInt(5)) {
					blocks[offs] = (unsigned char)Tile::unbreakable->id;
					continue;
				}

				int old = blocks[offs];
				if (old == 0) {
					run = -1;
				} else if (old == Tile::netherrack->id) {
					if (run == -1) {
						if (runDepth <= 0) {
							topMat = 0;
							bodyMat = Tile::netherrack->id;
						} else if (y >= lavaHeight - 4 && y <= lavaHeight + 1) {
							topMat = Tile::netherrack->id;
							bodyMat = Tile::netherrack->id;
							if (gravel) {
								topMat = Tile::gravel->id;
								bodyMat = Tile::netherrack->id;
							}
							if (soulSand) {
								topMat = Tile::hellSand ? Tile::hellSand->id : Tile::netherrack->id;
								bodyMat = Tile::hellSand ? Tile::hellSand->id : Tile::netherrack->id;
							}
						}

						run = runDepth;
						if (y >= lavaHeight - 1) {
							blocks[offs] = (unsigned char)topMat;
						} else {
							blocks[offs] = (unsigned char)bodyMat;
						}
					} else if (run > 0) {
						run--;
						blocks[offs] = (unsigned char)bodyMat;
					}
				}
			}
		}
	}
}

float* NetherRandomLevelSource::getHeights(float* buffer, int x, int y, int z, int xSize, int ySize, int zSize) {
	float s = 684.412f;
	float hs = 684.412f * 2.0f;

	sr = scaleNoise.getRegion(sr, x, z, xSize, zSize, 1.121f, 1.121f, 0.5f);
	dr = depthNoise.getRegion(dr, x, z, xSize, zSize, 200.0f, 200.0f, 0.5f);

	pnr = perlinNoise1.getRegion(pnr, (float)x, (float)y, (float)z, xSize, ySize, zSize, s / 80.0f, hs / 60.0f, s / 80.0f);
	ar = lperlinNoise1.getRegion(ar, (float)x, (float)y, (float)z, xSize, ySize, zSize, s, hs, s);
	br = lperlinNoise2.getRegion(br, (float)x, (float)y, (float)z, xSize, ySize, zSize, s, hs, s);

	int p = 0;
	int pp = 0;

	for (int xx = 0; xx < xSize; xx++) {
		for (int zz = 0; zz < zSize; zz++) {
			float scale = ((sr[pp] + 256.0f) / 512.0f);
			if (scale > 1.0f) scale = 1.0f;
			if (scale < 0.0f) scale = 0.0f;
			scale += 0.5f;

			pp++;

			for (int yy = 0; yy < ySize; yy++) {
				float bb = ar[p] / 512.0f;
				float cc = br[p] / 512.0f;

				float v = (pnr[p] / 10.0f + 1.0f) / 2.0f;
				float val = 0;
				if (v < 0.0f) val = bb;
				else if (v > 1.0f) val = cc;
				else val = bb + (cc - bb) * v;

				// Shaping Nether caves: open cavern in middle, solid at floor & ceiling
				float yNormalized = (float)yy / (float)(ySize - 1);
				float yOffset = (yNormalized - 0.5f) * 2.0f;
				val += (yOffset * yOffset) * 10.0f - 2.5f;

				// Slide boundaries
				if (yy < 3) {
					float slide = (float)(3 - yy) / 3.0f;
					val = val * (1.0f - slide) + 12.0f * slide;
				}
				if (yy > ySize - 4) {
					float slide = (float)(yy - (ySize - 4)) / 3.0f;
					val = val * (1.0f - slide) + 12.0f * slide;
				}

				buffer[p] = val;
				p++;
			}
		}
	}
	return buffer;
}

void NetherRandomLevelSource::postProcess(ChunkSource* parent, int xt, int zt) {
	int xo = xt * 16;
	int zo = zt * 16;

	random.setSeed(level->getSeed());
	int xScale = random.nextInt() / 2 * 2 + 1;
	int zScale = random.nextInt() / 2 * 2 + 1;
	random.setSeed(((xt * xScale) + (zt * zScale)) ^ level->getSeed());

	// 1. Fire on netherrack
	for (int i = 0; i < 10; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = 4 + random.nextInt(120);
		int z = zo + random.nextInt(16) + 8;
		if (level->isEmptyTile(x, y, z) && level->getTile(x, y - 1, z) == Tile::netherrack->id) {
			level->setTile(x, y, z, Tile::fire->id);
		}
	}

	// 2. Hanging Glowstone clusters from ceiling
	for (int i = 0; i < 4; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = 60 + random.nextInt(60);
		int z = zo + random.nextInt(16) + 8;
		if (level->isEmptyTile(x, y, z) && level->getTile(x, y + 1, z) == Tile::netherrack->id) {
			level->setTile(x, y, z, Tile::lightGem->id);
			for (int j = 0; j < 40; j++) {
				int gx = x + random.nextInt(6) - random.nextInt(6);
				int gy = y - random.nextInt(8);
				int gz = z + random.nextInt(6) - random.nextInt(6);
				if (level->isEmptyTile(gx, gy, gz)) {
					int count = 0;
					if (level->getTile(gx - 1, gy, gz) == Tile::lightGem->id) count++;
					if (level->getTile(gx + 1, gy, gz) == Tile::lightGem->id) count++;
					if (level->getTile(gx, gy - 1, gz) == Tile::lightGem->id) count++;
					if (level->getTile(gx, gy + 1, gz) == Tile::lightGem->id) count++;
					if (level->getTile(gx, gy, gz - 1) == Tile::lightGem->id) count++;
					if (level->getTile(gx, gy, gz + 1) == Tile::lightGem->id) count++;
					if (count == 1) {
						level->setTile(gx, gy, gz, Tile::lightGem->id);
					}
				}
			}
		}
	}

	// 3. Nether Quartz and Gravel veins in Netherrack
	for (int i = 0; i < 16; i++) {
		int x = xo + random.nextInt(16);
		int y = 10 + random.nextInt(108);
		int z = zo + random.nextInt(16);
		OreFeature quartzFeature(Tile::quartzBlock->id, 14, Tile::netherrack->id);
		quartzFeature.place(level, &random, x, y, z);
	}

	for (int i = 0; i < 8; i++) {
		int x = xo + random.nextInt(16);
		int y = 10 + random.nextInt(108);
		int z = zo + random.nextInt(16);
		OreFeature gravelFeature(Tile::gravel->id, 16, Tile::netherrack->id);
		gravelFeature.place(level, &random, x, y, z);
	}

	// 4. Flowing Lava Springs
	for (int i = 0; i < 16; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = 4 + random.nextInt(120);
		int z = zo + random.nextInt(16) + 8;
		SpringFeature lavaFeature(Tile::lava->id);
		lavaFeature.place(level, &random, x, y, z);
	}

	// 5. Spawn Nether Mobs (PigZombies)
	if (!level->isClientSide && level->getLevelData()->getSpawnMobs()) {
		for (int i = 0; i < 4; i++) {
			int x = xo + random.nextInt(16) + 8;
			int z = zo + random.nextInt(16) + 8;
			int y = 35 + random.nextInt(80);
			if (level->getTile(x, y - 1, z) == Tile::netherrack->id &&
				level->isEmptyTile(x, y, z) &&
				level->isEmptyTile(x, y + 1, z)) {
				// Valid mob spawn spot
				MobSpawner::postProcessSpawnMobs(level, NULL, x, z, 1, 1, &random);
			}
		}
	}

	// RPG mode: loot chests are rarer but better in the Nether
	if (!level->isClientSide && Rpg::isEnabled(level) && random.nextInt(2) == 0)
		Rpg::placeLootChest(level, xo, zo, &random);
}

bool NetherRandomLevelSource::tick() {
	return false;
}

Biome::MobList NetherRandomLevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
	if (&mobCategory == &MobCategory::monster) {
		return _netherEnemies;
	}
	return _emptyMobList;
}

bool NetherRandomLevelSource::shouldSave() {
	return true;
}

std::string NetherRandomLevelSource::gatherStats() {
	return "NetherRandomLevelSource";
}
