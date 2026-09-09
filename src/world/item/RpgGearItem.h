#ifndef NET_MINECRAFT_WORLD_ITEM__RpgGearItem_H__
#define NET_MINECRAFT_WORLD_ITEM__RpgGearItem_H__

//
// RPG mode unique gear: mythic armor sets with set bonuses and
// legendary weapons with special on-hit effects.
//

#include "WeaponItem.h"
#include "ArmorItem.h"
#include "BowItem.h"

namespace RpgGear {
	// Armor sets
	enum ArmorSet {
		SET_NONE        = -1,
		SET_DRAGONSCALE = 0,   // fire immunity, burns attackers
		SET_SHADOWWEAVE = 1,   // speed, stealth, no fall damage
		SET_TITANFORGED = 2,   // armor, knockback immunity, damage reduction
		SET_LIFEBLOOM   = 3,   // max health, regeneration
		SET_STORMCALLER = 4,   // bonus damage, chain lightning
		SET_COUNT       = 5
	};

	// Weapon special effects
	enum WeaponEffect {
		FX_NONE = 0,
		FX_CLEAVE,        // Obsidian Greatsword: heavy hit, big knockback
		FX_BLAZE,         // Blazing Katana: sets target ablaze
		FX_FROST,         // Frostbrand: freezes target in place
		FX_LEECH,         // Vampire Dagger: heals half the damage dealt
		FX_THUNDER,       // Thunder Hammer: shockwave damages nearby enemies
		FX_VENOM,         // Venom Fang: poisons target
		FX_EXECUTE,       // Executioner's Axe: double damage vs. wounded targets
		FX_SOUL,          // Soulreaper Scythe: +50% XP from kills
		FX_WIND,          // Blade of the Wind: wielder moves faster, never breaks
		FX_HUNT           // Ranger's Longbow: faster, harder-hitting arrows
	};
}

// A mythic weapon. Base damage is fixed rather than derived from a tool tier.
class RpgWeaponItem: public WeaponItem
{
	typedef WeaponItem super;
public:
	RpgWeaponItem(int id, int damage, int durability, RpgGear::WeaponEffect effect)
	:	super(id, Item::Tier::EMERALD),
		baseDamage(damage),
		effect(effect)
	{
		setMaxDamage(durability);
	}

	int getAttackDamage(Entity* entity) { return baseDamage; }

	const int baseDamage;
	const RpgGear::WeaponEffect effect;
};

// A mythic bow: arrows fly faster (and therefore hit harder) and always crit.
class RpgLongbowItem: public BowItem
{
	typedef BowItem super;
public:
	RpgLongbowItem(int id)
	:	super(id)
	{
		setMaxDamage(768);
	}

	void releaseUsing(ItemInstance* itemInstance, Level* level, Player* player, int durationLeft) {
		int timeHeld = getUseDuration(itemInstance) - durationLeft;
		float pow = timeHeld / (float) MAX_DRAW_DURATION;
		pow = ((pow * pow) + pow * 2) / 3;
		if (pow < 0.1f) return;
		if (pow > 1) pow = 1;

		itemInstance->hurt(1);
		level->playSound(player, "random.bow", 1.0f, 0.8f / (random.nextFloat() * 0.4f + 1.2f) + pow * 0.5f);
		player->inventory->removeResource(Item::arrow->id);
		if (!level->isClientSide) {
			Arrow* arrow = new Arrow(level, player, pow * 3.0f);
			arrow->critArrow = true;
			level->addEntity(arrow);
		}
	}
};

// A piece of a mythic armor set.
class RpgArmorItem: public ArmorItem
{
	typedef ArmorItem super;
public:
	static const ArmorMaterial MYTHIC;

	RpgArmorItem(int id, RpgGear::ArmorSet set, int slot)
	:	super(id, MYTHIC, 5 + (int) set, slot),
		set(set)
	{
	}

	const RpgGear::ArmorSet set;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__RpgGearItem_H__*/
