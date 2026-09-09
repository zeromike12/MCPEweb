#include "RpgDungeons.h"
#include "Rpg.h"

#include <cmath>
#include <vector>
#include <algorithm>

#include "../level/Level.h"
#include "../level/LevelConstants.h"
#include "../level/LevelSettings.h"
#include "../level/storage/LevelData.h"
#include "../level/tile/Tile.h"
#include "../level/material/Material.h"
#include "../level/dimension/Dimension.h"
#include "../entity/Mob.h"
#include "../entity/MobFactory.h"
#include "../entity/EntityTypes.h"
#include "../../util/Random.h"
#include "../../util/Mth.h"
#include "../../platform/log.h"

namespace RpgDungeons {

// ----------------------------------------------------------------------
// Building helpers
// ----------------------------------------------------------------------
struct Builder {
	Level* level;
	Random* random;
	int baseLevel;          // mob level of the dungeon guards
	std::vector<Pos> chests;
	std::vector<Pos> bossChests;

	Builder(Level* l, Random* r, int lvl) : level(l), random(r), baseLevel(lvl) {}

	void set(int x, int y, int z, Tile* tile, int data = 0) {
		if (y < 1 || y >= 127 || !tile) return;
		if (x < 1 || z < 1 || x >= LEVEL_WIDTH - 1 || z >= LEVEL_DEPTH - 1) return;
		level->setTileAndDataNoUpdate(x, y, z, tile->id, data);
	}
	void air(int x, int y, int z) {
		if (y < 1 || y >= 127) return;
		if (x < 1 || z < 1 || x >= LEVEL_WIDTH - 1 || z >= LEVEL_DEPTH - 1) return;
		level->setTileAndDataNoUpdate(x, y, z, 0, 0);
	}

	// Filled box (inclusive coordinates)
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
	// Hollow box: walls of `wall`, interior cleared
	void room(int x0, int y0, int z0, int x1, int y1, int z1, Tile* wall, Tile* floor = NULL, Tile* ceiling = NULL) {
		fill(x0, y0, z0, x1, y1, z1, wall);
		clear(x0 + 1, y0 + 1, z0 + 1, x1 - 1, y1 - 1, z1 - 1);
		if (floor) fill(x0, y0, z0, x1, y0, z1, floor);
		if (ceiling) fill(x0, y1, z0, x1, y1, z1, ceiling);
	}
	// Mossy / cracked stone brick look: mostly stone bricks with random moss / cracked variants
	Tile* brick() {
		int r = random->nextInt(10);
		if (r == 0) return Tile::mossStone;
		return Tile::stoneBrickSmooth;
	}
	int brickData() {
		int r = random->nextInt(6);
		if (r == 0) return 1; // mossy stone brick
		if (r == 1) return 2; // cracked stone brick
		return 0;
	}
	void brickFill(int x0, int y0, int z0, int x1, int y1, int z1) {
		for (int x = std::min(x0, x1); x <= std::max(x0, x1); ++x)
			for (int y = std::min(y0, y1); y <= std::max(y0, y1); ++y)
				for (int z = std::min(z0, z1); z <= std::max(z0, z1); ++z) {
					Tile* t = brick();
					set(x, y, z, t, t == Tile::stoneBrickSmooth ? brickData() : 0);
				}
	}
	void brickRoom(int x0, int y0, int z0, int x1, int y1, int z1) {
		brickFill(x0, y0, z0, x1, y1, z1);
		clear(x0 + 1, y0 + 1, z0 + 1, x1 - 1, y1 - 1, z1 - 1);
	}
	void torch(int x, int y, int z) {
		// standing torch on top of a solid block
		if (level->isSolidBlockingTile(x, y - 1, z)) set(x, y, z, Tile::torch, 5);
	}
	void glowstone(int x, int y, int z) { set(x, y, z, Tile::lightGem); }
	void chest(int x, int y, int z, bool boss = false) {
		set(x, y, z, Tile::chest, 0);
		if (boss) bossChests.push_back(Pos(x, y, z));
		else chests.push_back(Pos(x, y, z));
	}
	// Two-block bed: foot at (x,y,z) facing +z, head at z+1
	void bed(int x, int y, int z) {
		set(x, y, z, Tile::bed, 0);
		set(x, y, z + 1, Tile::bed, 8);
	}
	void ladder(int x, int y0, int y1, int z, int data) {
		for (int y = y0; y <= y1; ++y) set(x, y, z, Tile::ladder, data);
	}
	void webs(int x0, int y0, int z0, int x1, int y1, int z1, int chancePercent) {
		for (int x = x0; x <= x1; ++x)
			for (int y = y0; y <= y1; ++y)
				for (int z = z0; z <= z1; ++z)
					if (random->nextInt(100) < chancePercent && level->isEmptyTile(x, y, z))
						set(x, y, z, Tile::web);
	}
	// Vertical pillar
	void pillar(int x, int y0, int y1, int z, Tile* tile, int data = 0) {
		for (int y = y0; y <= y1; ++y) set(x, y, z, tile, data);
	}

