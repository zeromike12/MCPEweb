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
#include "../item/RpgGearItem.h"
#include "../phys/AABB.h"
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
	return ItemInstance::isArmorItem(&item) && dynamic_cast<RpgArmorItem*>(item.getItem()) == NULL;
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


// ----------------------------------------------------------------------
// Mythic gear: armor sets & unique weapons
// ----------------------------------------------------------------------
static RpgArmorItem* asMythicArmor(const ItemInstance* inst) {
	if (!inst || inst->isNull()) return NULL;
	return dynamic_cast<RpgArmorItem*>(inst->getItem());
}

static RpgWeaponItem* asMythicWeapon(const ItemInstance* inst) {
	if (!inst || inst->isNull()) return NULL;
	return dynamic_cast<RpgWeaponItem*>(inst->getItem());
}

int armorSetPieces(Player* player, int set) {
	if (!player || set < 0) return 0;
	int n = 0;
	for (int i = 0; i < 4; ++i) {
		RpgArmorItem* a = asMythicArmor(player->getArmor(i));
		if (a && a->set == set) n++;
	}
	return n;
}

int fullArmorSet(Player* player) {
	for (int s = 0; s < RpgGear::SET_COUNT; ++s)
		if (armorSetPieces(player, s) >= 4) return s;
	return -1;
}

std::string armorSetName(int set) {
	switch (set) {
	case RpgGear::SET_DRAGONSCALE: return "Dragonscale";
	case RpgGear::SET_SHADOWWEAVE: return "Shadowweave";
	case RpgGear::SET_TITANFORGED: return "Titanforged";
	case RpgGear::SET_LIFEBLOOM:   return "Lifebloom";
	case RpgGear::SET_STORMCALLER: return "Stormcaller";
	}
	return "";
}

std::string armorSetBonusDescription(int set, int pieces) {
	std::string two, four;
	switch (set) {
	case RpgGear::SET_DRAGONSCALE: two = "attackers burn";          four = "immune to fire and lava"; break;
	case RpgGear::SET_SHADOWWEAVE: two = "+15% speed";              four = "+35% speed, no fall damage"; break;
	case RpgGear::SET_TITANFORGED: two = "+1 armor per piece";      four = "-25% damage, no knockback"; break;
	case RpgGear::SET_LIFEBLOOM:   two = "+2 health per piece";     four = "regenerate health"; break;
	case RpgGear::SET_STORMCALLER: two = "+20% damage";             four = "lightning strikes on hit"; break;
	default: return "";
	}
	std::stringstream ss;
	ss << "\xa7" << (pieces >= 2 ? "a" : "8") << "(2) " << two << "  ";
	ss << "\xa7" << (pieces >= 4 ? "a" : "8") << "(4) " << four;
	return ss.str();
}

std::string weaponEffectDescription(const Item* item) {
	const RpgWeaponItem* w = dynamic_cast<const RpgWeaponItem*>(item);
	if (!w) {
		if (dynamic_cast<const RpgLongbowItem*>(item)) return "Arrows fly faster, hit harder and always crit";
		return "";
	}
	switch (w->effect) {
	case RpgGear::FX_CLEAVE:  return "Massive blows send enemies flying";
	case RpgGear::FX_BLAZE:   return "Sets enemies ablaze";
	case RpgGear::FX_FROST:   return "Freezes enemies in place";
	case RpgGear::FX_LEECH:   return "Heals you for half the damage dealt";
	case RpgGear::FX_THUNDER: return "Shockwave damages all nearby enemies";
	case RpgGear::FX_VENOM:   return "Poisons enemies";
	case RpgGear::FX_EXECUTE: return "Double damage to wounded enemies";
	case RpgGear::FX_SOUL:    return "Kills grant +50% XP";
	case RpgGear::FX_WIND:    return "+25% speed while held, never breaks";
	default: return "";
	}
}

