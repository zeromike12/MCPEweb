#ifndef NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__
#define NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include <string>

class ItemInstance;
class Level;
class Mob;
class Player;

class SpawnEggItem: public Item
{
    typedef Item super;
public:
    SpawnEggItem(int id);

    virtual int getIcon(int itemAuxValue);
    virtual std::string getDescriptionId(const ItemInstance* itemInstance) const;
    virtual bool useOn(ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ);
    virtual void interactEnemy(ItemInstance* itemInstance, Mob* mob);
};

#endif /*NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__*/