	// Spawns a persistent guard of the given type at the given position.
	Mob* guard(int type, float x, float y, float z, int levelBonus = 0) {
		if (level->isClientSide) return NULL;
		Mob* mob = MobFactory::CreateMob(type, level);
		if (!mob) return NULL;
		mob->moveTo(x + 0.5f, y, z + 0.5f, random->nextFloat() * 360, 0);
		mob->persistent = true;
		int lvl = baseLevel + levelBonus + random->nextInt(3) - 1;
		mob->setRpgLevel(lvl);
		level->addEntity(mob);
		mob->health = mob->getScaledMaxHealth();
		return mob;
	}
	int randomMonster() {
		switch (random->nextInt(4)) {
		case 0: return MobTypes::Zombie;
		case 1: return MobTypes::Skeleton;
		case 2: return MobTypes::Spider;
		default: return MobTypes::Zombie;
		}
	}
};

// Finds the surface height at (x,z): the y of the first air block above the top solid block.
static int surfaceY(Level* level, int x, int z) {
	// walk down from the top, ignoring trees so forests don't block placement
	for (int y = 126; y > 1; --y) {
		int t = level->getTile(x, y, z);
		if (t == 0) continue;
		Tile* tile = Tile::tiles[t];
		if (!tile || !tile->material->blocksMotion()) continue;
		if (tile->material == Material::leaves || t == Tile::treeTrunk->id) continue;
		return y + 1;
	}
	return -1;
}

// Average surface height over a footprint; returns -1 if the area is too uneven or watery.
static int footprintY(Level* level, int x0, int z0, int x1, int z1, int maxSlope) {
	int minY = 999, maxY = -1;
	long sum = 0; int n = 0;
	for (int x = x0; x <= x1; x += 2)
		for (int z = z0; z <= z1; z += 2) {
			int y = surfaceY(level, x, z);
			if (y <= 2 || y >= 120) return -1;
			const Material* m = level->getMaterial(x, y, z);
			if (m == Material::water || m == Material::lava) return -1;
			minY = std::min(minY, y); maxY = std::max(maxY, y);
			sum += y; n++;
		}
	if (n == 0) return -1;
	if (maxY - minY > maxSlope) return -1;
	return (int) (sum / n);
}

// ----------------------------------------------------------------------
// Dungeon layouts
// Each returns false if it couldn't be placed at (cx, cz).
// ----------------------------------------------------------------------

// 1. Stone brick watchtower with a spiral of ladders and a roof-top chest
static bool buildWatchtower(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 4, cz - 4, cx + 4, cz + 4, 6);
	if (y < 0) return false;
	int h = 14 + b.random->nextInt(5);
	// foundation
	b.brickFill(cx - 3, y - 3, cz - 3, cx + 3, y - 1, cz + 3);
	// tower
	b.brickRoom(cx - 3, y - 1, cz - 3, cx + 3, y + h, cz + 3);
	// door
	b.clear(cx, y, cz - 3, cx, y + 1, cz - 3);
	// ladder up the back wall
	b.ladder(cx, y, y + h - 1, cz + 2, 2);
	// floors every 5 blocks with a hole for the ladder
	for (int fy = y + 5; fy < y + h - 1; fy += 5) {
		b.fill(cx - 2, fy, cz - 2, cx + 2, fy, cz + 2, Tile::wood);
		b.air(cx, fy, cz + 2);
		b.torch(cx - 2, fy + 1, cz - 2);
		b.guard(b.randomMonster(), cx - 1, fy + 1, cz);
	}
	// crenellations
	for (int x = cx - 3; x <= cx + 3; ++x)
		for (int z = cz - 3; z <= cz + 3; ++z)
			if ((x == cx - 3 || x == cx + 3 || z == cz - 3 || z == cz + 3) && ((x + z) & 1))
				b.set(x, y + h + 1, z, Tile::stoneBrickSmooth);
	b.air(cx, y + h, cz + 2); // ladder exit
	// roof chest + boss
	b.chest(cx, y + h + 1, cz - 1, true);
	b.guard(MobTypes::Skeleton, cx + 1, y + h + 1, cz + 1, 4);
	b.guard(MobTypes::Skeleton, cx - 1, y + h + 1, cz + 1, 4);
	// ground floor guards
	b.guard(MobTypes::Zombie, cx + 1, y, cz + 1);
	b.guard(MobTypes::Zombie, cx - 1, y, cz);
	b.torch(cx + 2, y, cz + 2);
	return true;
}

// 2. Buried crypt: staircase down from a small shrine to a corridor of tombs
static bool buildCrypt(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 3, cz - 3, cx + 3, cz + 3, 5);
	if (y < 0) return false;
	int depth = 12 + b.random->nextInt(6);
	int by = y - depth;
	if (by < 6) by = 6;
	// shrine on the surface
	b.brickRoom(cx - 2, y - 1, cz - 2, cx + 2, y + 3, cz + 2);
	b.clear(cx, y, cz - 2, cx, y + 1, cz - 2);
	b.torch(cx - 1, y, cz + 1); b.torch(cx + 1, y, cz + 1);
	// vertical shaft with ladder
	b.clear(cx, by, cz, cx, y, cz);
	b.fill(cx - 1, by - 1, cz - 1, cx + 1, y - 1, cz + 1, Tile::stoneBrickSmooth);
	b.clear(cx, by, cz, cx, y, cz);
	b.ladder(cx, by, y - 1, cz + 1, 2);
	b.air(cx, y - 1, cz); // opening in the shrine floor
	// main corridor heading +x
	int len = 24 + b.random->nextInt(10);
	b.brickRoom(cx, by - 1, cz - 2, cx + len, by + 4, cz + 2);
	b.air(cx, by, cz); b.air(cx, by + 1, cz);
	b.clear(cx + 1, by, cz - 1, cx + len - 1, by + 3, cz + 1);
	// tombs alternating on each side
	for (int i = 4; i < len - 3; i += 5) {
		int side = ((i / 5) & 1) ? 1 : -1;
		int tz = cz + side * 2;
		int tz2 = cz + side * 5;
		b.brickRoom(cx + i - 1, by - 1, std::min(tz, tz2), cx + i + 1, by + 3, std::max(tz, tz2));
		b.air(cx + i, by, tz); b.air(cx + i, by + 1, tz);
		b.clear(cx + i, by, std::min(tz, tz2) + 1, cx + i, by + 2, std::max(tz, tz2) - 1);
		if (b.random->nextInt(3) == 0) b.chest(cx + i, by, cz + side * 4);
		b.guard(b.random->nextInt(2) ? MobTypes::Zombie : MobTypes::Skeleton, cx + i, by, cz + side * 3);
		b.webs(cx + i - 1, by, cz - 1, cx + i + 1, by + 2, cz + 1, 15);
		if ((i / 5) % 2 == 0) b.torch(cx + i, by, cz - side * 1);
	}
	// burial chamber at the end
	b.brickRoom(cx + len - 1, by - 1, cz - 5, cx + len + 8, by + 5, cz + 5);
	b.clear(cx + len, by, cz - 1, cx + len, by + 2, cz + 1);
	b.pillar(cx + len + 2, by, by + 3, cz - 3, Tile::stoneBrickSmooth);
	b.pillar(cx + len + 2, by, by + 3, cz + 3, Tile::stoneBrickSmooth);
	b.pillar(cx + len + 6, by, by + 3, cz - 3, Tile::stoneBrickSmooth);
	b.pillar(cx + len + 6, by, by + 3, cz + 3, Tile::stoneBrickSmooth);
	b.glowstone(cx + len + 4, by + 4, cz);
	b.fill(cx + len + 6, by, cz - 1, cx + len + 7, by, cz + 1, Tile::stoneSlabHalf);
	b.chest(cx + len + 7, by, cz, true);
	b.chest(cx + len + 5, by, cz - 4);
	b.guard(MobTypes::Zombie, cx + len + 4, by, cz, 5);
	b.guard(MobTypes::Skeleton, cx + len + 3, by, cz - 3, 3);
	b.guard(MobTypes::Skeleton, cx + len + 3, by, cz + 3, 3);
	b.guard(MobTypes::Spider, cx + len + 6, by, cz + 3, 2);
	return true;
}

