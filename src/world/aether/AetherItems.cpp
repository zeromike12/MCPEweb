#include "AetherItems.h"
#include "Aether.h"
#include "AetherTiles.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"
#include "../level/material/Material.h"
#include "../entity/player/Player.h"
#include "../phys/HitResult.h"
#include "../phys/Vec3.h"
#include "../Facing.h"
#include "../../util/Mth.h"

// ======================================================================
// BucketItem
// ======================================================================
BucketItem::BucketItem(int id, int tileId)
:	super(id), tileId(tileId)
{
	setMaxStackSize(1);
}

ItemInstance* BucketItem::use(ItemInstance* instance, Level* level, Player* player) {
	// Pick up liquids (empty bucket) by looking at a liquid source block.
	if (tileId != 0) return instance;
	if (!player) return instance;

	Vec3 from = player->getPos(1.0f);
	Vec3 to = from + player->getViewVector(1.0f) * 5.0f;
	HitResult hit = level->clip(from, to, true);
	if (!hit.isHit() || hit.type != TILE) return instance;

	int x = hit.x, y = hit.y, z = hit.z;
	const Material* m = level->getMaterial(x, y, z);
	int data = level->getData(x, y, z);
	Item* filled = NULL;
	if (m == Material::water && data == 0) filled = Item::bucket_water;
	else if (m == Material::lava && data == 0) filled = Item::bucket_lava;
	if (!filled) return instance;

	level->setTile(x, y, z, 0);
	if (player->abilities.instabuild) return instance;
	*instance = ItemInstance(filled, 1, 0);
	return instance;
}

bool BucketItem::emptyBucket(ItemInstance* instance, Level* level, Player* player, int x, int y, int z) {
	if (y < 0 || y >= Level::DEPTH) return false;
	int t = level->getTile(x, y, z);
	const Material* m = level->getMaterial(x, y, z);
	if (t != 0 && m->isSolid()) return false;
	if (t != 0 && !m->isLiquid() && !m->isReplaceable()) return false;

	if (tileId == Tile::water->id && level->dimension && level->dimension->ultraWarm) {
		// water evaporates in the Nether
		level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.fizz", 0.5f, 2.6f + (level->random.nextFloat() - level->random.nextFloat()) * 0.8f);
		for (int i = 0; i < 8; ++i) level->addParticle(PARTICLETYPE(largesmoke), x + level->random.nextFloat(), y + level->random.nextFloat(), z + level->random.nextFloat(), 0, 0, 0);
	} else {
		level->setTileAndData(x, y, z, tileId, 0);
	}
	if (player && !player->abilities.instabuild) *instance = ItemInstance(Item::bucket_empty, 1, 0);
	return true;
}

bool BucketItem::useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
	if (tileId == 0) {
		// Empty bucket: scoop the liquid we clicked (or the one behind the face)
		const Material* m = level->getMaterial(x, y, z);
		int data = level->getData(x, y, z);
		Item* filled = NULL;
		if (m == Material::water && data == 0) filled = Item::bucket_water;
		else if (m == Material::lava && data == 0) filled = Item::bucket_lava;
		if (!filled) return false;
		level->setTile(x, y, z, 0);
		if (player && !player->abilities.instabuild) *instance = ItemInstance(filled, 1, 0);
		return true;
	}

	// Water bucket on a glowstone frame lights an Aether portal (Survival / Creative only).
	if (tileId == Tile::water->id && Aether::isAvailable(level) && AetherPortalTile::isFrame(level->getTile(x, y, z))) {
		int px = x, py = y, pz = z;
		switch (face) {
		case Facing::DOWN:  py--; break;
		case Facing::UP:    py++; break;
		case Facing::NORTH: pz--; break;
		case Facing::SOUTH: pz++; break;
		case Facing::WEST:  px--; break;
		case Facing::EAST:  px++; break;
		}
		if (AetherPortalTile::trySpawnPortal(level, px, py, pz)) {
			level->playSound(px + 0.5f, py + 0.5f, pz + 0.5f, "random.splash", 1.0f, 1.0f);
			if (player && !player->abilities.instabuild) *instance = ItemInstance(Item::bucket_empty, 1, 0);
			return true;
		}
	}

	// Filled bucket: place the liquid against the clicked face (or into a replaceable block)
	int t = level->getTile(x, y, z);
	const Material* m = level->getMaterial(x, y, z);
	if (!(t != 0 && !m->isSolid())) {
		switch (face) {
		case Facing::DOWN:  y--; break;
		case Facing::UP:    y++; break;
		case Facing::NORTH: z--; break;
		case Facing::SOUTH: z++; break;
		case Facing::WEST:  x--; break;
		case Facing::EAST:  x++; break;
		}
	}
	return emptyBucket(instance, level, player, x, y, z);
}

