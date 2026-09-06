#ifndef NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__
#define NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include "../entity/player/Player.h"
#include "../entity/Mob.h"
#include "../entity/MobFactory.h"
#include "../entity/EntityTypes.h"
#include "../level/Level.h"
#include "ItemInstance.h"

class SpawnEggItem: public Item {
    typedef Item super;
public:
    SpawnEggItem(int id)
    :   super(id)
    {
        maxStackSize = 64;
        setStackedByData(true);
    }

    virtual Item* setIcon(int icon) {
        this->icon = icon;
        return this;
    }

    virtual Item* setIcon(int column, int row) {
        icon = column + row * ICON_COLUMNS;
        return this;
    }

    virtual bool useOn(ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
        if (!level->isClientSide) {
            int mobType = itemInstance->getAuxValue();
            Mob* mob = MobFactory::CreateMob(mobType, level);
            if (mob) {
                mob->setPos((float)x + 0.5f, (float)y + 1.0f, (float)z + 0.5f);
                level->addEntity(mob);
                
                // Play spawn sound
                level->playSound(player, "mob.chicken.plop", 0.5f, 1.0f);
                
                if (!player->abilities.instabuild) {
                    itemInstance->count--;
                    if (itemInstance->count <= 0) {
                        return true; // Item consumed
                    }
                }
            }
        }
        return false;
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__*/