std::string describeGear(Player* player, const ItemInstance* item) {
	if (!item || item->isNull()) return "";
	if (RpgArmorItem* a = asMythicArmor(item)) {
		int pieces = armorSetPieces(player, a->set);
		std::stringstream ss;
		ss << "\xa7" "d" << armorSetName(a->set) << " set (" << pieces << "/4)  " << armorSetBonusDescription(a->set, pieces);
		return ss.str();
	}
	std::string fx = weaponEffectDescription(item->getItem());
	if (!fx.empty()) {
		std::string base = "\xa7" "d" "Mythic: " + fx;
		if (item->hasModifier()) base += "  \xa7" "7" + describeModifier(item->getModifier());
		return base;
	}
	return "";
}

bool setFireImmune(Player* player) {
	return armorSetPieces(player, RpgGear::SET_DRAGONSCALE) >= 4;
}

float setSpeedMultiplier(Player* player) {
	float m = 1.0f;
	int shadow = armorSetPieces(player, RpgGear::SET_SHADOWWEAVE);
	if (shadow >= 4) m *= 1.35f;
	else if (shadow >= 2) m *= 1.15f;
	RpgWeaponItem* w = asMythicWeapon(player->getCarriedItem());
	if (w && w->effect == RpgGear::FX_WIND) m *= 1.25f;
	return m;
}

bool setNoFallDamage(Player* player) {
	return armorSetPieces(player, RpgGear::SET_SHADOWWEAVE) >= 4;
}

bool setKnockbackImmune(Player* player) {
	return armorSetPieces(player, RpgGear::SET_TITANFORGED) >= 4;
}

int setBonusDefense(Player* player) {
	int titan = armorSetPieces(player, RpgGear::SET_TITANFORGED);
	return titan >= 2 ? titan : 0;
}

int setBonusHealth(Player* player) {
	int life = armorSetPieces(player, RpgGear::SET_LIFEBLOOM);
	return life >= 2 ? life * 2 : 0;
}

float setDamageReduction(Player* player) {
	return armorSetPieces(player, RpgGear::SET_TITANFORGED) >= 4 ? 0.25f : 0.0f;
}

int setBonusDamage(Player* player, int baseDamage) {
	if (armorSetPieces(player, RpgGear::SET_STORMCALLER) >= 2)
		return (int) std::ceil(baseDamage * 0.2f);
	return 0;
}

int setThorns(Player* player) {
	return 0;
}

void tickArmorSets(Player* player) {
	if (!player || !player->level || player->level->isClientSide) return;
	if (player->health <= 0) return;
	// Lifebloom 4/4: regenerate 1 health every 2.5 seconds
	if (armorSetPieces(player, RpgGear::SET_LIFEBLOOM) >= 4 && player->tickCount % 50 == 0) {
		if (player->health < player->getMaxHealth()) player->heal(1);
	}
	// Dragonscale 4/4: never burn
	if (setFireImmune(player) && player->onFire > 0) player->onFire = 0;
}

void onPlayerHurt(Player* player, Entity* attacker) {
	if (!player || !attacker || player->level->isClientSide) return;
	if (!attacker->isMob() || attacker->isPlayer()) return;
	// Dragonscale 2/4: melee attackers catch fire
	if (armorSetPieces(player, RpgGear::SET_DRAGONSCALE) >= 2 && attacker->distanceToSqr(player) < 9.0f) {
		if (!attacker->isFireImmune() && attacker->onFire < 80) attacker->onFire = 80;
	}
}

static void strikeNearby(Player* attacker, Entity* center, int damage, float radius, bool freeze) {
	Level* level = attacker->level;
	AABB box = center->bb.grow(radius, 1.5f, radius);
	EntityList list = level->getEntities(center, box);
	for (unsigned int i = 0; i < list.size(); ++i) {
		Entity* e = list[i];
		if (!e || e == attacker || !e->isMob() || e->isPlayer()) continue;
		if (e->getCreatureBaseType() != MobTypes::BaseEnemy) continue;
		Mob* m = (Mob*) e;
		int before = m->invulnerableTime;
		m->invulnerableTime = 0;
		m->hurt(attacker, damage);
		if (m->invulnerableTime < before) m->invulnerableTime = before;
		if (freeze) m->frozenTicks = 40;
	}
}

