#include "AetherMobs.h"
#include "../../aether/Aether.h"
#include "../../aether/AetherTiles.h"
#include "../../level/Level.h"
#include "../../level/tile/Tile.h"
#include "../../level/tile/FireTile.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"
#include "../player/Player.h"
#include "../../../nbt/CompoundTag.h"
#include "../../../util/Mth.h"
#include "../../../platform/log.h"

// ======================================================================
// Moa
// ======================================================================
Moa::Moa(Level* level)
:	super(level),
	tame(false),
	riderId(0),
	jumpsLeft(0),
	flapCooldown(0),
	flap(0), flapSpeed(0), oFlapSpeed(0), oFlap(0), flapping(1)
{
	entityRendererId = ER_MOA_RENDERER;
	textureName = "mob/moa.png";
	setSize(1.0f, 1.6f);
	runSpeed = 0.7f;
}

int Moa::getEntityTypeId() const { return MobTypes::Moa; }
int Moa::getMaxHealth() { return 20; }

std::string Moa::getTexture() { return textureName; }

bool Moa::removeWhenFarAway() { return !tame; }

bool Moa::canSpawn() {
	int xt = Mth::floor(x), yt = Mth::floor(bb.y0), zt = Mth::floor(z);
	int below = level->getTile(xt, yt - 1, zt);
	return (below == Aether::aetherGrass->id || below == Aether::enchantedGrass->id) && level->getRawBrightness(xt, yt, zt) > 8 && Mob::canSpawn();
}

bool Moa::isImmobile() {
	// A ridden Moa ignores its own AI; the rider drives it in tickRiding().
	return riderId != 0 || super::isImmobile();
}

Entity* Moa::findAttackTarget() { return NULL; }

void Moa::causeFallDamage(float distance) {
	// Moas glide; never take fall damage
}

bool Moa::interactPreventDefault() {
	return true;
}

bool Moa::interact(Player* player) {
	if (!player) return false;
	ItemInstance* item = player->inventory->getSelected();
	// Feeding blueberries tames a wild Moa
	if (!tame && item && !item->isNull() && item->id == Aether::blueberry->id) {
		if (!level->isClientSide) {
			tame = true;
			heal(4);
			for (int i = 0; i < 7; ++i)
				level->addParticle(PARTICLETYPE(crit), x + (random.nextFloat() - 0.5f) * bbWidth, y + 0.5f + random.nextFloat() * bbHeight, z + (random.nextFloat() - 0.5f) * bbWidth, 0, 0.1f, 0);
		}
		if (!player->abilities.instabuild) {
			item->count--;
			if (item->count <= 0) player->inventory->removeItem(item);
		}
		return true;
	}
	if (!tame) {
		if (level->isClientSide) player->displayClientMessage("This Moa is wild - feed it Blueberries first");
		return true;
	}
	// Mount / dismount
	if (riderId == player->entityId) {
		dismount();
		return true;
	}
	if (riderId != 0) return true;
	riderId = player->entityId;
	jumpsLeft = 3;
	player->noPhysics = true;
	player->fallDistance = 0;
	if (level->isClientSide) player->displayClientMessage("Riding Moa - jump to glide, sneak to dismount");
	return true;
}

void Moa::dismount() {
	Entity* e = level->getEntity(riderId);
	riderId = 0;
	if (e && e->isPlayer()) {
		Player* p = (Player*)e;
		p->noPhysics = false;
		p->fallDistance = 0;
		p->setPos(x, bb.y1 + 0.3f, z);
		p->xd = xd; p->zd = zd; p->yd = 0.1f;
	}
}

void Moa::tickRiding(Player* rider) {
	// The rider's movement intent steers the Moa.
	float xa = rider->getXxa();
	float ya = rider->getYya();
	bool jump = rider->isJumping();

	yRot = rider->yRot;
	yBodyRot = yRot;
	xRot = rider->xRot * 0.5f;

	float speed = onGround ? 0.22f : 0.14f;
	moveRelative(xa, ya, speed);

	// Multi-jump / glide
	if (onGround) {
		jumpsLeft = 3;
		flapCooldown = 0;
	}
	if (flapCooldown > 0) flapCooldown--;
	if (jump) {
		if (onGround) {
			yd = 0.55f;
			flapCooldown = 8;
		} else if (jumpsLeft > 0 && flapCooldown == 0) {
			yd = 0.5f;
			jumpsLeft--;
			flapCooldown = 10;
		} else if (yd < 0) {
			yd *= 0.6f; // glide
		}
	} else if (!onGround && yd < 0) {
		yd *= 0.85f; // gentle descent
	}

	yd -= 0.08f;
	move(xd, yd, zd);
	xd *= onGround ? 0.6f : 0.91f;
	zd *= onGround ? 0.6f : 0.91f;
	yd *= 0.98f;
	fallDistance = 0;

	// Carry the rider
	rider->fallDistance = 0;
	rider->xd = rider->yd = rider->zd = 0;
	rider->setPos(x, bb.y1 + rider->heightOffset - 0.35f, z);
	rider->onGround = true;
}

