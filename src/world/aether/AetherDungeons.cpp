#include "AetherDungeons.h"
#include "Aether.h"
#include "AetherRandomLevelSource.h"
#include "../level/Level.h"
#include "../level/LevelConstants.h"
#include "../level/tile/Tile.h"
#include "../level/tile/entity/ChestTileEntity.h"
#include "../entity/EntityTypes.h"
#include "../entity/Mob.h"
#include "../entity/animal/AetherMobs.h"
#include "../item/Item.h"
#include "../item/ItemInstance.h"
#include "../../util/Random.h"
#include "../../platform/log.h"
#include <algorithm>

namespace AetherDungeons {

// ----------------------------------------------------------------------
// Placement: three dungeons per world, spread around the world square.
// ----------------------------------------------------------------------
void dungeonChunk(Level* level, int tier, int& outXt, int& outZt) {
	Random r(level->getSeed() ^ (0x4AE7E0 + tier * 7919));
	// keep away from the world border and from the central spawn island
	const int margin = 2;
	const int chunks = CHUNK_CACHE_WIDTH;
	int third = chunks / 3;
	// each tier gets its own third of the map along x, random z
	int xt = margin + tier * third + r.nextInt(std::max(1, third - margin));
	int zt = margin + r.nextInt(std::max(1, chunks - 2 * margin));
	// bronze/gold are never in the very middle column where spawn sits
	if (tier != Aether::Dungeon::Silver && xt >= chunks / 2 - 1 && xt <= chunks / 2 + 1) xt += 2;
	if (xt >= chunks - margin) xt = chunks - margin - 1;
	outXt = xt;
	outZt = zt;
}

// ----------------------------------------------------------------------
// Builder helpers
// ----------------------------------------------------------------------
struct Builder {
	Level* level;
	Random* random;
	std::vector<Pos> chests;

	Builder(Level* l, Random* r) : level(l), random(r) {}

