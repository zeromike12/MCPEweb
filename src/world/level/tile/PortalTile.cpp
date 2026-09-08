#include "PortalTile.h"
#include "ObsidianTile.h"
#include "FireTile.h"
#include "../../entity/Entity.h"
#include "../../entity/EntityTypes.h"
#include "../../Facing.h"

PortalTile::PortalTile(int id, int tex)
:	super(id, tex, Material::portal)
{
	setLightEmission(11.0f / 16.0f);
	setDestroyTime(-1.0f);
	setExplodeable(0.0f);
}

AABB* PortalTile::getAABB(Level* level, int x, int y, int z) {
	return NULL;
}

bool PortalTile::isSolidRender() {
	return false;
}

bool PortalTile::isCubeShaped() {
	return false;
}

bool PortalTile::blocksLight() {
	return false;
}

int PortalTile::getRenderLayer() {
	return Tile::RENDERLAYER_ALPHATEST;
}

int PortalTile::getRenderShape() {
	return Tile::SHAPE_BLOCK;
}

int PortalTile::getResourceCount(Random* random) {
	return 0;
}

bool PortalTile::shouldRenderFace(LevelSource* level, int x, int y, int z, int face) {
	if (level->getTile(x, y, z) == id) return false;
	return super::shouldRenderFace(level, x, y, z, face);
}

void PortalTile::updateShape(LevelSource* level, int x, int y, int z) {
	int data = level->getData(x, y, z);
	if (data == 2 || level->getTile(x, y, z - 1) == id || level->getTile(x, y, z + 1) == id) {
		// Z-axis aligned portal: thin along X
		setShape(0.375f, 0.0f, 0.0f, 0.625f, 1.0f, 1.0f);
	} else {
		// X-axis aligned portal: thin along Z
		setShape(0.0f, 0.0f, 0.375f, 1.0f, 1.0f, 0.625f);
	}
}

void PortalTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	int data = level->getData(x, y, z);
	bool broken = false;

	if (data == 1) { // X-aligned
		int left = level->getTile(x - 1, y, z);
		int right = level->getTile(x + 1, y, z);
		int down = level->getTile(x, y - 1, z);
		int up = level->getTile(x, y + 1, z);

		if ((left != Tile::obsidian->id && left != id) ||
			(right != Tile::obsidian->id && right != id) ||
			(down != Tile::obsidian->id && down != id) ||
			(up != Tile::obsidian->id && up != id)) {
			broken = true;
		}
	} else { // Z-aligned (data == 2 or fallback)
		int back = level->getTile(x, y, z - 1);
		int front = level->getTile(x, y, z + 1);
		int down = level->getTile(x, y - 1, z);
		int up = level->getTile(x, y + 1, z);

		if ((back != Tile::obsidian->id && back != id) ||
			(front != Tile::obsidian->id && front != id) ||
			(down != Tile::obsidian->id && down != id) ||
			(up != Tile::obsidian->id && up != id)) {
			broken = true;
		}
	}

	if (broken) {
		level->setTile(x, y, z, 0);
	}
}

void PortalTile::entityInside(Level* level, int x, int y, int z, Entity* entity) {
	if (entity != NULL) {
		entity->inPortal = true;
	}
}

void PortalTile::animateTick(Level* level, int x, int y, int z, Random* random) {
	for (int i = 0; i < 4; i++) {
		float px = (float)x + random->nextFloat();
		float py = (float)y + random->nextFloat();
		float pz = (float)z + random->nextFloat();
		float xd = (random->nextFloat() - 0.5f) * 0.5f;
		float yd = (random->nextFloat() - 0.5f) * 0.5f;
		float zd = (random->nextFloat() - 0.5f) * 0.5f;
		level->addParticle(ParticleType::portal, px, py, pz, xd, yd, zd);
	}
}

/*static*/
bool PortalTile::trySpawnPortal(Level* level, int x, int y, int z) {
	if (!level || !Tile::portalTile || !Tile::obsidian) return false;

	int obsId = Tile::obsidian->id;
	int portId = Tile::portalTile->id;
	int fireId = Tile::fire ? Tile::fire->id : -1;

	auto isPortalSpace = [&](int bx, int by, int bz) -> bool {
		int t = level->getTile(bx, by, bz);
		return (t == 0 || t == fireId || t == portId);
	};

	// 1. Try X-aligned portal (width 2 along X, height 3 along Y, constant Z)
	for (int cx = x - 1; cx <= x; cx++) {
		for (int cy = y - 2; cy <= y; cy++) {
			bool valid = true;

			// Bottom frame (cy - 1)
			if (level->getTile(cx, cy - 1, z) != obsId || level->getTile(cx + 1, cy - 1, z) != obsId) {
				valid = false;
			}
			// Top frame (cy + 3)
			if (valid && (level->getTile(cx, cy + 3, z) != obsId || level->getTile(cx + 1, cy + 3, z) != obsId)) {
				valid = false;
			}
			// Left column (cx - 1, cy..cy+2)
			for (int j = 0; valid && j < 3; j++) {
				if (level->getTile(cx - 1, cy + j, z) != obsId) valid = false;
			}
			// Right column (cx + 2, cy..cy+2)
			for (int j = 0; valid && j < 3; j++) {
				if (level->getTile(cx + 2, cy + j, z) != obsId) valid = false;
			}
			// Interior (cx..cx+1, cy..cy+2)
			for (int i = 0; valid && i < 2; i++) {
				for (int j = 0; valid && j < 3; j++) {
					if (!isPortalSpace(cx + i, cy + j, z)) valid = false;
				}
			}

			if (valid) {
				// Fill interior with portal blocks
				for (int i = 0; i < 2; i++) {
					for (int j = 0; j < 3; j++) {
						level->setTileAndData(cx + i, cy + j, z, portId, 1);
					}
				}
				return true;
			}
		}
	}

	// 2. Try Z-aligned portal (width 2 along Z, height 3 along Y, constant X)
	for (int cz = z - 1; cz <= z; cz++) {
		for (int cy = y - 2; cy <= y; cy++) {
			bool valid = true;

			// Bottom frame (cy - 1)
			if (level->getTile(x, cy - 1, cz) != obsId || level->getTile(x, cy - 1, cz + 1) != obsId) {
				valid = false;
			}
			// Top frame (cy + 3)
			if (valid && (level->getTile(x, cy + 3, cz) != obsId || level->getTile(x, cy + 3, cz + 1) != obsId)) {
				valid = false;
			}
			// Left column (cz - 1, cy..cy+2)
			for (int j = 0; valid && j < 3; j++) {
				if (level->getTile(x, cy + j, cz - 1) != obsId) valid = false;
			}
			// Right column (cz + 2, cy..cy+2)
			for (int j = 0; valid && j < 3; j++) {
				if (level->getTile(x, cy + j, cz + 2) != obsId) valid = false;
			}
			// Interior (cz..cz+1, cy..cy+2)
			for (int k = 0; valid && k < 2; k++) {
				for (int j = 0; valid && j < 3; j++) {
					if (!isPortalSpace(x, cy + j, cz + k)) valid = false;
				}
			}

			if (valid) {
				// Fill interior with portal blocks
				for (int k = 0; k < 2; k++) {
					for (int j = 0; j < 3; j++) {
						level->setTileAndData(x, cy + j, cz + k, portId, 2);
					}
				}
				return true;
			}
		}
	}

	return false;
}