void Moa::tick() {
	super::tick();
	if (riderId != 0) {
		Entity* e = level->getEntity(riderId);
		Player* rider = (e && e->isPlayer() && e->isAlive()) ? (Player*)e : NULL;
		if (!rider || rider->isSneaking() || rider->level != level || health <= 0) {
			dismount();
		} else {
			tickRiding(rider);
		}
	}
}

void Moa::aiStep() {
	super::aiStep();
	oFlap = flap;
	oFlapSpeed = flapSpeed;
	flapSpeed += (onGround ? -1 : 4) * 0.3f;
	if (flapSpeed < 0) flapSpeed = 0;
	if (flapSpeed > 1) flapSpeed = 1;
	if (!onGround && flapping < 1) flapping = 1;
	flapping *= 0.9f;
	if (!onGround && yd < 0 && riderId == 0) yd *= 0.6f;
	flap += flapping * 2;
}

void Moa::addAdditonalSaveData(CompoundTag* tag) {
	super::addAdditonalSaveData(tag);
	tag->putBoolean("Tame", tame);
}

void Moa::readAdditionalSaveData(CompoundTag* tag) {
	super::readAdditionalSaveData(tag);
	tame = tag->getBoolean("Tame");
}

Animal* Moa::getBreedOffspring(Animal* target) {
	Moa* m = new Moa(level);
	m->tame = true;
	return m;
}

const char* Moa::getAmbientSound() { return "mob.chicken"; }
std::string Moa::getHurtSound() { return "mob.chickenhurt"; }
std::string Moa::getDeathSound() { return "mob.chickenhurt"; }

void Moa::dropDeathLoot() {
	int count = 1 + random.nextInt(3);
	for (int i = 0; i < count; i++) spawnAtLocation(Item::feather->id, 1);
	if (random.nextInt(3) == 0) spawnAtLocation(Aether::moaEgg->id, 1);
}

// ======================================================================
// Sentry
// ======================================================================
Sentry::Sentry(Level* level)
:	super(level), chargeTime(0)
{
	entityRendererId = ER_SENTRY_RENDERER;
	textureName = "mob/sentry.png";
	setSize(0.9f, 0.9f);
	runSpeed = 0.55f;
	attackDamage = 3;
}

Sentry::~Sentry() {}

int Sentry::getEntityTypeId() const { return MobTypes::Sentry; }
int Sentry::getMaxHealth() { return 16; }
bool Sentry::useNewAi() { return false; }
int Sentry::getArmorValue() { return 4; }
void Sentry::causeFallDamage(float distance) {}

bool Sentry::canSpawn() {
	// Sentries appear in dark places on/inside islands
	return isDarkEnoughToSpawn() && Mob::canSpawn();
}

int Sentry::getAttackDamage(Entity* target) {
	return chargeTime > 0 ? attackDamage + 3 : attackDamage;
}

void Sentry::aiStep() {
	super::aiStep();
	if (level->isClientSide) return;
	// Sentries hop toward their target and charge every few seconds
	Entity* target = getAttackTarget();
	if (target && target->isAlive()) {
		if (chargeTime > 0) {
			chargeTime--;
		} else if (random.nextInt(60) == 0) {
			chargeTime = 20;
			float dx = target->x - x, dz = target->z - z;
			float d = Mth::sqrt(dx * dx + dz * dz);
			if (d > 0.01f) {
				xd += dx / d * 0.7f;
				zd += dz / d * 0.7f;
				yd = 0.35f;
			}
			level->playSound(this, "mob.slime", 0.8f, 0.6f);
		}
		if (onGround && random.nextInt(10) == 0) yd = 0.3f;
	}
}

const char* Sentry::getAmbientSound() { return "mob.slime"; }
std::string Sentry::getHurtSound() { return "mob.slime"; }
std::string Sentry::getDeathSound() { return "mob.slime"; }

void Sentry::dropDeathLoot() {
	if (random.nextInt(2) == 0) spawnAtLocation(Aether::ambrosiumShard->id, 1);
	if (random.nextInt(8) == 0) spawnAtLocation(Aether::zaniteGem->id, 1);
}

