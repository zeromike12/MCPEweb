#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__HellSandTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__HellSandTile_H__

//package net.minecraft.world.level.tile;

#include "Tile.h"
#include "../material/Material.h"
#include "../Level.h"
#include "../../entity/Entity.h"
#include "../../phys/AABB.h"

class HellSandTile: public Tile
{
	typedef Tile super;
public:
	HellSandTile(int id, int tex)
	:	super(id, tex, Material::sand)
	{
	}

	virtual AABB* getAABB(Level* level, int x, int y, int z) {
		float depth = 2.0f / 16.0f;
		return tmpBB.set(x, y, z, x + 1, (float)y + 1.0f - depth, z + 1);
	}

	virtual void entityInside(Level* level, int x, int y, int z, Entity* entity) {
		if (entity != NULL) {
			entity->xd *= 0.4f;
			entity->zd *= 0.4f;
		}
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__HellSandTile_H__*/
