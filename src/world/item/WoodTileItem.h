#ifndef NET_MINECRAFT_WORLD_ITEM__WoodTileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__WoodTileItem_H__

#include "TileItem.h"
#include "DyePowderItem.h"
#include "../level/tile/ClothTile.h"
#include "../level/tile/WoodTile.h"

class WoodTileItem: public TileItem
{
	typedef TileItem super;
public:
	WoodTileItem(int id)
	:	super(id)
	{
		setMaxDamage(0);
		setStackedByData(true);
	}

	/*@Override*/
	int getLevelDataForAuxValue(int auxValue) {
		return auxValue;
	}

	/*@Override*/
	int getIcon(int itemAuxValue) {
		return Tile::wood->getTexture(2, itemAuxValue);
	}

	/*@Override*/
	std::string getDescriptionId(const ItemInstance* instance) const {
		int aux = instance ? instance->getAuxValue() : 0;
		if (aux == 0) {
			return super::getDescriptionId();
		}
		int colorIdx = ClothTile::getTileDataForItemAuxValue(aux);
		if (colorIdx >= 0 && colorIdx < 16) {
			return super::getDescriptionId() + "." + DyePowderItem::COLOR_DESCS[colorIdx];
		}
		return super::getDescriptionId();
	}
};

#endif /*NET_MINECRAFT_WORLD_ITEM__WoodTileItem_H__*/