// ======================================================================
// Mimic
// ======================================================================
Mimic::Mimic(Level* level)
:	super(level)
{
	entityRendererId = ER_MIMIC_RENDERER;
	textureName = "mob/mimic.png";
	setSize(0.9f, 1.0f);
	runSpeed = 0.75f;
	attackDamage = 5;
}

Mimic::~Mimic() {}

int Mimic::getEntityTypeId() const { return MobTypes::Mimic; }
int Mimic::getMaxHealth() { return 30; }
bool Mimic::useNewAi() { return false; }
bool Mimic::canSpawn() { return false; } // only from chest mimic blocks
int Mimic::getAttackDamage(Entity* target) { return attackDamage; }

void Mimic::aiStep() {
	super::aiStep();
	if (!level->isClientSide && getAttackTarget() == NULL) {
		Player* p = level->getNearestPlayer(this, 24);
		if (p) setAttackTarget(p);
	}
}

const char* Mimic::getAmbientSound() { return "random.chestopen"; }
std::string Mimic::getHurtSound() { return "random.wood_click"; }
std::string Mimic::getDeathSound() { return "random.chestclosed"; }

void Mimic::dropDeathLoot() {
	spawnAtLocation(Aether::skyrootPlanks->id, 1 + random.nextInt(4));
	if (random.nextInt(2) == 0) spawnAtLocation(Aether::ambrosiumShard->id, 1 + random.nextInt(2));
	if (random.nextInt(4) == 0) spawnAtLocation(Aether::bronzeKey->id, 1);
}

// ======================================================================
// AetherBoss
// ======================================================================
AetherBoss::AetherBoss(Level* level)
:	super(level), homeX(0), homeY(0), homeZ(0), tier(0), specialCooldown(0)
{
	entityRendererId = ER_AETHERBOSS_RENDERER;
	textureName = "mob/sentry.png";
	setSize(1.4f, 2.6f);
	runSpeed = 0.6f;
	attackDamage = 6;
	persistent = true;
	setTier(0);
}

AetherBoss::~AetherBoss() {}

int AetherBoss::getEntityTypeId() const { return MobTypes::AetherBoss; }
bool AetherBoss::useNewAi() { return false; }
bool AetherBoss::canSpawn() { return false; }
bool AetherBoss::removeWhenFarAway() { return false; }
void AetherBoss::causeFallDamage(float distance) {}

void AetherBoss::setTier(int t) {
	tier = t;
	switch (tier) {
	case 0: textureName = "mob/sentry.png"; attackDamage = 6; setSize(1.4f, 1.4f); break;   // Slider
	case 1: textureName = "mob/mimic.png"; attackDamage = 7; setSize(0.7f, 2.0f); break;    // Valkyrie Queen
	default: textureName = "mob/sentry.png"; attackDamage = 8; setSize(1.6f, 1.6f); break; // Sun Spirit
	}
	homeX = Mth::floor(x); homeY = Mth::floor(y); homeZ = Mth::floor(z);
}

std::string AetherBoss::getTexture() { return textureName; }

int AetherBoss::getMaxHealth() {
	return tier == 0 ? 120 : (tier == 1 ? 160 : 200);
}

int AetherBoss::getArmorValue() { return 6 + tier * 2; }
int AetherBoss::getAttackDamage(Entity* target) { return attackDamage; }

Entity* AetherBoss::findAttackTarget() {
	Player* p = level->getNearestPlayer(this, 20);
	if (p && !p->abilities.invulnerable) return p;
	return NULL;
}

bool AetherBoss::hurt(Entity* source, int dmg) {
	// The Slider can only be hurt by pickaxes; other bosses take normal damage
	if (tier == 0 && source && source->isPlayer()) {
		Player* p = (Player*)source;
		ItemInstance* item = p->inventory->getSelected();
		bool pick = item && !item->isNull() && (item->id == Item::pickAxe_wood->id || item->id == Item::pickAxe_stone->id
			|| item->id == Item::pickAxe_iron->id || item->id == Item::pickAxe_gold->id || item->id == Item::pickAxe_emerald->id
			|| item->id == Aether::skyrootPickaxe->id || item->id == Aether::zanitePickaxe->id || item->id == Aether::gravititePickaxe->id);
		if (!pick) {
			if (level->isClientSide) p->displayClientMessage("The Slider can only be hurt with a pickaxe");
			dmg = Mth::Max(1, dmg / 4);
		}
	}
	return super::hurt(source, dmg);
}