// 3. Ruined temple: open-air colonnade with a sunken altar room
static bool buildRuinedTemple(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 9, cz - 9, cx + 9, cz + 9, 7);
	if (y < 0) return false;
	// platform
	b.fill(cx - 9, y - 5, cz - 9, cx + 9, y - 1, cz + 9, Tile::stoneBrickSmooth);
	b.brickFill(cx - 9, y - 1, cz - 9, cx + 9, y - 1, cz + 9);
	b.clear(cx - 9, y, cz - 9, cx + 9, y + 8, cz + 9);
	// pillars (some broken)
	for (int x = cx - 8; x <= cx + 8; x += 4)
		for (int z = cz - 8; z <= cz + 8; z += 4) {
			if (std::abs(x - cx) < 8 && std::abs(z - cz) < 8) continue;
			int h = 5 + b.random->nextInt(3);
			if (b.random->nextInt(4) == 0) h = 1 + b.random->nextInt(3); // broken
			b.pillar(x, y, y + h, z, Tile::stoneBrickSmooth);
		}
	// partial roof beams
	for (int x = cx - 8; x <= cx + 8; ++x)
		if (b.random->nextInt(3)) { b.set(x, y + 7, cz - 8, Tile::stoneSlabHalf); b.set(x, y + 7, cz + 8, Tile::stoneSlabHalf); }
	// sunken altar room in the middle
	b.brickRoom(cx - 4, y - 7, cz - 4, cx + 4, y - 1, cz + 4);
	b.clear(cx - 3, y - 6, cz - 3, cx + 3, y - 1, cz + 3); // open to the sky
	// stairs down on the -z side
	for (int i = 0; i < 6; ++i) {
		b.clear(cx, y - 1 - i, cz - 4 - 2 + i, cx, y - i + 1, cz - 4 - 2 + i);
	}
	for (int i = 0; i < 6; ++i)
		b.set(cx, y - 1 - i, cz - 6 + i, Tile::stairs_stoneBrickSmooth, 3);
	b.clear(cx, y - 6, cz - 4, cx, y - 4, cz - 4);
	// altar
	b.fill(cx - 1, y - 6, cz - 1, cx + 1, y - 6, cz + 1, Tile::obsidian);
	b.set(cx, y - 5, cz, Tile::lightGem);
	b.chest(cx, y - 6, cz + 2, true);
	b.chest(cx - 3, y - 6, cz + 3);
	b.chest(cx + 3, y - 6, cz - 3);
	// guards
	b.guard(MobTypes::Skeleton, cx - 6, y, cz - 6, 1);
	b.guard(MobTypes::Skeleton, cx + 6, y, cz + 6, 1);
	b.guard(MobTypes::Zombie, cx + 6, y, cz - 6);
	b.guard(MobTypes::Zombie, cx - 6, y, cz + 6);
	b.guard(MobTypes::Spider, cx - 2, y - 6, cz + 2, 3);
	b.guard(MobTypes::Spider, cx + 2, y - 6, cz - 2, 3);
	b.guard(MobTypes::Zombie, cx + 2, y - 6, cz + 2, 5);
	b.torch(cx - 8, y, cz); b.torch(cx + 8, y, cz);
	return true;
}

// 4. Spider nest: underground cavern full of webs, entered through a hole
static bool buildSpiderNest(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 2, cz - 2, cx + 2, cz + 2, 5);
	if (y < 0) return false;
	int by = y - 14;
	if (by < 8) by = 8;
	// entrance pit
	b.clear(cx - 1, by + 1, cz - 1, cx + 1, y + 1, cz + 1);
	b.ladder(cx, by + 1, y, cz + 2, 2);
	// irregular cavern: union of a few boxes
	int r = 7;
	b.fill(cx - r - 1, by - 1, cz - r - 1, cx + r + 1, by + 6, cz + r + 1, Tile::rock);
	b.clear(cx - r, by, cz - r, cx + r, by + 5, cz + r);
	b.clear(cx - r - 3, by, cz - 3, cx + r + 3, by + 3, cz + 3);
	b.clear(cx - 3, by, cz - r - 3, cx + 3, by + 3, cz + r + 3);
	for (int i = 0; i < 12; ++i) {
		int px = cx + b.random->nextInt(2 * r) - r;
		int pz = cz + b.random->nextInt(2 * r) - r;
		b.pillar(px, by, by + 5, pz, Tile::rock);
	}
	b.fill(cx - r, by - 1, cz - r, cx + r, by - 1, cz + r, Tile::mossStone);
	b.air(cx, by, cz + 2); // ladder foot
	b.webs(cx - r, by, cz - r, cx + r, by + 5, cz + r, 20);
	// egg sac corners with chests
	b.chest(cx - r + 1, by, cz - r + 1);
	b.chest(cx + r - 1, by, cz + r - 1);
	b.chest(cx + r - 1, by, cz - r + 1, true);
	b.glowstone(cx, by + 5, cz);
	for (int i = 0; i < 7; ++i)
		b.guard(MobTypes::Spider, cx + b.random->nextInt(2 * r - 2) - r + 1, by, cz + b.random->nextInt(2 * r - 2) - r + 1, i == 0 ? 6 : 1);
	return true;
}

