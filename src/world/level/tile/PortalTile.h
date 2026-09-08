#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__PortalTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__PortalTile_H__

//package net.minecraft.world.level.tile;

#include "Tile.h"
#include "../material/Material.h"
#include "../Level.h"
#include "../LevelSource.h"
#include "../../entity/Entity.h"
#include "../../../util/Random.h"

class PortalTile: public Tile
{
	typedef Tile super;
public:
	PortalTile(int id, int tex);

	virtual AABB* getAABB(Level* level, int x, int y, int z);
	virtual bool isSolidRender();
	virtual bool isCubeShaped();
	virtual bool blocksLight();
	virtual int getRenderLayer();
	virtual int getRenderShape();
	virtual int getResourceCount(Random* random);
	virtual bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face);
	virtual void updateShape(LevelSource* level, int x, int y, int z);
	virtual void neighborChanged(Level* level, int x, int y, int z, int type);
	virtual void entityInside(Level* level, int x, int y, int z, Entity* entity);
	virtual void animateTick(Level* level, int x, int y, int z, Random* random);

	static bool trySpawnPortal(Level* level, int x, int y, int z);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__PortalTile_H__*/
