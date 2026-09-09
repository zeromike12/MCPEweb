#ifndef NET_MINECRAFT_WORLD_ENTITY_ANIMAL__AetherMobs_H__
#define NET_MINECRAFT_WORLD_ENTITY_ANIMAL__AetherMobs_H__

//
// Mobs of the Aether dimension.
//
//   Moa        - large flightless bird; hatched from Moa Eggs in the
//                Incubator (tame) or found wild. Tame Moas can be ridden:
//                the rider steers, jumping lets the Moa glide.
//   Sentry     - hostile cube-ish guardian of the Bronze dungeon.
//   Mimic      - chest-shaped monster spawned from Chest Mimic blocks.
//   AetherBoss - the dungeon boss (Slider / Valkyrie Queen / Sun Spirit
//                depending on its tier). While alive its dungeon stones are
//                indestructible; on death it drops the dungeon key (and the
//                Sun Altar for the gold tier).
//

#include "Animal.h"
#include "../monster/Monster.h"

class Player;
class CompoundTag;

class Moa : public Animal {
	typedef Animal super;
public:
	Moa(Level* level);
	int getEntityTypeId() const;
	int getMaxHealth();
	void aiStep();
	void tick();
	bool interact(Player* player);
	bool interactPreventDefault();
	bool canSpawn();
	void causeFallDamage(float distance);
	void addAdditonalSaveData(CompoundTag* tag);
	void readAdditionalSaveData(CompoundTag* tag);
	Animal* getBreedOffspring(Animal* target);
	bool isImmobile();
	std::string getTexture();
	bool removeWhenFarAway();

	bool isTame() const { return tame; }
	void setTame(bool t) { tame = t; }
	bool hasRider() const { return riderId != 0; }
	int getRiderId() const { return riderId; }
	void dismount();
protected:
	const char* getAmbientSound();
	std::string getHurtSound();
	std::string getDeathSound();
	void dropDeathLoot();
	Entity* findAttackTarget();
private:
	void tickRiding(Player* rider);
	bool tame;
	int riderId;
	int jumpsLeft;
	int flapCooldown;
public:
	float flap, flapSpeed, oFlapSpeed, oFlap, flapping;
};

class Sentry : public Monster {
	typedef Monster super;
public:
	Sentry(Level* level);
	~Sentry();
	int getEntityTypeId() const;
	int getMaxHealth();
	void aiStep();
	bool canSpawn();
	bool useNewAi();
	int getAttackDamage(Entity* target);
	void causeFallDamage(float distance);
protected:
	const char* getAmbientSound();
	std::string getHurtSound();
	std::string getDeathSound();
	void dropDeathLoot();
	int getArmorValue();
private:
	int chargeTime;
};

class Mimic : public Monster {
	typedef Monster super;
public:
	Mimic(Level* level);
	~Mimic();
	int getEntityTypeId() const;
	int getMaxHealth();
	bool canSpawn();
	bool useNewAi();
	int getAttackDamage(Entity* target);
	void aiStep();
protected:
	const char* getAmbientSound();
	std::string getHurtSound();
	std::string getDeathSound();
	void dropDeathLoot();
};

class AetherBoss : public Monster {
	typedef Monster super;
public:
	AetherBoss(Level* level);
	~AetherBoss();
	int getEntityTypeId() const;
	int getMaxHealth();
	void aiStep();
	bool canSpawn();
	bool useNewAi();
	int getAttackDamage(Entity* target);
	bool hurt(Entity* source, int dmg);
	void die(Entity* source);
	void causeFallDamage(float distance);
	void addAdditonalSaveData(CompoundTag* tag);
	void readAdditionalSaveData(CompoundTag* tag);
	std::string getTexture();
	bool removeWhenFarAway();
	int getTier() const { return tier; }
	void setTier(int t);
	// world position the boss guards (set when spawned by the dungeon builder)
	int homeX, homeY, homeZ;
protected:
	const char* getAmbientSound();
	std::string getHurtSound();
	std::string getDeathSound();
	void dropDeathLoot();
	int getArmorValue();
	Entity* findAttackTarget();
private:
	int tier;
	int specialCooldown;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_ANIMAL__AetherMobs_H__*/