// 5. Fortress: walled compound with corner towers, barracks and a treasury
static bool buildFortress(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 12, cz - 12, cx + 12, cz + 12, 8);
	if (y < 0) return false;
	int R = 12;
	// level the ground
	b.fill(cx - R, y - 6, cz - R, cx + R, y - 1, cz + R, Tile::stoneBrick);
	b.brickFill(cx - R, y - 1, cz - R, cx + R, y - 1, cz + R);
	b.clear(cx - R, y, cz - R, cx + R, y + 12, cz + R);
	// outer wall
	b.brickRoom(cx - R, y - 1, cz - R, cx + R, y + 4, cz + R);
	b.clear(cx - R + 1, y, cz - R + 1, cx + R - 1, y + 4, cz + R - 1); // no roof
	for (int x = cx - R; x <= cx + R; ++x) { if (x & 1) { b.set(x, y + 5, cz - R, Tile::stoneBrickSmooth); b.set(x, y + 5, cz + R, Tile::stoneBrickSmooth); } }
	for (int z = cz - R; z <= cz + R; ++z) { if (z & 1) { b.set(cx - R, y + 5, z, Tile::stoneBrickSmooth); b.set(cx + R, y + 5, z, Tile::stoneBrickSmooth); } }
	// gate
	b.clear(cx - 1, y, cz - R, cx + 1, y + 2, cz - R);
	// corner towers
	int corners[4][2] = { {cx - R, cz - R}, {cx + R, cz - R}, {cx - R, cz + R}, {cx + R, cz + R} };
	for (int i = 0; i < 4; ++i) {
		int tx = corners[i][0], tz = corners[i][1];
		b.brickRoom(tx - 2, y - 1, tz - 2, tx + 2, y + 8, tz + 2);
		b.fill(tx - 2, y + 8, tz - 2, tx + 2, y + 8, tz + 2, Tile::wood);
		b.clear(tx - 1, y + 9, tz - 1, tx + 1, y + 9, tz + 1);
		int dx = tx < cx ? 1 : -1, dz = tz < cz ? 1 : -1;
		b.clear(tx + dx * 2, y, tz, tx + dx * 2, y + 1, tz); // door facing inside
		b.ladder(tx, y, y + 8, tz - dz * 1, dz > 0 ? 2 : 3);
		b.air(tx, y + 8, tz);
		b.guard(MobTypes::Skeleton, tx, y + 9, tz, 2);
		b.torch(tx + dx, y, tz + dz);
	}
	// barracks along +x wall
	b.brickRoom(cx + 4, y - 1, cz - 8, cx + R - 1, y + 4, cz + 2);
	b.clear(cx + 4, y, cz - 3, cx + 4, y + 1, cz - 3);
	for (int z = cz - 7; z <= cz; z += 3) { b.bed(cx + R - 2, y, z); }
	b.chest(cx + 5, y, cz - 7);
	b.torch(cx + 6, y, cz + 1);
	for (int i = 0; i < 4; ++i) b.guard(MobTypes::Zombie, cx + 6 + i, y, cz - 6 + i);
	// treasury along -x wall
	b.brickRoom(cx - R + 1, y - 1, cz - 8, cx - 4, y + 4, cz + 2);
	b.clear(cx - 4, y, cz - 3, cx - 4, y + 1, cz - 3);
	b.chest(cx - R + 2, y, cz - 7, true);
	b.chest(cx - R + 2, y, cz + 1);
	b.chest(cx - 5, y, cz - 7);
	b.glowstone(cx - 8, y + 3, cz - 3);
	b.guard(MobTypes::Zombie, cx - 7, y, cz - 3, 5);
	b.guard(MobTypes::Skeleton, cx - 9, y, cz - 5, 3);
	// courtyard
	b.torch(cx - 3, y, cz + 6); b.torch(cx + 3, y, cz + 6);
	b.guard(MobTypes::Zombie, cx, y, cz + 4, 1);
	b.guard(MobTypes::Zombie, cx - 2, y, cz + 8, 1);
	b.guard(MobTypes::Spider, cx + 3, y, cz + 9, 1);
	// keep at the back
	b.brickRoom(cx - 4, y - 1, cz + 4, cx + 4, y + 7, cz + R - 1);
	b.clear(cx, y, cz + 4, cx, y + 1, cz + 4);
	b.chest(cx, y, cz + R - 2, true);
	b.glowstone(cx, y + 6, cz + 8);
	b.guard(MobTypes::Zombie, cx, y, cz + 7, 7);
	b.guard(MobTypes::Skeleton, cx - 2, y, cz + 9, 4);
	b.guard(MobTypes::Skeleton, cx + 2, y, cz + 9, 4);
	return true;
}

// 6. Mine shaft: wooden-supported tunnels with rails and ore, deep underground
static bool buildMineshaft(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 2, cz - 2, cx + 2, cz + 2, 6);
	if (y < 0) return false;
	int by = 14 + b.random->nextInt(10);
	if (by > y - 10) by = y - 10;
	if (by < 6) return false;
	// shaft house
	b.room(cx - 2, y - 1, cz - 2, cx + 2, y + 3, cz + 2, Tile::wood, Tile::wood, Tile::wood);
	b.clear(cx, y, cz - 2, cx, y + 1, cz - 2);
	b.clear(cx, by, cz, cx, y - 1, cz);
	b.fill(cx - 1, by - 1, cz - 1, cx + 1, y - 1, cz + 1, Tile::wood);
	b.clear(cx, by, cz, cx, y - 1, cz);
	b.ladder(cx, by, y - 1, cz + 1, 2);
	// four tunnels
	int dirs[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
	for (int d = 0; d < 4; ++d) {
		int dx = dirs[d][0], dz = dirs[d][1];
		int len = 14 + b.random->nextInt(12);
		for (int i = 1; i <= len; ++i) {
			int x = cx + dx * i, z = cz + dz * i;
			// 3 wide, 3 tall
			b.fill(x - (dz ? 1 : 0), by - 1, z - (dx ? 1 : 0), x + (dz ? 1 : 0), by - 1, z + (dx ? 1 : 0), Tile::dirt);
			b.clear(x - (dz ? 1 : 0), by, z - (dx ? 1 : 0), x + (dz ? 1 : 0), by + 2, z + (dx ? 1 : 0));
			if (i % 2 == 0) b.set(x, by - 1, z, Tile::wood); // sleepers
			if (i % 4 == 0) {
				// support frame
				b.pillar(x - (dz ? 1 : 0), by, by + 2, z - (dx ? 1 : 0), Tile::fence);
				b.pillar(x + (dz ? 1 : 0), by, by + 2, z + (dx ? 1 : 0), Tile::fence);
				b.fill(x - (dz ? 1 : 0), by + 2, z - (dx ? 1 : 0), x + (dz ? 1 : 0), by + 2, z + (dx ? 1 : 0), Tile::wood);
				if (i % 8 == 0) b.set(x - (dz ? 1 : 0), by + 1, z - (dx ? 1 : 0), Tile::torch, 5);
			}
			if (b.random->nextInt(9) == 0) {
				Tile* ore = b.random->nextInt(5) == 0 ? Tile::goldOre : (b.random->nextInt(2) ? Tile::ironOre : Tile::coalOre);
				b.set(x + (dz ? 2 : 0), by + b.random->nextInt(3), z + (dx ? 2 : 0), ore);
			}
		}
		// dead-end chamber with loot
		int ex = cx + dx * (len + 1), ez = cz + dz * (len + 1);
		b.room(ex - 3, by - 1, ez - 3, ex + 3, by + 4, ez + 3, Tile::rock, Tile::dirt, Tile::rock);
		b.clear(ex - dx, by, ez - dz, ex - dx * 2, by + 2, ez - dz * 2);
		b.torch(ex + 2, by, ez + 2);
		b.chest(ex, by, ez, d == 0);
		b.webs(ex - 2, by, ez - 2, ex + 2, by + 3, ez + 2, 12);
		b.guard(d == 0 ? MobTypes::Zombie : b.randomMonster(), ex - 1, by, ez + 1, d == 0 ? 6 : 1);
		b.guard(MobTypes::Spider, ex + 1, by, ez - 1, 1);
		b.guard(b.randomMonster(), cx + dx * (len / 2), by, cz + dz * (len / 2));
	}
	b.guard(MobTypes::Zombie, cx + 1, y, cz + 1);
	return true;
}

