#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__WoodTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__WoodTile_H__

#include "Tile.h"
#include "../material/Material.h"
#include "../LevelSource.h"

class WoodTile: public Tile
{
	typedef Tile super;
public:
	static const int WOOD_COLORS[16];

	WoodTile(int id, int tex)
	:	super(id, tex, Material::wood)
	{
	}

	/*@Override*/
	int getColor(int data) {
		return WOOD_COLORS[data & 0xf];
	}

	/*@Override*/
	int getColor(LevelSource* level, int x, int y, int z) {
		return getColor(level->getData(x, y, z));
	}

	static int getColorByData(int data) {
		return WOOD_COLORS[data & 0xf];
	}

protected:
	/*@Override*/
	int getSpawnResourcesAuxValue(int data) {
		return data;
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__WoodTile_H__*/