// ======================================================================
// AmbrosiumShardItem
// ======================================================================
AmbrosiumShardItem::AmbrosiumShardItem(int id)
:	super(id)
{
}

ItemInstance* AmbrosiumShardItem::use(ItemInstance* instance, Level* level, Player* player) {
	if (!player || !player->isHurt()) return instance;
	player->heal(2);
	level->playSound(player, "random.burp", 0.5f, level->random.nextFloat() * 0.1f + 0.9f);
	if (!player->abilities.instabuild) instance->count--;
	return instance;
}

// ======================================================================
// Tool helpers
// ======================================================================
static float zaniteBonus(ItemInstance* item) {
	// Speed scales from 1x (fresh) up to 2x (nearly broken)
	if (!item || item->getMaxDamage() <= 0) return 1.0f;
	float worn = (float)item->getDamageValue() / (float)item->getMaxDamage();
	if (worn < 0) worn = 0;
	if (worn > 1) worn = 1;
	return 1.0f + worn;
}

static bool isAetherStone(Tile* tile) {
	return tile && tile->material == Material::stone;
}

ZanitePickaxeItem::ZanitePickaxeItem(int id, const Tier& tier) : super(id, tier) {}
float ZanitePickaxeItem::getDestroySpeed(ItemInstance* itemInstance, Tile* tile) {
	float s = super::getDestroySpeed(itemInstance, tile);
	if (s <= 1.0f && isAetherStone(tile)) s = tier.getSpeed();
	return s * zaniteBonus(itemInstance);
}

ZaniteAxeItem::ZaniteAxeItem(int id, const Tier& tier) : super(id, tier) {}
float ZaniteAxeItem::getDestroySpeed(ItemInstance* itemInstance, Tile* tile) {
	return super::getDestroySpeed(itemInstance, tile) * zaniteBonus(itemInstance);
}

ZaniteShovelItem::ZaniteShovelItem(int id, const Tier& tier) : super(id, tier) {}
float ZaniteShovelItem::getDestroySpeed(ItemInstance* itemInstance, Tile* tile) {
	float s = super::getDestroySpeed(itemInstance, tile);
	if (s <= 1.0f && tile && (tile->material == Material::dirt || tile->material == Material::sand)) s = tier.getSpeed();
	return s * zaniteBonus(itemInstance);
}

SkyrootPickaxeItem::SkyrootPickaxeItem(int id) : super(id, Item::Tier::WOOD) {}
float SkyrootPickaxeItem::getDestroySpeed(ItemInstance* itemInstance, Tile* tile) {
	float s = super::getDestroySpeed(itemInstance, tile);
	if (s <= 1.0f && isAetherStone(tile)) s = tier.getSpeed();
	return s;
}
SkyrootAxeItem::SkyrootAxeItem(int id) : super(id, Item::Tier::WOOD) {}
SkyrootShovelItem::SkyrootShovelItem(int id) : super(id, Item::Tier::WOOD) {}
float SkyrootShovelItem::getDestroySpeed(ItemInstance* itemInstance, Tile* tile) {
	float s = super::getDestroySpeed(itemInstance, tile);
	if (s <= 1.0f && tile && (tile->material == Material::dirt || tile->material == Material::sand)) s = tier.getSpeed();
	return s;
}

