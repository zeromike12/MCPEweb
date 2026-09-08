#include "Rpg.h"

#include <cmath>
#include <sstream>
#include <algorithm>

#include "../level/Level.h"
#include "../level/LevelSettings.h"
#include "../level/storage/LevelData.h"
#include "../level/tile/Tile.h"
#include "../level/tile/entity/ChestTileEntity.h"
#include "../level/material/Material.h"
#include "../entity/Mob.h"
#include "../entity/EntityTypes.h"
#include "../entity/player/Player.h"
#include "../item/Item.h"
#include "../item/ItemInstance.h"
#include "../item/WeaponItem.h"
#include "../item/ArmorItem.h"
#include "../item/BowItem.h"
#include "../../util/Random.h"
#include "../../util/Mth.h"
#include "../../locale/I18n.h"

namespace Rpg {

// ----------------------------------------------------------------------
// Mode
// ----------------------------------------------------------------------
bool isEnabled(const Level* level) {
	if (!level) return false;
	return const_cast<Level*>(level)->getLevelData()->getGameType() == GameType::RPG;
}

// ----------------------------------------------------------------------
// Player levels
// ----------------------------------------------------------------------
int xpToNextLevel(int level) {
	if (level < 1) level = 1;
	// 10, 17, 26, 37, 50, ... roughly quadratic
	return 10 + (level - 1) * 5 + (level - 1) * (level - 1) * 2;
}

int xpForKill(int mobLevel, bool hostile) {
	int base = hostile ? 4 : 1;
	return base + mobLevel * (hostile ? 2 : 1);
}

float playerDamageMultiplier(int playerLevel) {
	if (playerLevel < 1) playerLevel = 1;
	// +6% per level: level 10 = 1.54x, level 50 = 3.94x
	return 1.0f + (playerLevel - 1) * 0.06f;
}

float playerDefenseMultiplier(int playerLevel) {
	if (playerLevel < 1) playerLevel = 1;
	// -1.2% per level, floored at 45% (level 50 = 0.412 -> clamped 0.45)
	float m = 1.0f - (playerLevel - 1) * 0.012f;
	return m < 0.45f ? 0.45f : m;
}

int playerMaxHealth(int playerLevel) {
	if (playerLevel < 1) playerLevel = 1;
	// +1 heart every 2 levels, level 50 = 20 + 48 = 68 (34 hearts)
	return 20 + ((playerLevel - 1) / 2) * 2;
}

// ----------------------------------------------------------------------
// Mob levels
// ----------------------------------------------------------------------
int rollMobLevel(Level* level, float x, float z, Random* random) {
	Pos spawn = level->getSharedSpawnPos();
	float dx = x - spawn.x;
	float dz = z - spawn.z;
	float dist = std::sqrt(dx * dx + dz * dz);

	// One level per ~12 blocks from spawn, plus a little noise.
	int base = 1 + (int) (dist / 12.0f);

	// Nether is always dangerous
	if (level->dimension && level->dimension->id == Dimension::NETHER)
		base += 10;

	// Time played also slowly raises the baseline (one level per 10 days)
	long dayTicks = 24000;
	base += (int) (level->getTime() / (dayTicks * 10));

	int variance = random->nextInt(5) - 2;
	int result = base + variance;

	// Rare "elite" mobs
	if (random->nextInt(25) == 0) result += 3 + random->nextInt(5);

	if (result < 1) result = 1;
	if (result > MAX_MOB_LEVEL) result = MAX_MOB_LEVEL;
	return result;
}

int mobMaxHealth(int baseHealth, int mobLevel) {
	if (mobLevel < 1) mobLevel = 1;
	// +15% per level
	int hp = (int) std::ceil(baseHealth * (1.0f + (mobLevel - 1) * 0.15f));
	if (hp < 1) hp = 1;
	return hp;
}

int mobDamage(int baseDamage, int mobLevel) {
	if (mobLevel < 1) mobLevel = 1;
	if (baseDamage <= 0) return baseDamage;
	// +8% per level
	int dmg = (int) std::floor(baseDamage * (1.0f + (mobLevel - 1) * 0.08f) + 0.5f);
	if (dmg < 1) dmg = 1;
	return dmg;
}

static const char* mobTypeKey(int type) {
	switch (type) {
	case MobTypes::Chicken:   return "entity.Chicken.name";
	case MobTypes::Cow:       return "entity.Cow.name";
	case MobTypes::Pig:       return "entity.Pig.name";
	case MobTypes::Sheep:     return "entity.Sheep.name";
	case MobTypes::Zombie:    return "entity.Zombie.name";
	case MobTypes::Creeper:   return "entity.Creeper.name";
	case MobTypes::Skeleton:  return "entity.Skeleton.name";
	case MobTypes::Spider:    return "entity.Spider.name";
	case MobTypes::PigZombie: return "entity.PigZombie.name";
	}
	return NULL;
}

std::string mobLabel(Mob* mob) {
	std::stringstream ss;
	int lvl = mob->getRpgLevel();
	const char* key = mobTypeKey(mob->getEntityTypeId());
	bool hostile = mob->getCreatureBaseType() == MobTypes::BaseEnemy;

	// colour by relative threat
	const char* color = "\xa7" "a"; // green
	if (hostile) {
		if (lvl >= 20)      color = "\xa7" "d"; // light purple - elite
		else if (lvl >= 10) color = "\xa7" "c"; // red
		else if (lvl >= 5)  color = "\xa7" "6"; // gold
		else                color = "\xa7" "e"; // yellow
	}

	ss << color << "Lv." << lvl << " ";
	if (key) ss << I18n::get(key);
	// Mob health is only authoritative on the server / in single player
	if (mob->level && !mob->level->isClientSide)
		ss << " \xa7" "7" << mob->health << "/" << mob->getScaledMaxHealth();
	return ss.str();
}

// ----------------------------------------------------------------------
// Equipment modifiers
// ----------------------------------------------------------------------
int makeModifier(int rarity, int kind) {
	if (kind == Mod::None) return 0;
	return ((rarity & 0xf) << 4) | (kind & 0xf);
}

int modifierRarity(int modifier) {
	return (modifier >> 4) & 0xf;
}

int modifierKind(int modifier) {
	return modifier & 0xf;
}

static bool isWeapon(const ItemInstance& item) {
	Item* it = item.getItem();
	if (!it) return false;
	return dynamic_cast<WeaponItem*>(it) != NULL || dynamic_cast<BowItem*>(it) != NULL;
}

static bool isArmor(const ItemInstance& item) {
	return ItemInstance::isArmorItem(&item);
}

static int rollWeaponKind(Random* random, int rarity) {
	// Higher rarities unlock the fancier kinds
	static const int common[]    = { Mod::Sharp, Mod::Swift };
	static const int uncommon[]  = { Mod::Sharp, Mod::Heavy, Mod::Swift };
	static const int rare[]      = { Mod::Heavy, Mod::Vampiric, Mod::Flaming, Mod::Slaying };
	static const int legendary[] = { Mod::Vampiric, Mod::Flaming, Mod::Slaying, Mod::Heavy };
	switch (rarity) {
	case Rarity::Common:    return common[random->nextInt(2)];
	case Rarity::Uncommon:  return uncommon[random->nextInt(3)];
	case Rarity::Rare:      return rare[random->nextInt(4)];
	case Rarity::Legendary: return legendary[random->nextInt(4)];
	}
	return Mod::None;
}

static int rollArmorKind(Random* random, int rarity) {
	static const int common[]    = { Mod::Sturdy, Mod::Vital };
	static const int uncommon[]  = { Mod::Sturdy, Mod::Vital, Mod::Fortified };
	static const int rare[]      = { Mod::Fortified, Mod::Thorned, Mod::Warding, Mod::Vital };
	static const int legendary[] = { Mod::Blessed, Mod::Thorned, Mod::Warding, Mod::Fortified };
	switch (rarity) {
	case Rarity::Common:    return common[random->nextInt(2)];
	case Rarity::Uncommon:  return uncommon[random->nextInt(3)];
	case Rarity::Rare:      return rare[random->nextInt(4)];
	case Rarity::Legendary: return legendary[random->nextInt(4)];
	}
	return Mod::None;
}

int rollCraftedModifier(const ItemInstance& item, Random* random) {
	bool weapon = isWeapon(item);
	bool armor = !weapon && isArmor(item);
	if (!weapon && !armor) return 0;

	// Crafted gear: 55% common, 30% uncommon, 15% rare. Never legendary.
	int roll = random->nextInt(100);
	int rarity = Rarity::Common;
	if (roll >= 85) rarity = Rarity::Rare;
	else if (roll >= 55) rarity = Rarity::Uncommon;

	int kind = weapon ? rollWeaponKind(random, rarity) : rollArmorKind(random, rarity);
	return makeModifier(rarity, kind);
}

int rollLootModifier(const ItemInstance& item, Random* random, bool forceLegendary) {
	bool weapon = isWeapon(item);
	bool armor = !weapon && isArmor(item);
	if (!weapon && !armor) return 0;

	// Loot: 20% uncommon, 50% rare, 30% legendary (12% overall of chest gear ends up legendary
	// after the caller's own roll).
	int rarity;
	if (forceLegendary) {
		rarity = Rarity::Legendary;
	} else {
		int roll = random->nextInt(100);
		if (roll >= 70) rarity = Rarity::Legendary;
		else if (roll >= 20) rarity = Rarity::Rare;
		else rarity = Rarity::Uncommon;
	}

	int kind = weapon ? rollWeaponKind(random, rarity) : rollArmorKind(random, rarity);
	return makeModifier(rarity, kind);
}

std::string modifierPrefix(int modifier) {
	switch (modifierKind(modifier)) {
	case Mod::Sharp:     return "Sharp";
	case Mod::Heavy:     return "Heavy";
	case Mod::Vampiric:  return "Vampiric";
	case Mod::Flaming:   return "Flaming";
	case Mod::Swift:     return "Swift";
	case Mod::Slaying:   return "Slaying";
	case Mod::Sturdy:    return "Sturdy";
	case Mod::Fortified: return "Fortified";
	case Mod::Vital:     return "Vital";
	case Mod::Thorned:   return "Thorned";
	case Mod::Warding:   return "Warding";
	case Mod::Blessed:   return "Blessed";
	}
	return "";
}

std::string modifierColorCode(int modifier) {
	switch (modifierRarity(modifier)) {
	case Rarity::Common:    return "\xa7" "f"; // white
	case Rarity::Uncommon:  return "\xa7" "a"; // green
	case Rarity::Rare:      return "\xa7" "9"; // blue
	case Rarity::Legendary: return "\xa7" "6"; // gold
	}
	return "\xa7" "f";
}

static std::string rarityName(int modifier) {
	switch (modifierRarity(modifier)) {
	case Rarity::Common:    return "Common";
	case Rarity::Uncommon:  return "Uncommon";
	case Rarity::Rare:      return "Rare";
	case Rarity::Legendary: return "Legendary";
	}
	return "";
}

// Rarity scales the strength of every modifier: 1x, 1.5x, 2x, 3x
static float rarityScale(int modifier) {
	switch (modifierRarity(modifier)) {
	case Rarity::Common:    return 1.0f;
	case Rarity::Uncommon:  return 1.5f;
	case Rarity::Rare:      return 2.0f;
	case Rarity::Legendary: return 3.0f;
	}
	return 0;
}

std::string describeModifier(int modifier) {
	if (modifier == 0) return "";
	std::stringstream ss;
	ss << rarityName(modifier) << ": ";
	float s = rarityScale(modifier);
	switch (modifierKind(modifier)) {
	case Mod::Sharp:     ss << "+" << (int) (1 * s) << " damage"; break;
	case Mod::Heavy:     ss << "+" << (int) (2 * s) << " damage"; break;
	case Mod::Vampiric:  ss << "+" << (int) (1 * s) << " damage, heals on hit"; break;
	case Mod::Flaming:   ss << "+" << (int) (1 * s) << " damage, burns target"; break;
	case Mod::Swift:     ss << "+" << (int) (1 * s) << " damage, never breaks"; break;
	case Mod::Slaying:   ss << "+" << (int) (3 * s) << " damage vs. monsters"; break;
	case Mod::Sturdy:    ss << "+" << (int) (1 * s) << " armor"; break;
	case Mod::Fortified: ss << "+" << (int) (2 * s) << " armor"; break;
	case Mod::Vital:     ss << "+" << (int) (2 * s) << " max health"; break;
	case Mod::Thorned:   ss << "reflects " << (int) (1 * s) << " damage"; break;
	case Mod::Warding:   ss << "-" << (int) (5 * s) << "% damage taken"; break;
	case Mod::Blessed:   ss << "+" << (int) (1 * s) << " armor, +" << (int) (2 * s) << " max health"; break;
	}
	return ss.str();
}

int weaponBonusDamage(int modifier, int baseDamage, bool targetHostile) {
	float s = rarityScale(modifier);
	switch (modifierKind(modifier)) {
	case Mod::Sharp:    return (int) (1 * s);
	case Mod::Heavy:    return (int) (2 * s);
	case Mod::Vampiric: return (int) (1 * s);
	case Mod::Flaming:  return (int) (1 * s);
	case Mod::Swift:    return (int) (1 * s);
	case Mod::Slaying:  return targetHostile ? (int) (3 * s) : 0;
	}
	return 0;
}

int armorBonusDefense(int modifier) {
	float s = rarityScale(modifier);
	switch (modifierKind(modifier)) {
	case Mod::Sturdy:    return (int) (1 * s);
	case Mod::Fortified: return (int) (2 * s);
	case Mod::Blessed:   return (int) (1 * s);
	}
	return 0;
}

int armorBonusHealth(int modifier) {
	float s = rarityScale(modifier);
	switch (modifierKind(modifier)) {
	case Mod::Vital:   return (int) (2 * s);
	case Mod::Blessed: return (int) (2 * s);
	}
	return 0;
}

float armorDamageReduction(int modifier) {
	if (modifierKind(modifier) == Mod::Warding) return 0.05f * rarityScale(modifier);
	return 0;
}

int armorThornsDamage(int modifier) {
	if (modifierKind(modifier) == Mod::Thorned) return (int) (1 * rarityScale(modifier));
	return 0;
}

bool weaponNoDurabilityLoss(int modifier) {
	return modifierKind(modifier) == Mod::Swift;
}

int weaponFireSeconds(int modifier) {
	if (modifierKind(modifier) == Mod::Flaming) return (int) (2 * rarityScale(modifier));
	return 0;
}

int weaponLifeSteal(int modifier, int damageDealt) {
	if (modifierKind(modifier) != Mod::Vampiric) return 0;
	int heal = (int) std::ceil(damageDealt * 0.15f * rarityScale(modifier));
	return heal < 1 ? 1 : heal;
}

void applyWeaponOnHit(Player* attacker, Entity* target, ItemInstance* weapon, int damageDealt) {
	if (!attacker || !target || !weapon || !weapon->hasModifier()) return;
	int mod = weapon->getModifier();

	int fire = weaponFireSeconds(mod);
	if (fire > 0 && !target->isFireImmune()) {
		int ticks = fire * 20;
		if (target->onFire < ticks) target->onFire = ticks;
	}

	int heal = weaponLifeSteal(mod, damageDealt);
	if (heal > 0) attacker->heal(heal);
}

int playerArmorBonusDefense(Player* player) {
	int total = 0;
	for (int i = 0; i < 4; ++i) {
		const ItemInstance* a = player->getArmor(i);
		if (a && !a->isNull()) total += armorBonusDefense(a->getModifier());
	}
	return total;
}

int playerArmorBonusHealth(Player* player) {
	int total = 0;
	for (int i = 0; i < 4; ++i) {
		const ItemInstance* a = player->getArmor(i);
		if (a && !a->isNull()) total += armorBonusHealth(a->getModifier());
	}
	return total;
}

float playerArmorDamageReduction(Player* player) {
	float total = 0;
	for (int i = 0; i < 4; ++i) {
		const ItemInstance* a = player->getArmor(i);
		if (a && !a->isNull()) total += armorDamageReduction(a->getModifier());
	}
	return total > 0.6f ? 0.6f : total;
}

int playerArmorThorns(Player* player) {
	int total = 0;
	for (int i = 0; i < 4; ++i) {
		const ItemInstance* a = player->getArmor(i);
		if (a && !a->isNull()) total += armorThornsDamage(a->getModifier());
	}
	return total;
}

// ----------------------------------------------------------------------
// Loot chests
// ----------------------------------------------------------------------
static Item* pickWeapon(Random* random, int tierRoll) {
	// tierRoll 0..99 : higher = better material
	if (random->nextInt(6) == 0) return Item::bow;
	if (tierRoll >= 92) return Item::sword_emerald;
	if (tierRoll >= 70) return Item::sword_iron;
	if (tierRoll >= 55) return Item::sword_gold;
	if (tierRoll >= 25) return Item::sword_stone;
	return Item::sword_wood;
}

static Item* pickArmor(Random* random, int tierRoll) {
	int slot = random->nextInt(4);
	Item* const cloth[]   = { Item::helmet_cloth,   Item::chestplate_cloth,   Item::leggings_cloth,   Item::boots_cloth };
	Item* const chain[]   = { Item::helmet_chain,   Item::chestplate_chain,   Item::leggings_chain,   Item::boots_chain };
	Item* const gold[]    = { Item::helmet_gold,    Item::chestplate_gold,    Item::leggings_gold,    Item::boots_gold };
	Item* const iron[]    = { Item::helmet_iron,    Item::chestplate_iron,    Item::leggings_iron,    Item::boots_iron };
	Item* const diamond[] = { Item::helmet_diamond, Item::chestplate_diamond, Item::leggings_diamond, Item::boots_diamond };
	if (tierRoll >= 92) return diamond[slot];
	if (tierRoll >= 70) return iron[slot];
	if (tierRoll >= 55) return chain[slot];
	if (tierRoll >= 30) return gold[slot];
	return cloth[slot];
}

void fillLootChest(Level* level, int x, int y, int z, Random* random) {
	ChestTileEntity* chest = dynamic_cast<ChestTileEntity*>(level->getTileEntity(x, y, z));
	if (!chest) return;

	const int size = chest->getContainerSize();

	// Distance from spawn nudges the material tier upwards
	Pos spawn = level->getSharedSpawnPos();
	float dx = (float) x - spawn.x;
	float dz = (float) z - spawn.z;
	int distBonus = (int) (std::sqrt(dx * dx + dz * dz) / 10.0f);
	if (distBonus > 40) distBonus = 40;

	// 2-4 pieces of gear, at least one weapon and one armor piece
	int gearCount = 2 + random->nextInt(3);
	for (int i = 0; i < gearCount; ++i) {
		int tierRoll = random->nextInt(100) + distBonus;
		if (tierRoll > 99) tierRoll = 99;

		bool weapon = (i == 0) ? true : (i == 1 ? false : random->nextInt(2) == 0);
		Item* item = weapon ? pickWeapon(random, tierRoll) : pickArmor(random, tierRoll);
		if (!item) continue;

		ItemInstance inst(item, 1, 0);
		// Every piece of chest gear is modified; ~12% are legendary (rollLootModifier
		// is 30% legendary, and we only ask for a modifier 40% of the time otherwise).
		bool forceLegendary = random->nextInt(100) < 8;
		inst.setModifier(rollLootModifier(inst, random, forceLegendary));

		int slot = random->nextInt(size);
		for (int tries = 0; tries < size; ++tries) {
			ItemInstance* existing = chest->getItem(slot);
			if (!existing || existing->isNull()) break;
			slot = (slot + 1) % size;
		}
		chest->setItem(slot, &inst);
	}

	// Some consumables
	int extraCount = 1 + random->nextInt(3);
	for (int i = 0; i < extraCount; ++i) {
		ItemInstance inst;
		switch (random->nextInt(6)) {
		case 0: inst = ItemInstance(Item::arrow, 4 + random->nextInt(12), 0); break;
		case 1: inst = ItemInstance(Item::bread, 1 + random->nextInt(3), 0); break;
		case 2: inst = ItemInstance(Item::apple, 1 + random->nextInt(3), 0); break;
		case 3: inst = ItemInstance(Item::ironIngot, 1 + random->nextInt(4), 0); break;
		case 4: inst = ItemInstance(Tile::torch, 4 + random->nextInt(8), 0); break;
		case 5: inst = ItemInstance(Item::emerald, 1 + random->nextInt(2), 0); break;
		}
		int slot = random->nextInt(size);
		for (int tries = 0; tries < size; ++tries) {
			ItemInstance* existing = chest->getItem(slot);
			if (!existing || existing->isNull()) break;
			slot = (slot + 1) % size;
		}
		chest->setItem(slot, &inst);
	}
}

void placeLootChest(Level* level, int xo, int zo, Random* random) {
	// Roughly one chest per 12 chunks.
	if (random->nextInt(12) != 0) return;

	for (int attempt = 0; attempt < 8; ++attempt) {
		int x = xo + random->nextInt(16) + 8;
		int z = zo + random->nextInt(16) + 8;

		int y;
		if (random->nextInt(3) == 0) {
			// Surface chest
			y = level->getTopSolidBlock(x, z);
			if (y <= 1 || y >= 126) continue;
		} else {
			// Cave chest: find an air pocket underground with a solid floor
			y = 8 + random->nextInt(50);
			bool found = false;
			for (int scan = 0; scan < 20 && y > 5; ++scan, --y) {
				if (level->isEmptyTile(x, y, z) && level->isSolidBlockingTile(x, y - 1, z)) {
					found = true;
					break;
				}
			}
			if (!found) continue;
		}

		if (!level->isEmptyTile(x, y, z)) continue;
		if (!level->isSolidBlockingTile(x, y - 1, z)) continue;
		if (level->getMaterial(x, y - 1, z) == Material::water || level->getMaterial(x, y - 1, z) == Material::lava) continue;

		level->setTile(x, y, z, Tile::chest->id);
		fillLootChest(level, x, y, z, random);
		return;
	}
}

} // namespace Rpg
