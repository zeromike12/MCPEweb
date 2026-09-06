#ifndef NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__
#define NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__

#include "Item.h"
#include "../Facing.h"
#include "../entity/MobFactory.h"
#include "../entity/player/Player.h"
#include "../level/Level.h"
#include "../level/MobSpawner.h"

// A spawn egg creates the mob associated with the item when it is used on a
// block.  The item is deliberately data-driven by mob type so every mob that
// MobFactory can create can be exposed to Creative mode in the same way.
class SpawnEggItem: public Item
{
    typedef Item super;
public:
    SpawnEggItem(int id, int mobType)
    :   super(id),
        mobType(mobType)
    {
        maxStackSize = 64;
    }

    int getMobType() const {
        return mobType;
    }

    bool useOn(ItemInstance* itemInstance, Player* player, Level* level,
               int x, int y, int z, int face, float clickX, float clickY,
               float clickZ)
    {
        (void)clickX;
        (void)clickY;
        (void)clickZ;

        if (!itemInstance || itemInstance->count <= 0 || !player || !level)
            return false;
        if (face < Facing::DOWN || face > Facing::EAST)
            return false;

        // Spawn on the face that was clicked, just like a block placement.
        const int spawnX = x + Facing::STEP_X[face];
        const int spawnY = y + Facing::STEP_Y[face];
        const int spawnZ = z + Facing::STEP_Z[face];
        if (spawnY < 0 || spawnY >= Level::DEPTH)
            return false;

        Mob* mob = MobFactory::CreateMob(mobType, level);
        if (!mob)
            return false;

        bool spawned = true;
        if (level->isClientSide) {
            // The server owns entity creation. The client still reports the
            // interaction as handled and receives the mob through AddMob.
            delete mob;
        } else {
            spawned = MobSpawner::addMob(level, mob,
                                         (float)spawnX + 0.5f,
                                         (float)spawnY,
                                         (float)spawnZ + 0.5f,
                                         level->random.nextFloat() * 360.0f,
                                         0.0f, true);
            if (!spawned)
                delete mob;
        }

        if (spawned && !player->abilities.instabuild)
            --itemInstance->count;
        return spawned;
    }

private:
    int mobType;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__*/