void AetherBoss::aiStep() {
	super::aiStep();
	if (level->isClientSide) return;
	// Stay near home
	float hdx = homeX + 0.5f - x, hdz = homeZ + 0.5f - z;
	if (hdx * hdx + hdz * hdz > 28 * 28) {
		setPos(homeX + 0.5f, homeY + 1.0f, homeZ + 0.5f);
		xd = yd = zd = 0;
	}
	if (specialCooldown > 0) { specialCooldown--; return; }
	Entity* target = getAttackTarget();
	if (!target || !target->isAlive()) return;
	float dx = target->x - x, dy = target->y - y, dz = target->z - z;
	float d = Mth::sqrt(dx * dx + dz * dz);
	switch (tier) {
	case 0: // Slider: charge in a straight line
		if (d > 0.5f) {
			xd += dx / d * 0.9f;
			zd += dz / d * 0.9f;
			yd = 0.2f;
		}
		specialCooldown = 50;
		level->playSound(this, "random.explode", 0.4f, 1.6f);
		break;
	case 1: // Valkyrie Queen: teleports behind the player and strikes
		if (d > 3.0f) {
			float rr = target->yRot * Mth::DEGRAD;
			setPos(target->x + Mth::sin(rr) * 2.0f, target->y, target->z - Mth::cos(rr) * 2.0f);
			for (int i = 0; i < 10; ++i)
				level->addParticle(PARTICLETYPE(portal), x + (random.nextFloat() - 0.5f), y + random.nextFloat() * 2, z + (random.nextFloat() - 0.5f), 0, 0, 0);
			level->playSound(this, "mob.endermen.portal", 1.0f, 1.0f);
		}
		specialCooldown = 70;
		break;
	default: // Sun Spirit: sets the ground ablaze around the target
		if (d < 12.0f) {
			for (int i = 0; i < 5; ++i) {
				int fx = Mth::floor(target->x) + random.nextInt(5) - 2;
				int fz = Mth::floor(target->z) + random.nextInt(5) - 2;
				int fy = Mth::floor(target->y);
				if (level->isEmptyTile(fx, fy, fz) && level->isSolidBlockingTile(fx, fy - 1, fz)) level->setTile(fx, fy, fz, Tile::fire->id);
			}
			if (target->isMob() && dy > -3 && dy < 3) ((Mob*)target)->hurt(this, 2);
			level->playSound(this, "mob.ghast.fireball", 0.8f, 1.0f);
		}
		specialCooldown = 80;
		break;
	}
	(void)dy;
}

void AetherBoss::die(Entity* source) {
	super::die(source);
	if (!level->isClientSide) {
		level->playSound(this, "random.levelup", 1.0f, 0.8f);
		LOGI("Aether boss tier %d defeated\n", tier);
	}
}

const char* AetherBoss::getAmbientSound() { return tier == 1 ? "mob.endermen.idle" : "mob.slime"; }
std::string AetherBoss::getHurtSound() { return tier == 1 ? "mob.endermen.hit" : "mob.slime"; }
std::string AetherBoss::getDeathSound() { return "random.explode"; }

void AetherBoss::dropDeathLoot() {
	Item* key = tier == 0 ? Aether::bronzeKey : (tier == 1 ? Aether::silverKey : Aether::goldKey);
	spawnAtLocation(key->id, 1 + random.nextInt(2));
	spawnAtLocation(Aether::ambrosiumShard->id, 4 + random.nextInt(6));
	spawnAtLocation(Aether::zaniteGem->id, 2 + random.nextInt(4));
	if (tier >= 1) spawnAtLocation(Aether::gravititePlate->id, 1 + random.nextInt(3));
	if (tier == 1) spawnAtLocation(Aether::zaniteSword->id, 1);
	if (tier == 2) {
		spawnAtLocation(Aether::sunAltar->id, 1);
		spawnAtLocation(Aether::gravititeSword->id, 1);
	}
}

void AetherBoss::addAdditonalSaveData(CompoundTag* tag) {
	super::addAdditonalSaveData(tag);
	tag->putInt("BossTier", tier);
	tag->putInt("HomeX", homeX);
	tag->putInt("HomeY", homeY);
	tag->putInt("HomeZ", homeZ);
}

void AetherBoss::readAdditionalSaveData(CompoundTag* tag) {
	super::readAdditionalSaveData(tag);
	int t = tag->getInt("BossTier");
	setTier(t);
	homeX = tag->getInt("HomeX");
	homeY = tag->getInt("HomeY");
	homeZ = tag->getInt("HomeZ");
	persistent = true;
}
