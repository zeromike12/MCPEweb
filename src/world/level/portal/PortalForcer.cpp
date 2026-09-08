#include "PortalForcer.h"
#include "../Level.h"
#include "../tile/Tile.h"
#include "../dimension/Dimension.h"

bool PortalForcer::findPortal(Level* level, int startX, int startY, int startZ, int radius, int& outX, int& outY, int& outZ) {
	if (!level || !Tile::portalTile) return false;

	int portId = Tile::portalTile->id;
	double bestDist = 9999999.0;
	int bestX = 0, bestY = 0, bestZ = 0;
	bool found = false;

	for (int dx = -radius; dx <= radius; dx += 2) {
		for (int dz = -radius; dz <= radius; dz += 2) {
			int cx = startX + dx;
			int cz = startZ + dz;
			for (int cy = 10; cy < 120; cy++) {
				if (level->getTile(cx, cy, cz) == portId) {
					int by = cy;
					while (by > 1 && level->getTile(cx, by - 1, cz) == portId) {
						by--;
					}
					double dist = (double)(dx * dx + dz * dz + (by - startY) * (by - startY));
					if (dist < bestDist) {
						bestDist = dist;
						bestX = cx;
						bestY = by;
						bestZ = cz;
						found = true;
					}
				}
			}
		}
	}

	if (found) {
		outX = bestX;
		outY = bestY;
		outZ = bestZ;
		return true;
	}
	return false;
}

bool PortalForcer::createPortal(Level* level, int x, int z, int targetDim, int& outX, int& outY, int& outZ) {
	if (!level || !Tile::obsidian || !Tile::portalTile) return false;

	int obsId = Tile::obsidian->id;
	int portId = Tile::portalTile->id;
	int baseY = 64;

	if (targetDim == Dimension::NETHER) {
		bool found = false;
		for (int testY = 40; testY < 90; testY++) {
			if (level->getTile(x, testY - 1, z) == Tile::netherrack->id &&
				level->isEmptyTile(x, testY, z) &&
				level->isEmptyTile(x, testY + 1, z) &&
				level->isEmptyTile(x, testY + 2, z)) {
				baseY = testY;
				found = true;
				break;
			}
		}
		if (!found) {
			baseY = 50;
		}
	} else {
		int topY = level->getTopSolidBlock(x, z);
		if (topY >= 10 && topY <= 115) {
			baseY = topY + 1;
		} else {
			baseY = 64;
		}
	}

	// Build obsidian portal frame along X-axis (width 4, height 5).
	// Suppress neighbour updates so the partially built portal doesn't tear
	// itself down (see PortalTile::neighborChanged).
	bool oldNoUpdate = level->noNeighborUpdate;
	level->noNeighborUpdate = true;
	for (int ix = 0; ix < 4; ix++) {
		for (int iy = -1; iy < 4; iy++) {
			for (int iz = -1; iz <= 1; iz++) {
				int bx = x + ix;
				int by = baseY + iy;
				int bz = z + iz;

				if (iz == 0) {
					if (iy == -1 || iy == 3 || ix == 0 || ix == 3) {
						level->setTile(bx, by, bz, obsId);
					} else {
						level->setTileAndData(bx, by, bz, portId, 1);
					}
				} else {
					if (iy >= 0 && iy <= 2) {
						level->setTile(bx, by, bz, 0);
					} else if (iy == -1) {
						int current = level->getTile(bx, by, bz);
						if (current == 0 || current == Tile::calmLava->id || current == Tile::lava->id) {
							level->setTile(bx, by, bz, (targetDim == Dimension::NETHER) ? Tile::netherrack->id : Tile::stoneBrick->id);
						}
					}
				}
			}
		}
	}

	level->noNeighborUpdate = oldNoUpdate;

	outX = x + 1;
	outY = baseY;
	outZ = z;
	return true;
}

bool PortalForcer::findOrCreatePortal(Level* level, int startX, int startZ, int targetDim, float& outX, float& outY, float& outZ) {
	int px = 0, py = 0, pz = 0;
	int searchY = (targetDim == Dimension::NETHER) ? 50 : 64;

	if (findPortal(level, startX, searchY, startZ, 128, px, py, pz)) {
		outX = (float)px + 0.5f;
		outY = (float)py;
		outZ = (float)pz + 0.5f;
		return true;
	}

	if (createPortal(level, startX, startZ, targetDim, px, py, pz)) {
		outX = (float)px + 0.5f;
		outY = (float)py;
		outZ = (float)pz + 0.5f;
		return true;
	}

	return false;
}
