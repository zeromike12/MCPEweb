#ifndef NET_MINECRAFT_WORLD_ITEM__FlintAndSteelItem_H__
#define NET_MINECRAFT_WORLD_ITEM__FlintAndSteelItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include "ItemInstance.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"
#include "../level/tile/FireTile.h"
#include "../level/tile/PortalTile.h"
#include "../entity/player/Player.h"

class FlintAndSteelItem: public Item
{
    typedef Item super;
public:
    FlintAndSteelItem(int id)
    :   super(id)
    {
		setMaxStackSize(1);
		setMaxDamage(64);
    }

    virtual bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
        if (face == 0) y--;
        if (face == 1) y++;
        if (face == 2) z--;
        if (face == 3) z++;
        if (face == 4) x--;
        if (face == 5) x++;

        int targetType = level->getTile(x, y, z);
        int fireId = Tile::fire ? Tile::fire->id : 51;

        if (targetType == 0 || targetType == fireId) {
            level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.click", 1.0f, level->random.nextFloat() * 0.4f + 0.8f);

            // Attempt to activate a Nether Portal frame
            if (!PortalTile::trySpawnPortal(level, x, y, z)) {
                level->setTile(x, y, z, fireId);
            }
        }

        if (instance != NULL) {
            instance->hurt(1);
        }
        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__FlintAndSteelItem_H__*/
