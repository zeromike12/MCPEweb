#include "AetherFeatures.h"
#include "Aether.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"
#include "../level/tile/LeafTile.h"
#include "../../util/Random.h"
#include <cstdlib>

static bool isAetherSoil(int t) {
	return t == Aether::aetherGrass->id || t == Aether::enchantedGrass->id || t == Aether::aetherDirt->id;
}

AetherTreeFeature::AetherTreeFeature(bool doUpdate, bool golden)
:	super(doUpdate), golden(golden)
{
}

bool AetherTreeFeature::place(Level* level, Random* random, int x, int y, int z) {
	int treeHeight = random->nextInt(3) + (golden ? 6 : 4);
	if (y < 1 || y + treeHeight + 1 >= Level::DEPTH) return false;

	int trunkId = golden ? Aether::goldenOakLog->id : Aether::skyrootLog->id;
	int leafId = golden ? Aether::goldenOakLeaves->id : Aether::skyrootLeaves->id;

	// Room check
	for (int yy = y; yy <= y + 1 + treeHeight; yy++) {
		int r = 1;
		if (yy == y) r = 0;
		if (yy >= y + 1 + treeHeight - 2) r = 2;
		for (int xx = x - r; xx <= x + r; xx++) {
			for (int zz = z - r; zz <= z + r; zz++) {
				int tt = level->getTile(xx, yy, zz);
				if (tt != 0 && tt != leafId && tt != Aether::skyrootLeaves->id && tt != Aether::goldenOakLeaves->id) return false;
			}
		}
	}
	if (!isAetherSoil(level->getTile(x, y - 1, z))) return false;

	placeBlock(level, x, y - 1, z, Aether::aetherDirt->id);

	if (golden) {
		// Golden oaks: tall trunk with a wide, round canopy
		int top = y + treeHeight;
		for (int yy = top - 3; yy <= top + 1; yy++) {
			int yo = yy - top;
			int r = (yo <= -2) ? 3 : (yo == -1 ? 3 : (yo == 0 ? 2 : 1));
			for (int xx = x - r; xx <= x + r; xx++) {
				for (int zz = z - r; zz <= z + r; zz++) {
					int dx = std::abs(xx - x), dz = std::abs(zz - z);
					if (dx == r && dz == r && (random->nextInt(2) == 0 || r >= 2)) continue;
					if (dx * dx + dz * dz > r * r + 1) continue;
					int t = level->getTile(xx, yy, zz);
					if (t == 0 || !Tile::solid[t]) placeBlock(level, xx, yy, zz, leafId);
				}
			}
		}
		for (int hh = 0; hh < treeHeight; hh++) {
			int t = level->getTile(x, y + hh, z);
			if (t == 0 || t == leafId) placeBlock(level, x, y + hh, z, trunkId);
		}
		// a few branches
		for (int b = 0; b < 3; ++b) {
			int bx = x + random->nextInt(3) - 1, bz = z + random->nextInt(3) - 1, by = y + treeHeight - 2 - random->nextInt(2);
			if ((bx != x || bz != z) && (level->getTile(bx, by, bz) == 0 || level->getTile(bx, by, bz) == leafId))
				placeBlock(level, bx, by, bz, trunkId);
		}
		return true;
	}

	// Skyroot: classic oak-ish shape
	for (int yy = y - 3 + treeHeight; yy <= y + treeHeight; yy++) {
		int yo = yy - (y + treeHeight);
		int offs = 1 - yo / 2;
		for (int xx = x - offs; xx <= x + offs; xx++) {
			int xo = xx - x;
			for (int zz = z - offs; zz <= z + offs; zz++) {
				int zo = zz - z;
				if (std::abs(xo) == offs && std::abs(zo) == offs && (random->nextInt(2) == 0 || yo == 0)) continue;
				int t = level->getTile(xx, yy, zz);
				if (t == 0 || !Tile::solid[t]) placeBlock(level, xx, yy, zz, leafId);
			}
		}
	}
	for (int hh = 0; hh < treeHeight; hh++) {
		int t = level->getTile(x, y + hh, z);
		if (t == 0 || t == leafId) placeBlock(level, x, y + hh, z, trunkId);
	}
	return true;
}

AetherPlantPatchFeature::AetherPlantPatchFeature(int tileId, int count)
:	super(false), tileId(tileId), count(count)
{
}

bool AetherPlantPatchFeature::place(Level* level, Random* random, int x, int y, int z) {
	bool any = false;
	for (int i = 0; i < count; i++) {
		int xx = x + random->nextInt(8) - random->nextInt(8);
		int zz = z + random->nextInt(8) - random->nextInt(8);
		// Islands float over the void, so locate the grass surface of this column directly
		int yy = -1;
		for (int sy = 120; sy > 32; sy--) {
			int below = level->getTile(xx, sy - 1, zz);
			if (below == Aether::aetherGrass->id || below == Aether::enchantedGrass->id) { yy = sy; break; }
		}
		if (yy < 1 || !level->isEmptyTile(xx, yy, zz)) continue;
		level->setTileNoUpdate(xx, yy, zz, tileId);
		any = true;
	}
	return any;
}
