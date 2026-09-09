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

// Flint and Steel
//
// Right-click a block face to strike a spark on the adjacent air block.
// If that block is the inside of a complete obsidian frame (2 wide x 3 tall
// interior, 4x5 outer, either axis) the frame is lit and filled with portal
// blocks; otherwise a fire block is placed (which also lights portals through
// FireTile::onPlace when placed at the base of a frame).
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
        // Offset to the block adjacent to the clicked face
        if (face == 0) y--;
        if (face == 1) y++;
        if (face == 2) z--;
        if (face == 3) z++;
        if (face == 4) x--;
        if (face == 5) x++;

        if (y < 0 || y >= Level::DEPTH) return false;

        int targetType = level->getTile(x, y, z);
        int fireId = Tile::fire ? ((Tile*)Tile::fire)->id : 51;

        // Only strike in air (or re-light existing fire)
        if (targetType != 0 && targetType != fireId) {
            return false;
        }

        level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.click", 1.0f, level->random.nextFloat() * 0.4f + 0.8f);

        // 1. Try to activate a Nether portal frame around the struck block
        bool lit = PortalTile::trySpawnPortal(level, x, y, z);

        // 2. Otherwise place fire, which also works as a portal igniter via
        //    FireTile::onPlace on the server side.
        if (!lit) {
            level->setTile(x, y, z, fireId);
            lit = true;
        }

        // Wear the tool in survival; the creative game mode restores aux/count
        // after useOn so this is a no-op there.
        if (instance != NULL) {
            instance->hurt(1);
        }
        return lit;
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__FlintAndSteelItem_H__*/