int applyWeaponPreHit(Player* attacker, Entity* target, ItemInstance* weapon, int damage) {
	if (!attacker || !target) return damage;
	// Stormcaller set: +20% damage
	damage += setBonusDamage(attacker, damage);
	RpgWeaponItem* w = asMythicWeapon(weapon);
	if (!w || !target->isMob()) return damage;
	Mob* mob = (Mob*) target;
	switch (w->effect) {
	case RpgGear::FX_EXECUTE:
		if (mob->health * 2 <= mob->getScaledMaxHealth()) damage *= 2;
		break;
	default: break;
	}
	return damage;
}

void applyWeaponOnHit(Player* attacker, Entity* target, ItemInstance* weapon, int damageDealt) {
	if (!attacker || !target || !weapon) return;
	Level* level = attacker->level;

	// Ordinary modifier effects
	if (weapon->hasModifier()) {
		int mod = weapon->getModifier();
		int fire = weaponFireSeconds(mod);
		if (fire > 0 && !target->isFireImmune()) {
			int ticks = fire * 20;
			if (target->onFire < ticks) target->onFire = ticks;
		}
		int heal = weaponLifeSteal(mod, damageDealt);
		if (heal > 0) attacker->heal(heal);
	}

	// Stormcaller 4/4: lightning strikes nearby enemies (1 in 3 hits)
	if (armorSetPieces(attacker, RpgGear::SET_STORMCALLER) >= 4 && level->random.nextInt(3) == 0) {
		strikeNearby(attacker, target, 3 + damageDealt / 3, 3.0f, false);
		if (!target->isFireImmune() && target->onFire < 40) target->onFire = 40;
	}

	// Mythic weapon effects
	RpgWeaponItem* w = asMythicWeapon(weapon);
	if (!w || !target->isMob()) return;
	Mob* mob = (Mob*) target;
	switch (w->effect) {
	case RpgGear::FX_CLEAVE: {
		// extra knockback
		float dx = target->x - attacker->x, dz = target->z - attacker->z;
		float d = std::sqrt(dx * dx + dz * dz);
		if (d < 0.01f) d = 0.01f;
		target->xd += dx / d * 1.2f;
		target->zd += dz / d * 1.2f;
		target->yd += 0.3f;
		break;
	}
	case RpgGear::FX_BLAZE:
		if (!target->isFireImmune() && target->onFire < 120) target->onFire = 120;
		break;
	case RpgGear::FX_FROST:
		mob->frozenTicks = 60;
		break;
	case RpgGear::FX_LEECH:
		attacker->heal((damageDealt + 1) / 2);
		break;
	case RpgGear::FX_THUNDER:
		strikeNearby(attacker, target, (damageDealt + 1) / 2, 3.5f, false);
		break;
	case RpgGear::FX_VENOM:
		mob->poisonTicks = 150;
		break;
	default: break;
	}
}

float weaponXpMultiplier(Player* player) {
	if (!player) return 1.0f;
	RpgWeaponItem* w = asMythicWeapon(player->getCarriedItem());
	if (w && w->effect == RpgGear::FX_SOUL) return 1.5f;
	return 1.0f;
}

bool gearNoDurabilityLoss(const ItemInstance* item) {
	RpgWeaponItem* w = asMythicWeapon(item);
	if (w && w->effect == RpgGear::FX_WIND) return true;
	if (item && item->hasModifier() && weaponNoDurabilityLoss(item->getModifier())) return true;
	return false;
}

Item* rollMythicItem(Random* random) {
	Item* const armor[5][4] = {
		{ Item::helmet_dragonscale, Item::chestplate_dragonscale, Item::leggings_dragonscale, Item::boots_dragonscale },
		{ Item::helmet_shadowweave, Item::chestplate_shadowweave, Item::leggings_shadowweave, Item::boots_shadowweave },
		{ Item::helmet_titanforged, Item::chestplate_titanforged, Item::leggings_titanforged, Item::boots_titanforged },
		{ Item::helmet_lifebloom,   Item::chestplate_lifebloom,   Item::leggings_lifebloom,   Item::boots_lifebloom },
		{ Item::helmet_stormcaller, Item::chestplate_stormcaller, Item::leggings_stormcaller, Item::boots_stormcaller },
	};
	Item* const weapons[10] = {
		Item::greatsword_obsidian, Item::katana_blazing, Item::sword_frostbrand, Item::dagger_vampire, Item::hammer_thunder,
		Item::fang_venom, Item::axe_executioner, Item::scythe_soulreaper, Item::blade_wind, Item::longbow_ranger,
	};
	if (random->nextInt(2) == 0) return weapons[random->nextInt(10)];
	return armor[random->nextInt(5)][random->nextInt(4)];
}

