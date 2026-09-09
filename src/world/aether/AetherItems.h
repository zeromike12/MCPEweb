#ifndef NET_MINECRAFT_WORLD_AETHER__AetherItems_H__
#define NET_MINECRAFT_WORLD_AETHER__AetherItems_H__

//
// Item classes for the Aether plus the (previously missing) bucket.
//

#include "../item/Item.h"
#include "../item/FoodItem.h"
#include "../item/DoorItem.h"
#include "../item/SignItem.h"
#include "../item/BedItem.h"
#include "../item/PickaxeItem.h"
#include "../item/HatchetItem.h"
#include "../item/ShovelItem.h"
#include "../item/WeaponItem.h"
#include "../item/ItemInstance.h"

class Level;
class Player;
class Tile;

// Bucket: empty (tileId 0), water or lava. Water buckets also light Aether
// portals when used on a glowstone frame.
class BucketItem : public Item {
	typedef Item super;
public:
	BucketItem(int id, int tileId);
	ItemInstance* use(ItemInstance* instance, Level* level, Player* player);
	bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ);
private:
	bool emptyBucket(ItemInstance* instance, Level* level, Player* player, int x, int y, int z);
	int tileId;
};

// Ambrosium shard: right-click to heal 1 heart (also fuel for the altar).
class AmbrosiumShardItem : public Item {
	typedef Item super;
public:
	AmbrosiumShardItem(int id);
	ItemInstance* use(ItemInstance* instance, Level* level, Player* player);
};

// Zanite tools: stats improve as durability drops.
class ZanitePickaxeItem : public PickaxeItem {
	typedef PickaxeItem super;
public:
	ZanitePickaxeItem(int id, const Tier& tier);
	float getDestroySpeed(ItemInstance* itemInstance, Tile* tile);
};
class ZaniteAxeItem : public HatchetItem {
	typedef HatchetItem super;
public:
	ZaniteAxeItem(int id, const Tier& tier);
	float getDestroySpeed(ItemInstance* itemInstance, Tile* tile);
};
class ZaniteShovelItem : public ShovelItem {
	typedef ShovelItem super;
public:
	ZaniteShovelItem(int id, const Tier& tier);
	float getDestroySpeed(ItemInstance* itemInstance, Tile* tile);
};

// Skyroot tools use the wood tier; double drops are handled by the tiles.
class SkyrootPickaxeItem : public PickaxeItem {
	typedef PickaxeItem super;
public:
	SkyrootPickaxeItem(int id);
	float getDestroySpeed(ItemInstance* itemInstance, Tile* tile);
};
class SkyrootAxeItem : public HatchetItem {
	typedef HatchetItem super;
public:
	SkyrootAxeItem(int id);
};
class SkyrootShovelItem : public ShovelItem {
	typedef ShovelItem super;
public:
	SkyrootShovelItem(int id);
	float getDestroySpeed(ItemInstance* itemInstance, Tile* tile);
};

// Gravitite tools: diamond tier; the pickaxe lifts blocks it right-clicks.
class GravititePickaxeItem : public PickaxeItem {
	typedef PickaxeItem super;
public:
	GravititePickaxeItem(int id);
	float getDestroySpeed(ItemInstance* itemInstance, Tile* tile);
	bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ);
};
class GravititeAxeItem : public HatchetItem {
	typedef HatchetItem super;
public:
	GravititeAxeItem(int id);
};
class GravititeShovelItem : public ShovelItem {
	typedef ShovelItem super;
public:
	GravititeShovelItem(int id);
	float getDestroySpeed(ItemInstance* itemInstance, Tile* tile);
};

// Door / bed / sign items that place the skyroot variants.
class SkyrootDoorItem : public Item {
	typedef Item super;
public:
	SkyrootDoorItem(int id);
	bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ);
};

class SkyrootBedItem : public Item {
	typedef Item super;
public:
	SkyrootBedItem(int id);
	bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ);
};

class AetherSignItem : public Item {
	typedef Item super;
public:
	AetherSignItem(int id);
	bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ);
};

// Dungeon keys: plain items, consumed by locked doors.
class DungeonKeyItem : public Item {
	typedef Item super;
public:
	DungeonKeyItem(int id, int tier);
	int getTier() const { return tier; }
private:
	int tier;
};

// Moa egg: place it in an Incubator; throwing does nothing special.
class MoaEggItem : public Item {
	typedef Item super;
public:
	MoaEggItem(int id);
};

// Cold Parachute: made from cloudwool. Hold it (selected) while falling and
// you drift down slowly without fall damage; each use wears it a little.
class ColdParachuteItem : public Item {
	typedef Item super;
public:
	ColdParachuteItem(int id);
	bool isHandEquipped() { return false; }
};

#endif /*NET_MINECRAFT_WORLD_AETHER__AetherItems_H__*/