// 7. Arena: circular pit with tiered seating; a boss waits in the middle
static bool buildArena(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 11, cz - 11, cx + 11, cz + 11, 8);
	if (y < 0) return false;
	int R = 11;
	for (int x = cx - R; x <= cx + R; ++x)
		for (int z = cz - R; z <= cz + R; ++z) {
			float d = std::sqrt((float) ((x - cx) * (x - cx) + (z - cz) * (z - cz)));
			if (d > R + 0.5f) continue;
			// tiers: floor of the pit at y-5, seating rising towards the edge
			int floorY;
			if (d < 6) floorY = y - 5;
			else floorY = y - 5 + (int) (d - 5); // 1 block per ring
			if (floorY > y + 1) floorY = y + 1;
			for (int yy = floorY; yy <= y + 6; ++yy) b.air(x, yy, z);
			b.fill(x, floorY - 2, z, x, floorY - 1, z, d < 6 ? Tile::sand : Tile::stoneBrickSmooth);
			if (d >= 5 && d < 6) b.set(x, floorY, z, Tile::fence); // barrier around the pit
		}
	// perimeter wall & torches
	for (int x = cx - R - 1; x <= cx + R + 1; ++x)
		for (int z = cz - R - 1; z <= cz + R + 1; ++z) {
			float d = std::sqrt((float) ((x - cx) * (x - cx) + (z - cz) * (z - cz)));
			if (d > R - 0.5f && d <= R + 1.5f) {
				b.fill(x, y - 1, z, x, y + 3, z, Tile::stoneBrickSmooth);
				if (((x + z) & 3) == 0) b.set(x, y + 4, z, Tile::torch, 5);
			}
		}
	// entrance ramp on -z
	for (int i = 0; i < 6; ++i) {
		b.clear(cx - 1, y - 5 + i, cz - R + 1 + i, cx + 1, y + 1, cz - R + 1 + i);
		b.fill(cx - 1, y - 6 + i, cz - R + 1 + i, cx + 1, y - 6 + i, cz - R + 1 + i, Tile::stoneBrickSmooth);
	}
	b.clear(cx - 1, y, cz - R - 1, cx + 1, y + 2, cz - R);
	b.clear(cx - 1, y - 5, cz - 6, cx + 1, y - 3, cz - 5); // gap in the barrier
	// gates around the pit hide chests under the seats
	b.chest(cx + 7, y - 3, cz, false);
	b.chest(cx - 7, y - 3, cz, false);
	b.clear(cx + 6, y - 3, cz, cx + 6, y - 2, cz);
	b.clear(cx - 6, y - 3, cz, cx - 6, y - 2, cz);
	// champion's chest on a plinth in the middle
	b.set(cx, y - 5, cz, Tile::obsidian);
	b.chest(cx, y - 4, cz, true);
	b.glowstone(cx, y + 6, cz);
	// combatants
	b.guard(MobTypes::Zombie, cx, y - 5, cz + 3, 8);   // champion
	b.guard(MobTypes::Spider, cx - 3, y - 5, cz - 2, 3);
	b.guard(MobTypes::Spider, cx + 3, y - 5, cz - 2, 3);
	b.guard(MobTypes::Skeleton, cx + 8, y - 2, cz + 3, 2); // archers in the stands
	b.guard(MobTypes::Skeleton, cx - 8, y - 2, cz - 3, 2);
	b.guard(MobTypes::Skeleton, cx + 2, y - 2, cz + 8, 2);
	return true;
}

// 8. Wizard's spire: tall narrow tower of obsidian & glass with a library and a summoning room
static bool buildWizardSpire(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 3, cz - 3, cx + 3, cz + 3, 6);
	if (y < 0) return false;
	int h = 22;
	b.fill(cx - 3, y - 5, cz - 3, cx + 3, y - 1, cz + 3, Tile::obsidian);
	b.room(cx - 3, y - 1, cz - 3, cx + 3, y + h, cz + 3, Tile::obsidian);
	// glass windows in a spiral
	for (int i = 0; i < h; i += 2) {
		int side = (i / 2) & 3;
		if (side == 0) b.set(cx, y + i + 1, cz - 3, Tile::glass);
		if (side == 1) b.set(cx + 3, y + i + 1, cz, Tile::glass);
		if (side == 2) b.set(cx, y + i + 1, cz + 3, Tile::glass);
		if (side == 3) b.set(cx - 3, y + i + 1, cz, Tile::glass);
	}
	b.clear(cx, y, cz - 3, cx, y + 1, cz - 3); // door
	b.ladder(cx + 2, y, y + h - 1, cz + 2, 2);
	// floors
	int floors[] = { y + 6, y + 12, y + 18 };
	for (int f = 0; f < 3; ++f) {
		int fy = floors[f];
		b.fill(cx - 2, fy, cz - 2, cx + 2, fy, cz + 2, Tile::wood);
		b.air(cx + 2, fy, cz + 2);
		b.glowstone(cx, fy + 4 < y + h ? fy + 4 : y + h - 1, cz);
		if (f == 0) {
			// library
			for (int z = cz - 2; z <= cz + 1; ++z) b.fill(cx - 2, fy + 1, z, cx - 2, fy + 3, z, Tile::bookshelf);
			b.fill(cx - 1, fy + 1, cz - 2, cx + 1, fy + 3, cz - 2, Tile::bookshelf);
			b.chest(cx + 1, fy + 1, cz - 1);
			b.guard(MobTypes::Skeleton, cx, fy + 1, cz + 1, 3);
		} else if (f == 1) {
			// alchemy room
			b.set(cx - 1, fy + 1, cz - 1, Tile::workBench);
			b.set(cx + 1, fy + 1, cz - 1, Tile::furnace);
			b.chest(cx - 1, fy + 1, cz + 1);
			b.guard(MobTypes::Zombie, cx, fy + 1, cz, 4);
			b.guard(MobTypes::Spider, cx + 1, fy + 1, cz + 1, 4);
		} else {
			// summoning room
			b.fill(cx - 1, fy, cz - 1, cx + 1, fy, cz + 1, Tile::obsidian);
			b.set(cx, fy, cz, Tile::glowingObsidian);
			b.chest(cx, fy + 1, cz - 1, true);
			b.guard(MobTypes::Skeleton, cx - 1, fy + 1, cz + 1, 8);
			b.guard(MobTypes::Skeleton, cx + 1, fy + 1, cz + 1, 8);
		}
	}
	// pointed roof
	for (int i = 0; i < 4; ++i)
		b.fill(cx - 3 + i, y + h + i, cz - 3 + i, cx + 3 - i, y + h + i, cz + 3 - i, Tile::obsidian);
	b.set(cx, y + h + 4, cz, Tile::lightGem);
	b.guard(MobTypes::Zombie, cx - 1, y, cz + 1, 1);
	return true;
}