int playerArmorBonusDefense(Player* player) {
	int total = setBonusDefense(player);
	for (int i = 0; i < 4; ++i) {
		const ItemInstance* a = player->getArmor(i);
		if (a && !a->isNull()) total += armorBonusDefense(a->getModifier());
	}
	return total;
}

int playerArmorBonusHealth(Player* player) {
	int total = setBonusHealth(player);
	for (int i = 0; i < 4; ++i) {
		const ItemInstance* a = player->getArmor(i);
		if (a && !a->isNull()) total += armorBonusHealth(a->getModifier());
	}
	return total;
}

float playerArmorDamageReduction(Player* player) {
	float total = setDamageReduction(player);
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

void fillLootChest(Level* level, int x, int y, int z, Random* random, int quality) {
	ChestTileEntity* chest = dynamic_cast<ChestTileEntity*>(level->getTileEntity(x, y, z));
	if (!chest) return;
	if (quality < 0) quality = 0;
	if (quality > 2) quality = 2;
	const bool rpg = isEnabled(level);

	const int size = chest->getContainerSize();

	// Distance from spawn nudges the material tier upwards
	Pos spawn = level->getSharedSpawnPos();
	float dx = (float) x - spawn.x;
	float dz = (float) z - spawn.z;
	int distBonus = (int) (std::sqrt(dx * dx + dz * dz) / 10.0f) + quality * 15;
	if (distBonus > 55) distBonus = 55;

	// 2-4 pieces of gear (+1 per quality tier), at least one weapon and one armor piece
	int gearCount = 2 + random->nextInt(3) + quality;
	for (int i = 0; i < gearCount; ++i) {
		int tierRoll = random->nextInt(100) + distBonus;
		if (tierRoll > 99) tierRoll = 99;

		bool weapon = (i == 0) ? true : (i == 1 ? false : random->nextInt(2) == 0);
		Item* item = weapon ? pickWeapon(random, tierRoll) : pickArmor(random, tierRoll);
		if (!item) continue;

		ItemInstance inst(item, 1, 0);
		// Every piece of chest gear is modified; ~12% are legendary (rollLootModifier
		// is 30% legendary, and we only ask for a modifier 40% of the time otherwise).
		if (rpg) {
			bool forceLegendary = random->nextInt(100) < 8 + quality * 12;
			if (quality == 2 && i == 0) forceLegendary = true; // boss chests always hold a legendary
			inst.setModifier(rollLootModifier(inst, random, forceLegendary));
		}

		int slot = random->nextInt(size);
		for (int tries = 0; tries < size; ++tries) {
			ItemInstance* existing = chest->getItem(slot);
			if (!existing || existing->isNull()) break;
			slot = (slot + 1) % size;
		}
		chest->setItem(slot, &inst);
	}

	// Mythic gear is exceptionally rare: ~1 in 6 boss chests, ~1 in 40 dungeon chests, ~1 in 200 world chests
	if (rpg) {
		int chance = quality == 2 ? 6 : (quality == 1 ? 40 : 200);
		if (random->nextInt(chance) == 0) {
			Item* mythic = rollMythicItem(random);
			if (mythic) {
				ItemInstance inst(mythic, 1, 0);
				int slot = random->nextInt(size);
				for (int tries = 0; tries < size; ++tries) {
					ItemInstance* existing = chest->getItem(slot);
					if (!existing || existing->isNull()) break;
					slot = (slot + 1) % size;
				}
				chest->setItem(slot, &inst);
			}
		}
	}

	// Some consumables
	int extraCount = 1 + random->nextInt(3) + quality;
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
