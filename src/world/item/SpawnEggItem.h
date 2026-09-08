#ifndef NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__
#define NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__

#include "Item.h"
#include "ItemInstance.h"
#include "../entity/player/Player.h"
#include "../entity/MobFactory.h"
#include "../entity/Mob.h"
#include "../level/Level.h"
#include "../../locale/I18n.h"
#include "../Facing.h"

class SpawnEggItem : public Item
{
	typedef Item super;
public:
	using super::getDescriptionId;

	SpawnEggItem(int id)
	:	super(id)
	{
		setStackedByData(true);
		setMaxDamage(0);
	}

	virtual int getIcon(int auxValue) override {
		// Use the egg icon for all spawn eggs
		return icon;
	}

	virtual std::string getDescriptionId(const ItemInstance* itemInstance) const override {
		int mobType = itemInstance->getAuxValue();
		std::string entityKey = getEntityNameKey(mobType);
		// Return a key like "item.monsterPlacer.Chicken" so the lang file can resolve it
		return super::getDescriptionId() + "." + entityKey;
	}

	virtual std::string getName() const override {
		return I18n::get(getDescriptionId() + ".name");
	}

	virtual bool useOn(ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) override {
		if (!level->isClientSide) {
			int mobType = itemInstance->getAuxValue();

			// Calculate spawn position based on the face clicked
			float spawnX = (float)x + 0.5f;
			float spawnY = (float)y + 1.0f;
			float spawnZ = (float)z + 0.5f;

			if (face == Facing::UP) {
				spawnY = (float)y + 1.0f;
			} else if (face == Facing::DOWN) {
				spawnY = (float)y - 1.0f;
			} else if (face == Facing::NORTH) {
				spawnZ = (float)z - 1.0f;
			} else if (face == Facing::SOUTH) {
				spawnZ = (float)z + 1.0f;
			} else if (face == Facing::WEST) {
				spawnX = (float)x - 1.0f;
			} else if (face == Facing::EAST) {
				spawnX = (float)x + 1.0f;
			}

			Mob* mob = MobFactory::CreateMob(mobType, level);
			if (mob) {
				mob->moveTo(spawnX, spawnY, spawnZ, player->yRot, 0.0f);
				level->addEntity(mob);

				if (!player->abilities.instabuild) {
					itemInstance->count--;
				}
			}
		}
		return true;
	}

private:
	static std::string getEntityNameKey(int mobType) {
		switch (mobType) {
			case MobTypes::Chicken:  return "Chicken";
			case MobTypes::Cow:      return "Cow";
			case MobTypes::Pig:      return "Pig";
			case MobTypes::Sheep:    return "Sheep";
			case MobTypes::Zombie:   return "Zombie";
			case MobTypes::Creeper:  return "Creeper";
			case MobTypes::Skeleton: return "Skeleton";
			case MobTypes::Spider:   return "Spider";
			case MobTypes::PigZombie:return "PigZombie";
			default:                 return "Unknown";
		}
	}
};

#endif /*NET_MINECRAFT_WORLD_ITEM__SpawnEggItem_H__*/