// 9. Barrow mounds: a cluster of grass-covered burial mounds, each with a chamber
static bool buildBarrows(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 12, cz - 12, cx + 12, cz + 12, 9);
	if (y < 0) return false;
	int count = 4 + b.random->nextInt(2);
	int bossIndex = b.random->nextInt(count);
	for (int i = 0; i < count; ++i) {
		float ang = (float) i / count * 6.2831f;
		int mx = cx + (int) (std::cos(ang) * 8);
		int mz = cz + (int) (std::sin(ang) * 8);
		int my = surfaceY(b.level, mx, mz);
		if (my <= 2) continue;
		int r = 4 + b.random->nextInt(2);
		// dome of dirt with grass on top
		for (int x = -r; x <= r; ++x)
			for (int z = -r; z <= r; ++z) {
				float d = std::sqrt((float) (x * x + z * z));
				if (d > r + 0.5f) continue;
				int hh = (int) std::sqrt((float) (r * r - d * d)) * 3 / 4;
				for (int yy = 0; yy <= hh; ++yy) b.set(mx + x, my + yy, mz + z, yy == hh ? Tile::grass : Tile::dirt);
			}
		// chamber
		b.brickRoom(mx - 2, my - 1, mz - 2, mx + 2, my + 2, mz + 2);
		b.fill(mx - 2, my - 1, mz - 2, mx + 2, my - 1, mz + 2, Tile::mossStone);
		// entrance facing the centre
		int dx = (mx < cx) ? 1 : (mx > cx ? -1 : 0);
		int dz = (mz < cz) ? 1 : (mz > cz ? -1 : 0);
		if (dx == 0 && dz == 0) dx = 1;
		for (int k = 2; k <= r + 1; ++k) { b.clear(mx + dx * k, my, mz + dz * k, mx + dx * k, my + 1, mz + dz * k); }
		b.chest(mx - dx, my, mz - dz, i == bossIndex);
		b.torch(mx + (dz ? 1 : 0), my, mz + (dx ? 1 : 0));
		b.guard(i == bossIndex ? MobTypes::Zombie : (b.random->nextInt(2) ? MobTypes::Zombie : MobTypes::Skeleton), mx, my, mz, i == bossIndex ? 6 : 1);
		b.webs(mx - 1, my, mz - 1, mx + 1, my + 1, mz + 1, 10);
	}
	// standing stones in the middle
	for (int i = 0; i < 6; ++i) {
		float ang = (float) i / 6 * 6.2831f;
		int sx = cx + (int) (std::cos(ang) * 3), sz = cz + (int) (std::sin(ang) * 3);
		b.pillar(sx, surfaceY(b.level, sx, sz), surfaceY(b.level, sx, sz) + 2, sz, Tile::mossStone);
	}
	b.set(cx, surfaceY(b.level, cx, cz), cz, Tile::lightGem);
	b.guard(MobTypes::Skeleton, cx + 1, surfaceY(b.level, cx + 1, cz), cz, 3);
	return true;
}

// 10. Sunken vault: underwater-looking flooded chamber under a lake-like basin (sandstone)
static bool buildDesertVault(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 7, cz - 7, cx + 7, cz + 7, 7);
	if (y < 0) return false;
	// stepped pyramid of sandstone
	int layers = 7;
	for (int i = 0; i < layers; ++i) {
		int r = layers - i;
		b.fill(cx - r, y - 1 + i, cz - r, cx + r, y - 1 + i, cz + r, Tile::sandStone);
	}
	b.set(cx, y - 1 + layers, cz, Tile::lightGem);
	// entrance tunnel from -z at ground level
	b.clear(cx, y, cz - layers, cx, y + 1, cz - 1);
	// inner chamber under the pyramid
	b.room(cx - 5, y - 8, cz - 5, cx + 5, y - 1, cz + 5, Tile::sandStone);
	b.clear(cx - 4, y - 7, cz - 4, cx + 4, y - 2, cz + 4);
	// pit from the entrance into the chamber, with a ladder
	b.clear(cx, y - 7, cz - 1, cx, y, cz - 1);
	b.ladder(cx, y - 7, y - 1, cz - 2, 2);
	// pillars & traps (lava pool in the middle under a glass floor)
	b.pillar(cx - 3, y - 7, y - 2, cz - 3, Tile::sandStone);
	b.pillar(cx + 3, y - 7, y - 2, cz - 3, Tile::sandStone);
	b.pillar(cx - 3, y - 7, y - 2, cz + 3, Tile::sandStone);
	b.pillar(cx + 3, y - 7, y - 2, cz + 3, Tile::sandStone);
	b.fill(cx - 1, y - 9, cz - 1, cx + 1, y - 9, cz + 1, Tile::calmLava);
	b.fill(cx - 1, y - 8, cz - 1, cx + 1, y - 8, cz + 1, Tile::glass);
	b.glowstone(cx, y - 2, cz);
	// treasure alcoves
	b.chest(cx + 4, y - 7, cz + 4, true);
	b.chest(cx - 4, y - 7, cz + 4);
	b.chest(cx + 4, y - 7, cz - 4);
	b.chest(cx - 4, y - 7, cz - 4);
	b.guard(MobTypes::Zombie, cx, y - 7, cz + 2, 6);
	b.guard(MobTypes::Zombie, cx - 2, y - 7, cz, 3);
	b.guard(MobTypes::Zombie, cx + 2, y - 7, cz, 3);
	b.guard(MobTypes::Skeleton, cx, y - 7, cz - 3, 3);
	b.guard(MobTypes::Skeleton, cx + 2, y, cz - layers - 1, 1);
	return true;
}

