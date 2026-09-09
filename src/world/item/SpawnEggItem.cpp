#include "SpawnEggItem.h"
#include "../entity/EntityTypes.h"
#include "../entity/MobFactory.h"
#include "../entity/Mob.h"
#include "../entity/animal/Animal.h"
#include "../entity/animal/Sheep.h"
#include "../entity/player/Player.h"
#include "../Facing.h"
#include "../level/Level.h"
#include "../../SharedConstants.h"

SpawnEggItem::SpawnEggItem(int id)
:   super(id)
{
    setStackedByData(true);
    setMaxDamage(0);
}

int SpawnEggItem::getIcon(int itemAuxValue)
{
    switch (itemAuxValue) {
        case MobTypes::Chicken:   return 13 * ICON_COLUMNS + 0;
        case MobTypes::Cow:       return 13 * ICON_COLUMNS + 1;
        case MobTypes::Pig:       return 13 * ICON_COLUMNS + 2;
        case MobTypes::Sheep:     return 13 * ICON_COLUMNS + 3;
        case MobTypes::Zombie:    return 13 * ICON_COLUMNS + 4;
        case MobTypes::Creeper:   return 13 * ICON_COLUMNS + 5;
        case MobTypes::Skeleton:  return 13 * ICON_COLUMNS + 6;
        case MobTypes::Spider:    return 13 * ICON_COLUMNS + 7;
        case MobTypes::PigZombie: return 13 * ICON_COLUMNS + 8;
        case MobTypes::Moa:       return 13 * ICON_COLUMNS + 0;
        case MobTypes::Sentry:    return 13 * ICON_COLUMNS + 4;
        case MobTypes::Mimic:     return 13 * ICON_COLUMNS + 4;
        default:                  return 13 * ICON_COLUMNS + 0;
    }
}

std::string SpawnEggItem::getDescriptionId(const ItemInstance* itemInstance) const
{
    int mobType = itemInstance ? itemInstance->getAuxValue() : 0;
    std::string mobName;
    switch (mobType) {
        case MobTypes::Chicken:   mobName = "chicken"; break;
        case MobTypes::Cow:       mobName = "cow"; break;
        case MobTypes::Pig:       mobName = "pig"; break;
        case MobTypes::Sheep:     mobName = "sheep"; break;
        case MobTypes::Zombie:    mobName = "zombie"; break;
        case MobTypes::Creeper:   mobName = "creeper"; break;
        case MobTypes::Skeleton:  mobName = "skeleton"; break;
        case MobTypes::Spider:    mobName = "spider"; break;
        case MobTypes::PigZombie: mobName = "pigZombie"; break;
        case MobTypes::Moa:       mobName = "moa"; break;
        case MobTypes::Sentry:    mobName = "sentry"; break;
        case MobTypes::Mimic:     mobName = "mimic"; break;
        default:                  mobName = "generic"; break;
    }
    return super::getDescriptionId() + "." + mobName;
}

bool SpawnEggItem::useOn(ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ)
{
    switch (face) {
        case Facing::DOWN:  y--; break;
        case Facing::UP:    y++; break;
        case Facing::NORTH: z--; break;
        case Facing::SOUTH: z++; break;
        case Facing::WEST:  x--; break;
        case Facing::EAST:  x++; break;
    }

    float xx = (float)x + 0.5f;
    float yy = (float)y;
    float zz = (float)z + 0.5f;

    int mobType = itemInstance ? itemInstance->getAuxValue() : 0;
    Mob* mob = MobFactory::CreateMob(mobType, level);
    if (mob != NULL) {
        mob->moveTo(xx, yy, zz, level->random.nextFloat() * 360.0f, 0.0f);
        if (!level->isClientSide) {
            level->addEntity(mob);
        }
        if (mob->getEntityTypeId() == MobTypes::Sheep) {
            ((Sheep*)mob)->setColor(Sheep::getSheepColor(&level->random));
        }
        if (mob->getEntityTypeId() == MobTypes::Moa) {
            // Moa eggs hatched in the Incubator yield tame, rideable Moas
            ((Moa*)mob)->setTame(true);
        }
        if (player && !player->abilities.instabuild) {
            itemInstance->count--;
        }
        return true;
    }
    return false;
}

void SpawnEggItem::interactEnemy(ItemInstance* itemInstance, Mob* mob)
{
    if (!mob || !itemInstance) return;
    int mobType = itemInstance->getAuxValue();
    if (mob->getEntityTypeId() == mobType) {
        Level* level = mob->level;
        if (!level) return;
        Mob* baby = MobFactory::CreateMob(mobType, level);
        if (baby) {
            if (MobTypes::BaseCreature == baby->getCreatureBaseType()) {
                ((Animal*)baby)->setAge(-20 * 60 * SharedConstants::TicksPerSecond);
            }
            baby->moveTo(mob->x, mob->y, mob->z, level->random.nextFloat() * 360.0f, 0.0f);
            if (!level->isClientSide) {
                level->addEntity(baby);
            }
            if (baby->getEntityTypeId() == MobTypes::Sheep) {
                ((Sheep*)baby)->setColor(((Sheep*)mob)->getColor());
            }
            if (mob->level && !mob->level->isClientSide) {
                itemInstance->count--;
            }
        }
    }
}