	void set(int x, int y, int z, Tile* tile, int data = 0) {
		if (!tile || y < 1 || y >= 127) return;
		if (x < 1 || z < 1 || x >= LEVEL_WIDTH - 1 || z >= LEVEL_DEPTH - 1) return;
		level->setTileAndDataNoUpdate(x, y, z, tile->id, data);
	}
	void air(int x, int y, int z) {
		if (y < 1 || y >= 127) return;
		if (x < 1 || z < 1 || x >= LEVEL_WIDTH - 1 || z >= LEVEL_DEPTH - 1) return;
		level->setTileAndDataNoUpdate(x, y, z, 0, 0);
	}
	void fill(int x0, int y0, int z0, int x1, int y1, int z1, Tile* tile, int data = 0) {
		for (int x = std::min(x0, x1); x <= std::max(x0, x1); ++x)
			for (int y = std::min(y0, y1); y <= std::max(y0, y1); ++y)
				for (int z = std::min(z0, z1); z <= std::max(z0, z1); ++z)
					set(x, y, z, tile, data);
	}
	void clear(int x0, int y0, int z0, int x1, int y1, int z1) {
		for (int x = std::min(x0, x1); x <= std::max(x0, x1); ++x)
			for (int y = std::min(y0, y1); y <= std::max(y0, y1); ++y)
				for (int z = std::min(z0, z1); z <= std::max(z0, z1); ++z)
					air(x, y, z);
	}
	// Walls of `wall` with random `light` blocks mixed in, interior cleared.
	void room(int x0, int y0, int z0, int x1, int y1, int z1, Tile* wall, Tile* light, int lightChance) {
		for (int x = std::min(x0, x1); x <= std::max(x0, x1); ++x)
			for (int y = std::min(y0, y1); y <= std::max(y0, y1); ++y)
				for (int z = std::min(z0, z1); z <= std::max(z0, z1); ++z) {
					bool shell = x == x0 || x == x1 || y == y0 || y == y1 || z == z0 || z == z1;
					if (shell) set(x, y, z, (light && random->nextInt(lightChance) == 0) ? light : wall);
					else air(x, y, z);
				}
	}
	void chest(int x, int y, int z, int facing = 3) {
		set(x, y, z, Aether::skyrootChest, facing);
		chests.push_back(Pos(x, y, z));
	}
	void mimic(int x, int y, int z) {
		set(x, y, z, Aether::chestMimic, 3);
	}
	void pillar(int x, int y0, int y1, int z) {
		for (int y = y0; y <= y1; ++y) set(x, y, z, Aether::pillar, 0);
	}
	Mob* guard(int type, int x, int y, int z) {
		Mob* m = Aether::spawnMob(level, type, x + 0.5f, (float)y, z + 0.5f);
		if (m) m->persistent = true;
		return m;
	}
};

static int islandTopY(Level* level, int x, int z) {
	for (int y = AetherRandomLevelSource::ISLAND_MAX_Y + 4; y >= AetherRandomLevelSource::ISLAND_MIN_Y; --y) {
		int t = level->getTile(x, y, z);
		if (t == Aether::aetherGrass->id || t == Aether::enchantedGrass->id || t == Aether::holystone->id || t == Aether::quicksoil->id) return y;
	}
	return -1;
}

static void fillLoot(Level* level, const Pos& p, Random* random, int tier) {
	ChestTileEntity* chest = dynamic_cast<ChestTileEntity*>(level->getTileEntity(p.x, p.y, p.z));
	if (!chest) return;
	int size = chest->getContainerSize();
	int count = 3 + random->nextInt(3) + tier;
	for (int i = 0; i < count; ++i) {
		ItemInstance inst;
		int r = random->nextInt(100);
		if (r < 20) inst = ItemInstance(Aether::ambrosiumShard, 2 + random->nextInt(6), 0);
		else if (r < 35) inst = ItemInstance(Aether::zaniteGem, 1 + random->nextInt(4), 0);
		else if (r < 45) inst = ItemInstance(Aether::blueberry, 3 + random->nextInt(6), 0);
		else if (r < 52) inst = ItemInstance(Aether::goldenAmber, 1 + random->nextInt(2), 0);
		else if (r < 60) inst = ItemInstance(Aether::moaEgg, 1, 0);
		else if (r < 68) inst = ItemInstance(Aether::gravititePlate, 1 + random->nextInt(2), 0);
		else if (r < 76) inst = ItemInstance(Aether::ambrosiumTorch, 4 + random->nextInt(8), 0);
		else if (r < 84) inst = ItemInstance(tier >= 1 ? Aether::zaniteSword : Aether::skyrootSword, 1, 0);
		else if (r < 92) inst = ItemInstance(tier >= 2 ? Aether::gravititePickaxe : Aether::zanitePickaxe, 1, 0);
		else inst = ItemInstance(Aether::enchantedBlueberry, 1 + random->nextInt(2), 0);
		int slot = random->nextInt(size);
		for (int tries = 0; tries < size; ++tries) {
			ItemInstance* existing = chest->getItem(slot);
			if (!existing || existing->isNull()) break;
			slot = (slot + 1) % size;
		}
		chest->setItem(slot, &inst);
	}
	// every dungeon chest carries the key of its own tier
	Item* key = tier == Aether::Dungeon::Bronze ? Aether::bronzeKey : (tier == Aether::Dungeon::Silver ? Aether::silverKey : Aether::goldKey);
	ItemInstance k(key, 1, 0);
	for (int slot = 0; slot < size; ++slot) {
		ItemInstance* existing = chest->getItem(slot);
		if (!existing || existing->isNull()) { chest->setItem(slot, &k); break; }
	}
}

// ----------------------------------------------------------------------
// Bronze: a buried maze of carved stone rooms with trapped floors.
// ----------------------------------------------------------------------
static void buildBronze(Builder& b, int cx, int cz, int top) {
	Level* level = b.level;
	int y0 = top - 12;
	if (y0 < AetherRandomLevelSource::ISLAND_MIN_Y + 2) y0 = AetherRandomLevelSource::ISLAND_MIN_Y + 2;
	Tile* wall = Aether::carvedStone;
	Tile* light = Aether::lightSentryStone;

	// Outer shell 21x8x21
	b.room(cx - 10, y0, cz - 10, cx + 10, y0 + 7, cz + 10, wall, light, 9);
	// Inner cross walls of sentry stone with door gaps
	for (int i = -9; i <= 9; ++i) {
		if (i == -1 || i == 0 || i == 1) continue;
		b.fill(cx + i, y0 + 1, cz, cx + i, y0 + 6, cz, Aether::sentryStone);
		b.fill(cx, y0 + 1, cz + i, cx, y0 + 6, cz + i, Aether::sentryStone);
	}
	// Trapped floor tiles
	for (int x = cx - 9; x <= cx + 9; ++x)
		for (int z = cz - 9; z <= cz + 9; ++z)
			if (b.random->nextInt(9) == 0) b.set(x, y0, z, Aether::trappedSentryStone);
	// Pillars in each quadrant
	int px[4] = { cx - 5, cx + 5, cx - 5, cx + 5 };
	int pz[4] = { cz - 5, cz - 5, cz + 5, cz + 5 };
	for (int i = 0; i < 4; ++i) {
		b.pillar(px[i], y0 + 1, y0 + 6, pz[i]);
		if (b.random->nextInt(2) == 0) b.chest(px[i] + 1, y0 + 1, pz[i]);
		else b.mimic(px[i] + 1, y0 + 1, pz[i]);
	}
	// Boss room: locked carved stone box in the middle
	b.fill(cx - 3, y0 + 1, cz - 3, cx + 3, y0 + 6, cz + 3, Aether::lockedCarved);
	b.clear(cx - 2, y0 + 1, cz - 2, cx + 2, y0 + 5, cz + 2);
	b.chest(cx, y0 + 1, cz + 2);
	b.chest(cx - 2, y0 + 1, cz);
	// Entrance shaft from the surface, marked with light stone
	for (int y = y0 + 7; y <= top + 1; ++y) { b.air(cx + 8, y, cz + 8); b.air(cx + 8, y, cz + 9); }
	b.set(cx + 7, top + 1, cz + 8, light);
	b.set(cx + 9, top + 1, cz + 9, light);
	// Guards
	for (int i = 0; i < 6; ++i) b.guard(MobTypes::Sentry, cx + b.random->nextInt(15) - 7, y0 + 1, cz + b.random->nextInt(15) - 7);
	Mob* boss = b.guard(MobTypes::AetherBoss, cx, y0 + 1, cz);
	if (AetherBoss* ab = dynamic_cast<AetherBoss*>(boss)) ab->setTier(Aether::Dungeon::Bronze);
	(void)level;
}

// ----------------------------------------------------------------------
// Silver: an angelic temple on the surface with a central throne room.
// ----------------------------------------------------------------------
static void buildSilver(Builder& b, int cx, int cz, int top) {
	int y0 = top + 1;
	Tile* wall = Aether::angelicStone;
	Tile* light = Aether::lightAngelicStone;
	// Platform
	b.fill(cx - 12, y0 - 1, cz - 12, cx + 12, y0 - 1, cz + 12, wall);
	// Main hall
	b.room(cx - 11, y0, cz - 11, cx + 11, y0 + 9, cz + 11, wall, light, 7);
	// Colonnade
	for (int i = -8; i <= 8; i += 4) {
		b.pillar(cx + i, y0 + 1, y0 + 8, cz - 8);
		b.pillar(cx + i, y0 + 1, y0 + 8, cz + 8);
		b.pillar(cx - 8, y0 + 1, y0 + 8, cz + i);
		b.pillar(cx + 8, y0 + 1, y0 + 8, cz + i);
	}
	// Second floor ring
	b.fill(cx - 10, y0 + 5, cz - 10, cx + 10, y0 + 5, cz + 10, wall);
	b.clear(cx - 6, y0 + 5, cz - 6, cx + 6, y0 + 5, cz + 6);
	// Entrance doorway (south side) and windows of quicksoil glass
	b.clear(cx - 1, y0 + 1, cz + 11, cx + 1, y0 + 3, cz + 11);
	for (int i = -9; i <= 9; i += 3) {
		b.set(cx + i, y0 + 3, cz - 11, Aether::quicksoilGlass);
		b.set(cx + i, y0 + 7, cz - 11, Aether::quicksoilGlass);
		b.set(cx - 11, y0 + 3, cz + i, Aether::quicksoilGlass);
		b.set(cx + 11, y0 + 7, cz + i, Aether::quicksoilGlass);
	}
	// Treasure alcoves
	b.chest(cx - 9, y0 + 1, cz - 9);
	b.chest(cx + 9, y0 + 1, cz - 9);
	b.mimic(cx - 9, y0 + 1, cz + 9);
	b.chest(cx + 9, y0 + 6, cz + 9);
	// Throne room: locked angelic stone core
	b.fill(cx - 4, y0 + 1, cz - 4, cx + 4, y0 + 8, cz + 4, Aether::lockedAngelic);
	b.clear(cx - 3, y0 + 1, cz - 3, cx + 3, y0 + 7, cz + 3);
	b.fill(cx - 1, y0 + 1, cz - 3, cx + 1, y0 + 1, cz - 2, Aether::pillar);
	b.chest(cx, y0 + 2, cz - 3);
	b.chest(cx + 3, y0 + 1, cz);
	// Guards
	for (int i = 0; i < 5; ++i) b.guard(MobTypes::Sentry, cx + b.random->nextInt(17) - 8, y0 + 1, cz + b.random->nextInt(17) - 8);
	Mob* boss = b.guard(MobTypes::AetherBoss, cx, y0 + 1, cz + 1);
	if (AetherBoss* ab = dynamic_cast<AetherBoss*>(boss)) ab->setTier(Aether::Dungeon::Silver);
}

// ----------------------------------------------------------------------
// Gold: a hellfire stone vault on a pillar of holystone.
// ----------------------------------------------------------------------
static void buildGold(Builder& b, int cx, int cz, int top) {
	int y0 = top + 3;
	Tile* wall = Aether::hellfireStone;
	Tile* light = Aether::lightHellfireStone;
	// Raised base
	b.fill(cx - 9, top + 1, cz - 9, cx + 9, y0 - 1, cz + 9, Aether::holystoneBricks);
	// Vault
	b.room(cx - 8, y0, cz - 8, cx + 8, y0 + 8, cz + 8, wall, light, 6);
	// Lava moat around the core (contained by the base)
	for (int x = cx - 6; x <= cx + 6; ++x)
		for (int z = cz - 6; z <= cz + 6; ++z)
			if ((std::abs(x - cx) == 6 || std::abs(z - cz) == 6)) b.set(x, y0, z, Tile::calmLava);
	// Bridges over the moat
	b.set(cx, y0, cz - 6, wall); b.set(cx, y0, cz + 6, wall); b.set(cx - 6, y0, cz, wall); b.set(cx + 6, y0, cz, wall);
	// Entrance (west)
	b.clear(cx - 8, y0 + 1, cz - 1, cx - 8, y0 + 3, cz + 1);
	// Stairs up to the entrance made of holystone brick slabs
	for (int i = 0; i < 4; ++i) b.fill(cx - 9 - i, top + 1, cz - 1, cx - 9 - i, y0 - 1 - i, cz + 1, Aether::holystoneBricks);
	// Corner treasuries
	b.chest(cx - 7, y0 + 1, cz - 7);
	b.chest(cx + 7, y0 + 1, cz + 7);
	b.mimic(cx + 7, y0 + 1, cz - 7);
	b.mimic(cx - 7, y0 + 1, cz + 7);
	// Sun Spirit's chamber: locked hellfire core with the Sun Altar
	b.fill(cx - 3, y0 + 1, cz - 3, cx + 3, y0 + 7, cz + 3, Aether::lockedHellfire);
	b.clear(cx - 2, y0 + 1, cz - 2, cx + 2, y0 + 6, cz + 2);
	b.set(cx, y0 + 1, cz, Aether::sunAltar);
	b.chest(cx - 2, y0 + 1, cz - 2);
	b.chest(cx + 2, y0 + 1, cz + 2);
	// Guards
	for (int i = 0; i < 6; ++i) b.guard(MobTypes::Sentry, cx + b.random->nextInt(11) - 5, y0 + 1, cz + b.random->nextInt(11) - 5);
	Mob* boss = b.guard(MobTypes::AetherBoss, cx + 1, y0 + 1, cz + 1);
	if (AetherBoss* ab = dynamic_cast<AetherBoss*>(boss)) ab->setTier(Aether::Dungeon::Gold);
}

void placeForChunk(Level* level, int xt, int zt) {
	if (!level || level->isClientSide) return;
	for (int tier = 0; tier < 3; ++tier) {
		int dx, dz;
		dungeonChunk(level, tier, dx, dz);
		if (dx != xt || dz != zt) continue;

		Random random(level->getSeed() ^ (0x7A3D1 + tier * 104729));
		int cx = xt * 16 + 8, cz = zt * 16 + 8;

		// Find (or make) ground: dungeons need an island under them.
		int top = islandTopY(level, cx, cz);
		if (top < 0) {
			// no island here - raise one so the dungeon has a foundation
			top = 70;
			Builder ground(level, &random);
			for (int x = cx - 14; x <= cx + 14; ++x)
				for (int z = cz - 14; z <= cz + 14; ++z) {
					int d2 = (x - cx) * (x - cx) + (z - cz) * (z - cz);
					if (d2 > 14 * 14) continue;
					int depth = 4 + (int)((14 * 14 - d2) / 12.0f);
					for (int y = top; y > top - depth; --y)
						ground.set(x, y, z, y == top ? Aether::aetherGrass : (y > top - 3 ? Aether::aetherDirt : Aether::holystone));
				}
		}

		Builder b(level, &random);
		bool wasGenerating = level->isGeneratingTerrain;
		bool savedNoNeighbor = level->noNeighborUpdate;
		level->isGeneratingTerrain = true;
		level->noNeighborUpdate = true;

		if (tier == Aether::Dungeon::Bronze) buildBronze(b, cx, cz, top);
		else if (tier == Aether::Dungeon::Silver) buildSilver(b, cx, cz, top);
		else buildGold(b, cx, cz, top);

		for (unsigned int i = 0; i < b.chests.size(); ++i) fillLoot(level, b.chests[i], &random, tier);

		level->noNeighborUpdate = savedNoNeighbor;
		level->isGeneratingTerrain = wasGenerating;
		LOGI("Aether dungeon tier %d placed at %d, %d, %d\n", tier, cx, top, cz);
	}
}

} // namespace AetherDungeons