// 11. Bandit camp: palisade with tents, a watch platform and a loot wagon
static bool buildBanditCamp(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 9, cz - 9, cx + 9, cz + 9, 7);
	if (y < 0) return false;
	int R = 9;
	// clear the camp
	b.clear(cx - R, y, cz - R, cx + R, y + 6, cz + R);
	b.fill(cx - R, y - 5, cz - R, cx + R, y - 2, cz + R, Tile::dirt);
	b.fill(cx - R, y - 1, cz - R, cx + R, y - 1, cz + R, Tile::grass);
	// palisade of fences with a wooden gate on -z
	for (int x = cx - R; x <= cx + R; ++x) { b.pillar(x, y, y + 2, cz - R, Tile::fence); b.pillar(x, y, y + 2, cz + R, Tile::fence); }
	for (int z = cz - R; z <= cz + R; ++z) { b.pillar(cx - R, y, y + 2, z, Tile::fence); b.pillar(cx + R, y, y + 2, z, Tile::fence); }
	b.clear(cx - 1, y, cz - R, cx + 1, y + 2, cz - R);
	b.pillar(cx - 2, y, y + 3, cz - R, Tile::wood); b.pillar(cx + 2, y, y + 3, cz - R, Tile::wood);
	b.fill(cx - 2, y + 3, cz - R, cx + 2, y + 3, cz - R, Tile::wood);
	// campfire in the middle
	b.set(cx, y - 1, cz, Tile::netherrack);
	b.fill(cx - 1, y - 1, cz - 1, cx + 1, y - 1, cz + 1, Tile::stoneBrick);
	b.set(cx, y - 1, cz, Tile::netherrack);
	b.set(cx, y, cz, Tile::torch, 5);
	// tents (wool A-frames)
	int tents[4][2] = { {cx - 6, cz - 5}, {cx + 6, cz - 5}, {cx - 6, cz + 5}, {cx + 6, cz + 5} };
	for (int i = 0; i < 4; ++i) {
		int tx = tents[i][0], tz = tents[i][1];
		int color = 14; // red wool
		if (i & 1) color = 15; // black
		b.fill(tx - 2, y, tz - 2, tx - 2, y, tz + 2, Tile::cloth, color);
		b.fill(tx + 2, y, tz - 2, tx + 2, y, tz + 2, Tile::cloth, color);
		b.fill(tx - 1, y + 1, tz - 2, tx - 1, y + 1, tz + 2, Tile::cloth, color);
		b.fill(tx + 1, y + 1, tz - 2, tx + 1, y + 1, tz + 2, Tile::cloth, color);
		b.fill(tx, y + 2, tz - 2, tx, y + 2, tz + 2, Tile::cloth, color);
		b.bed(tx, y, tz);
		if (i < 2) b.chest(tx, y, tz - 1);
		b.guard(MobTypes::Zombie, tx, y, tz, 1);
	}
	// watch platform
	b.pillar(cx + 7, y, y + 5, cz, Tile::wood);
	b.fill(cx + 6, y + 6, cz - 1, cx + 8, y + 6, cz + 1, Tile::wood);
	b.ladder(cx + 7, y, y + 5, cz + 1, 2);
	b.air(cx + 7, y + 6, cz + 1);
	b.guard(MobTypes::Skeleton, cx + 7, y + 7, cz, 3);
	b.guard(MobTypes::Skeleton, cx - 7, y, cz, 2);
	// loot wagon (chests boxed in with wood & fence wheels)
	b.fill(cx - 7, y, cz - 1, cx - 4, y, cz + 1, Tile::wood);
	b.set(cx - 7, y, cz - 2, Tile::fence); b.set(cx - 4, y, cz - 2, Tile::fence);
	b.set(cx - 7, y, cz + 2, Tile::fence); b.set(cx - 4, y, cz + 2, Tile::fence);
	b.chest(cx - 6, y + 1, cz, true);
	b.chest(cx - 5, y + 1, cz);
	b.guard(MobTypes::Zombie, cx - 5, y, cz + 3, 5); // bandit leader
	b.guard(MobTypes::Spider, cx + 3, y, cz + 3, 1);
	return true;
}

// 12. Nether outpost (overworld version): netherbrick keep with lava moat
static bool buildInfernalKeep(Builder& b, int cx, int cz) {
	int y = footprintY(b.level, cx - 10, cz - 10, cx + 10, cz + 10, 8);
	if (y < 0) return false;
	int R = 7;
	// base platform and moat
	b.fill(cx - R - 3, y - 6, cz - R - 3, cx + R + 3, y - 1, cz + R + 3, Tile::netherBrick);
	b.clear(cx - R - 3, y, cz - R - 3, cx + R + 3, y + 10, cz + R + 3);
	for (int x = cx - R - 2; x <= cx + R + 2; ++x)
		for (int z = cz - R - 2; z <= cz + R + 2; ++z)
			if (x <= cx - R - 1 || x >= cx + R + 1 || z <= cz - R - 1 || z >= cz + R + 1)
				b.set(x, y - 1, z, Tile::calmLava);
	// bridge over the moat on -z
	b.fill(cx - 1, y - 1, cz - R - 3, cx + 1, y - 1, cz - R, Tile::netherBrick);
	// keep
	b.room(cx - R, y - 1, cz - R, cx + R, y + 6, cz + R, Tile::netherBrick);
	b.clear(cx - 1, y, cz - R, cx + 1, y + 2, cz - R);
	for (int x = cx - R; x <= cx + R; x += 2) { b.set(x, y + 7, cz - R, Tile::netherBrick); b.set(x, y + 7, cz + R, Tile::netherBrick); }
	for (int z = cz - R; z <= cz + R; z += 2) { b.set(cx - R, y + 7, z, Tile::netherBrick); b.set(cx + R, y + 7, z, Tile::netherBrick); }
	// inner pillars & lava channels
	b.pillar(cx - 3, y, y + 5, cz - 3, Tile::netherBrick);
	b.pillar(cx + 3, y, y + 5, cz - 3, Tile::netherBrick);
	b.pillar(cx - 3, y, y + 5, cz + 3, Tile::netherBrick);
	b.pillar(cx + 3, y, y + 5, cz + 3, Tile::netherBrick);
	b.fill(cx - 5, y - 1, cz, cx + 5, y - 1, cz, Tile::calmLava);
	b.set(cx, y - 1, cz, Tile::netherBrick);
	b.fill(cx - 1, y - 1, cz - 5, cx + 1, y - 1, cz + 5, Tile::netherBrick); // walkway
	b.glowstone(cx, y + 5, cz);
	b.glowstone(cx - 5, y + 5, cz - 5); b.glowstone(cx + 5, y + 5, cz + 5);
	// throne and treasure at the back
	b.fill(cx - 1, y, cz + 5, cx + 1, y, cz + 5, Tile::stairs_netherBricks, 2);
	b.set(cx, y + 1, cz + 6, Tile::obsidian);
	b.chest(cx - 5, y, cz + 5, true);
	b.chest(cx + 5, y, cz + 5, true);
	b.chest(cx - 5, y, cz - 5);
	b.chest(cx + 5, y, cz - 5);
	// upper battlements via ladder
	b.ladder(cx + R - 1, y, y + 6, cz + R - 1, 2);
	b.air(cx + R - 1, y + 6, cz + R - 1);
	b.guard(MobTypes::Skeleton, cx - 4, y + 7, cz - 4, 4);
	b.guard(MobTypes::Skeleton, cx + 4, y + 7, cz + 4, 4);
	// garrison
	b.guard(MobTypes::PigZombie, cx, y, cz + 4, 10); // warlord
	b.guard(MobTypes::PigZombie, cx - 2, y, cz + 2, 5);
	b.guard(MobTypes::PigZombie, cx + 2, y, cz + 2, 5);
	b.guard(MobTypes::Zombie, cx - 4, y, cz - 2, 3);
	b.guard(MobTypes::Zombie, cx + 4, y, cz - 2, 3);
	b.guard(MobTypes::Skeleton, cx, y, cz - 4, 3);
	return true;
}

