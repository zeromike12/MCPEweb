#ifndef NET_MINECRAFT_WORLD_RPG__Rpg_H__
#define NET_MINECRAFT_WORLD_RPG__Rpg_H__

//
// RPG game mode: player levels, mob levels, equipment modifiers and loot chests.
//
// Everything mode-specific lives behind Rpg::isEnabled(level) so the other
// game modes are untouched.
//

#include <string>

class Level;
class Mob;
class Player;
class Entity;
class ItemInstance;
class Random;

namespace Rpg {

	// ------------------------------------------------------------------
	// Mode
	// ------------------------------------------------------------------
	bool isEnabled(const Level* level);

	// ------------------------------------------------------------------
	// Player levels
	// ------------------------------------------------------------------
	const int MAX_PLAYER_LEVEL = 50;

	// XP needed to advance from `level` to `level + 1`.
	int xpToNextLevel(int level);

	// XP gained for killing a mob of the given level.
	int xpForKill(int mobLevel, bool hostile);

	// Multiplier applied to damage the player deals (1.0 at level 1).
	float playerDamageMultiplier(int playerLevel);

	// Multiplier applied to damage the player takes (1.0 at level 1, decreasing).
	float playerDefenseMultiplier(int playerLevel);

	// Player max health at the given level (20 at level 1).
	int playerMaxHealth(int playerLevel);

	// ------------------------------------------------------------------
	// Mob levels
	// ------------------------------------------------------------------
	const int MAX_MOB_LEVEL = 50;

	// Rolls a level for a mob spawning at (x, z) in `level`.
	int rollMobLevel(Level* level, float x, float z, Random* random);

	// Scaled max health for a mob whose base health is `baseHealth`.
	int mobMaxHealth(int baseHealth, int mobLevel);

	// Scaled melee / projectile damage for a mob.
	int mobDamage(int baseDamage, int mobLevel);

	// Text drawn above the mob, e.g. "Lv. 7 Zombie".
	std::string mobLabel(Mob* mob);

	// ------------------------------------------------------------------
	// Equipment modifiers
	// ------------------------------------------------------------------
	// Modifier ids stored in ItemInstance::modifier.
	// Rarity is encoded in the high nibble, the modifier kind in the low nibble.
	namespace Rarity {
		const int None      = 0;
		const int Common    = 1;
		const int Uncommon  = 2;
		const int Rare      = 3;
		const int Legendary = 4;
	}

	namespace Mod {
		const int None       = 0;
		// Weapon modifiers
		const int Sharp      = 1;  // +damage
		const int Heavy      = 2;  // ++damage, more knockback
		const int Vampiric   = 3;  // heals on hit
		const int Flaming    = 4;  // sets target on fire
		const int Swift      = 5;  // +damage, no durability loss
		const int Slaying    = 6;  // huge bonus vs. hostiles
		// Armor modifiers
		const int Sturdy     = 7;  // +defense
		const int Fortified  = 8;  // ++defense
		const int Vital      = 9;  // +max health
		const int Thorned    = 10; // reflects damage
		const int Warding    = 11; // reduces damage taken (%)
		const int Blessed    = 12; // +max health, +defense
	}

	int  makeModifier(int rarity, int kind);
	int  modifierRarity(int modifier);
	int  modifierKind(int modifier);

	// Rolls a modifier for a freshly crafted weapon/armor piece (never legendary).
	int rollCraftedModifier(const ItemInstance& item, Random* random);

	// Rolls a modifier for a piece of chest loot (can be legendary).
	int rollLootModifier(const ItemInstance& item, Random* random, bool forceLegendary);

	// Display name including the modifier prefix, with a colour code.
	std::string modifierPrefix(int modifier);
	std::string modifierColorCode(int modifier);
	std::string describeModifier(int modifier);

	// Effects
	int   weaponBonusDamage(int modifier, int baseDamage, bool targetHostile);
	int   armorBonusDefense(int modifier);
	int   armorBonusHealth(int modifier);
	float armorDamageReduction(int modifier); // 0..1 fraction removed
	int   armorThornsDamage(int modifier);    // reflected per hit
	bool  weaponNoDurabilityLoss(int modifier);
	int   weaponFireSeconds(int modifier);
	int   weaponLifeSteal(int modifier, int damageDealt);

	// Applies all on-hit weapon effects (fire, life steal). Called after hurt().
	void  applyWeaponOnHit(Player* attacker, Entity* target, ItemInstance* weapon, int damageDealt);

	// Summed bonuses from the player's four armor slots.
	int   playerArmorBonusDefense(Player* player);
	int   playerArmorBonusHealth(Player* player);
	float playerArmorDamageReduction(Player* player);
	int   playerArmorThorns(Player* player);

	// ------------------------------------------------------------------
	// Loot chests
	// ------------------------------------------------------------------
	// Tries to place a loot chest somewhere in the chunk starting at (xo, zo).
	void  placeLootChest(Level* level, int xo, int zo, Random* random);

	// Fills a container-like item array with loot.
	void  fillLootChest(Level* level, int x, int y, int z, Random* random);
}

#endif /*NET_MINECRAFT_WORLD_RPG__Rpg_H__*/