GravititePickaxeItem::GravititePickaxeItem(int id) : super(id, Item::Tier::EMERALD) {}
float GravititePickaxeItem::getDestroySpeed(ItemInstance* itemInstance, Tile* tile) {
	float s = super::getDestroySpeed(itemInstance, tile);
	if (s <= 1.0f && isAetherStone(tile)) s = tier.getSpeed();
	return s;
}
bool GravititePickaxeItem::useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
	// Gravitite tools lift the block they touch one space upward
	int t = level->getTile(x, y, z);
	if (t == 0 || y + 1 >= Level::DEPTH || !level->isEmptyTile(x, y + 1, z)) return false;
	Tile* tile = Tile::tiles[t];
	if (!tile || tile->destroySpeed < 0 || Tile::isEntityTile[t] || tile->material->isLiquid()) return false;
	if (!level->isClientSide) {
		int data = level->getData(x, y, z);
		level->setTile(x, y, z, 0);
		level->setTileAndData(x, y + 1, z, t, data);
		level->playSound(x + 0.5f, y + 1.0f, z + 0.5f, "random.pop", 0.6f, 1.4f);
	}
	if (player && !player->abilities.instabuild && instance) instance->hurt(1);
	return true;
}
GravititeAxeItem::GravititeAxeItem(int id) : super(id, Item::Tier::EMERALD) {}
GravititeShovelItem::GravititeShovelItem(int id) : super(id, Item::Tier::EMERALD) {}
float GravititeShovelItem::getDestroySpeed(ItemInstance* itemInstance, Tile* tile) {
	float s = super::getDestroySpeed(itemInstance, tile);
	if (s <= 1.0f && tile && (tile->material == Material::dirt || tile->material == Material::sand)) s = tier.getSpeed();
	return s;
}

// ======================================================================
// Skyroot door / bed / sign items
// ======================================================================
SkyrootDoorItem::SkyrootDoorItem(int id) : super(id) {
	setMaxStackSize(1);
}

bool SkyrootDoorItem::useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
	if (face != Facing::UP) return false;
	y++;
	Tile* tile = Aether::skyrootDoor;
	if (!tile->mayPlace(level, x, y, z)) return false;
	int dir = Mth::floor(((player->yRot + 180) * 4) / 360 - 0.5f) & 3;
	DoorItem::place(level, x, y, z, dir, tile);
	instance->count--;
	return true;
}

SkyrootBedItem::SkyrootBedItem(int id) : super(id) {
	setMaxStackSize(1);
}

bool SkyrootBedItem::useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
	if (face != Facing::UP) return false;
	y += 1;
	Tile* tile = Aether::skyrootBed;
	int dir = (Mth::floor(player->yRot * 4 / 360.0f + 0.5f)) & 3;
	int xra = 0, zra = 0;
	if (dir == 0) zra = 1;
	if (dir == 1) xra = -1;
	if (dir == 2) zra = -1;
	if (dir == 3) xra = 1;
	if (level->isEmptyTile(x, y, z) && level->isEmptyTile(x + xra, y, z + zra) && level->isSolidBlockingTile(x, y - 1, z) && level->isSolidBlockingTile(x + xra, y - 1, z + zra)) {
		level->setTileAndData(x, y, z, tile->id, dir);
		if (level->getTile(x, y, z) == tile->id) level->setTileAndData(x + xra, y, z + zra, tile->id, dir + BedTile::HEAD_PIECE_DATA);
		instance->count--;
		return true;
	}
	return false;
}

AetherSignItem::AetherSignItem(int id) : super(id) {
	setMaxStackSize(16);
}

bool AetherSignItem::useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
	if (face == 0) return false;
	if (!level->getMaterial(x, y, z)->isSolid()) return false;
	if (face == 1) y++;
	if (face == 2) z--;
	if (face == 3) z++;
	if (face == 4) x--;
	if (face == 5) x++;
	if (!Aether::aetherSign->mayPlace(level, x, y, z)) return false;
	if (face == 1) {
		int rot = Mth::floor(((player->yRot + 180) * 16) / 360 + 0.5f) & 15;
		level->setTileAndData(x, y, z, Aether::aetherSign->id, rot);
	} else {
		level->setTileAndData(x, y, z, Aether::aetherWallSign->id, face);
	}
	instance->count--;
	TileEntity* te = level->getTileEntity(x, y, z);
	if (te != NULL) player->openTextEdit(te);
	return true;
}

DungeonKeyItem::DungeonKeyItem(int id, int tier) : super(id), tier(tier) {
	setMaxStackSize(16);
}

MoaEggItem::MoaEggItem(int id) : super(id) {
	setMaxStackSize(16);
}

// ======================================================================
// ColdParachuteItem
// ======================================================================
ColdParachuteItem::ColdParachuteItem(int id)
:	super(id)
{
	setMaxStackSize(1);
	setMaxDamage(240);
}