// ----------------------------------------------------------------------
// Placement
// ----------------------------------------------------------------------
typedef bool (*BuildFn)(Builder&, int, int);

struct DungeonType {
	const char* name;
	BuildFn build;
	int radius;      // footprint radius, used for spacing
	int levelBonus;  // added to the distance-based mob level
};

static const DungeonType DUNGEON_TYPES[] = {
	{ "Watchtower",     buildWatchtower,   5,  0 },
	{ "Crypt",          buildCrypt,        8,  2 },
	{ "Ruined Temple",  buildRuinedTemple, 10, 3 },
	{ "Spider Nest",    buildSpiderNest,   9,  2 },
	{ "Fortress",       buildFortress,     14, 6 },
	{ "Mineshaft",      buildMineshaft,    8,  1 },
	{ "Arena",          buildArena,        12, 5 },
	{ "Wizard's Spire", buildWizardSpire,  5,  5 },
	{ "Barrows",        buildBarrows,      13, 2 },
	{ "Desert Vault",   buildDesertVault,  9,  4 },
	{ "Bandit Camp",    buildBanditCamp,   10, 1 },
	{ "Infernal Keep",  buildInfernalKeep, 12, 9 },
};
static const int DUNGEON_TYPE_COUNT = sizeof(DUNGEON_TYPES) / sizeof(DUNGEON_TYPES[0]);

void generate(Level* level) {
	if (!level || level->isClientSide) return;
	if (!Rpg::isEnabled(level)) return;
	if (level->dimension && level->dimension->id != Dimension::NORMAL) return;
	if (level->getLevelData()->getGameType() != GameType::RPG) return;

	Random random(level->getSeed() ^ 0x5D0A6E0F);
	Pos spawn = level->getSharedSpawnPos();

	int target = MIN_DUNGEONS + random.nextInt(MAX_DUNGEONS - MIN_DUNGEONS + 1);
	std::vector<Pos> placed;
	int built = 0;
	int attempts = 0;

	// Shuffle the type order so every one of the 12 layouts appears at least once
	// before repeating.
	std::vector<int> order;
	for (int i = 0; i < DUNGEON_TYPE_COUNT; ++i) order.push_back(i);
	for (int i = DUNGEON_TYPE_COUNT - 1; i > 0; --i) std::swap(order[i], order[random.nextInt(i + 1)]);

	const int margin = 20;
	const bool wasGenerating = level->isGeneratingTerrain;
	level->isGeneratingTerrain = true;
	bool savedNoNeighbor = level->noNeighborUpdate;
	level->noNeighborUpdate = true;

	int typeCursor = 0;      // index into `order`
	int typeFailures = 0;    // consecutive failed attempts for the current type
	while (built < target && attempts < 3000) {
		++attempts;
		if (typeFailures >= 60) {
			// this layout can't find a site in this world; move on to the next one
			typeCursor++;
			typeFailures = 0;
		}
		const DungeonType& type = DUNGEON_TYPES[order[typeCursor % DUNGEON_TYPE_COUNT]];

		int cx = margin + random.nextInt(LEVEL_WIDTH - 2 * margin);
		int cz = margin + random.nextInt(LEVEL_DEPTH - 2 * margin);

		// keep away from spawn and from other dungeons
		int sdx = cx - spawn.x, sdz = cz - spawn.z;
		if (sdx * sdx + sdz * sdz < 40 * 40) { typeFailures++; continue; }
		bool tooClose = false;
		for (unsigned int i = 0; i < placed.size(); ++i) {
			int dx = cx - placed[i].x, dz = cz - placed[i].z;
			int minDist = type.radius + placed[i].y + 14; // placed[i].y stores that dungeon's radius
			if (dx * dx + dz * dz < minDist * minDist) { tooClose = true; break; }
		}
		if (tooClose) { typeFailures++; continue; }

		float dist = std::sqrt((float) (sdx * sdx + sdz * sdz));
		int mobLevel = 3 + (int) (dist / 14.0f) + type.levelBonus;
		if (mobLevel > Rpg::MAX_MOB_LEVEL - 10) mobLevel = Rpg::MAX_MOB_LEVEL - 10;

		Builder b(level, &random, mobLevel);
		if (!type.build(b, cx, cz)) { typeFailures++; continue; }

		// fill chests (tile entities are created lazily by getTileEntity)
		for (unsigned int i = 0; i < b.chests.size(); ++i)
			Rpg::fillLootChest(level, b.chests[i].x, b.chests[i].y, b.chests[i].z, &random, 1);
		for (unsigned int i = 0; i < b.bossChests.size(); ++i)
			Rpg::fillLootChest(level, b.bossChests[i].x, b.bossChests[i].y, b.bossChests[i].z, &random, 2);

		placed.push_back(Pos(cx, type.radius, cz));
		LOGI("RPG dungeon '%s' (mob level %d) at %d, %d\n", type.name, mobLevel, cx, cz);
		built++;
		typeCursor++;
		typeFailures = 0;
	}

	level->noNeighborUpdate = savedNoNeighbor;
	level->isGeneratingTerrain = wasGenerating;

	// settle the lighting changed by the new structures
	for (int i = 0; i < 100000 && level->updateLights(); ++i)
		;
	LOGI("RPG: placed %d dungeons (%d attempts)\n", built, attempts);
}

} // namespace RpgDungeons
