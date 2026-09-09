#include "AetherTiles.h"
#include "Aether.h"
#include "AetherTex.h"
#include "AetherTileEntity.h"
#include "AetherFeatures.h"

#include "../level/Level.h"
#include "../level/dimension/Dimension.h"
#include "../level/tile/entity/TileEntity.h"
#include "../level/tile/entity/ChestTileEntity.h"
#include "../level/tile/entity/SignTileEntity.h"
#include "../level/tile/IceTile.h"
#include "../level/tile/LiquidTile.h"
#include "../entity/player/Player.h"
#include "../entity/item/ItemEntity.h"
#include "../entity/Mob.h"
#include "../entity/EntityTypes.h"
#include "../item/Item.h"
#include "../item/ItemInstance.h"
#include "../item/DyePowderItem.h"
#include "../item/crafting/Recipe.h"
#include "../Facing.h"
#include "../Direction.h"
#include "../phys/AABB.h"
#include "../../util/Mth.h"
#include "../../platform/log.h"

// ======================================================================
// AetherBlock
// ======================================================================
AetherBlock::AetherBlock(int id, int tex, const Material* material)
:	super(id, tex, material), topTex(-1), bottomTex(-1)
{
}

AetherBlock* AetherBlock::setTopBottom(int top, int bottom) {
	topTex = top;
	bottomTex = bottom;
	return this;
}

int AetherBlock::getTexture(int face) {
	if (face == Facing::UP && topTex >= 0) return topTex;
	if (face == Facing::DOWN && bottomTex >= 0) return bottomTex;
	return tex;
}

int AetherBlock::getTexture(int face, int data) {
	return getTexture(face);
}

// ======================================================================
// AetherGrassTile
// ======================================================================
AetherGrassTile::AetherGrassTile(int id, int topTex, int sideTex, bool enchanted)
:	super(id, sideTex, Material::dirt), enchanted(enchanted)
{
	setTopBottom(topTex, AetherTex::AETHER_DIRT);
	setTicking(true);
}

int AetherGrassTile::getTexture(LevelSource* level, int x, int y, int z, int face) {
	return super::getTexture(face);
}

int AetherGrassTile::getTexture(int face, int data) {
	return super::getTexture(face);
}

void AetherGrassTile::tick(Level* level, int x, int y, int z, Random* random) {
	if (level->isClientSide) return;

	if (level->getRawBrightness(x, y + 1, z) < 4 && level->getMaterial(x, y + 1, z)->blocksLight()) {
		if (random->nextInt(4) != 0) return;
		level->setTile(x, y, z, Aether::aetherDirt->id);
		return;
	}
	if (level->getRawBrightness(x, y + 1, z) >= Level::MAX_BRIGHTNESS - 6) {
		int xt = x + random->nextInt(3) - 1;
		int yt = y + random->nextInt(5) - 3;
		int zt = z + random->nextInt(3) - 1;
		if (level->getTile(xt, yt, zt) == Aether::aetherDirt->id && level->getRawBrightness(xt, yt + 1, zt) >= 4 && !level->getMaterial(xt, yt + 1, zt)->blocksLight()) {
			level->setTile(xt, yt, zt, Aether::aetherGrass->id);
		}
	}
	// Enchanted grass occasionally sprouts berry bushes / flowers on top
	if (enchanted && random->nextInt(8) == 0 && level->isEmptyTile(x, y + 1, z) && level->getRawBrightness(x, y + 1, z) >= 9) {
		int r = random->nextInt(6);
		Tile* plant = (r == 0) ? Aether::berryBush : (r == 1 ? Aether::whiteFlower : (r == 2 ? Aether::purpleFlower : NULL));
		if (plant) level->setTile(x, y + 1, z, plant->id);
	}
}

int AetherGrassTile::getResource(int data, Random* random) {
	return Aether::aetherDirt->id;
}

bool AetherGrassTile::use(Level* level, int x, int y, int z, Player* player) {
	if (enchanted) return false;
	ItemInstance* item = player->inventory->getSelected();
	if (item == NULL || item->isNull() || item->id != Aether::ambrosiumShard->id) return false;
	if (!level->isClientSide) {
		level->setTile(x, y, z, Aether::enchantedGrass->id);
		for (int i = 0; i < 8; ++i) {
			level->addParticle(PARTICLETYPE(crit), x + level->random.nextFloat(), y + 1.1f, z + level->random.nextFloat(), 0, 0.05f, 0);
		}
	}
	if (!player->abilities.instabuild) {
		item->count--;
		if (item->count <= 0) player->inventory->removeItem(item);
	}
	return true;
}

// ======================================================================
// HolystoneTile
// ======================================================================
HolystoneTile::HolystoneTile(int id, int tex)
:	super(id, tex, Material::stone), drop(NULL)
{
}

HolystoneTile* HolystoneTile::setDrop(Tile* d) {
	drop = d;
	return this;
}

int HolystoneTile::getResource(int data, Random* random) {
	return drop ? drop->id : id;
}

// ======================================================================
// IcestoneTile
// ======================================================================
IcestoneTile::IcestoneTile(int id, int tex)
:	super(id, tex, Material::stone)
{
	setTicking(true);
}

void IcestoneTile::onPlace(Level* level, int x, int y, int z) {
	super::onPlace(level, x, y, z);
	freezeAround(level, x, y, z);
}

void IcestoneTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	super::neighborChanged(level, x, y, z, type);
	if (!level->isClientSide) level->addToTickNextTick(x, y, z, id, 4);
}

void IcestoneTile::tick(Level* level, int x, int y, int z, Random* random) {
	freezeAround(level, x, y, z);
}

/*static*/ void IcestoneTile::freezeAround(Level* level, int x, int y, int z) {
	if (level->isClientSide) return;
	freezeAt(level, x - 1, y, z);
	freezeAt(level, x + 1, y, z);
	freezeAt(level, x, y - 1, z);
	freezeAt(level, x, y + 1, z);
	freezeAt(level, x, y, z - 1);
	freezeAt(level, x, y, z + 1);
}

/*static*/ void IcestoneTile::freezeAt(Level* level, int x, int y, int z) {
	const Material* m = level->getMaterial(x, y, z);
	if (m == Material::water) {
		level->setTile(x, y, z, Tile::ice->id);
	} else if (m == Material::lava) {
		level->setTile(x, y, z, Tile::obsidian->id);
		level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.fizz", 0.5f, 2.6f + (level->random.nextFloat() - level->random.nextFloat()) * 0.8f);
	}
}

// ======================================================================
// QuicksoilTile
// ======================================================================
QuicksoilTile::QuicksoilTile(int id, int tex)
:	super(id, tex, Material::sand)
{
	friction = 1.1f; // slipperier than ice (0.98)
}

static void quicksoilBoost(Entity* entity) {
	if (!entity || entity->isSneaking()) return;
	float speed = Mth::sqrt(entity->xd * entity->xd + entity->zd * entity->zd);
	if (speed < 0.001f) return;
	float target = 0.85f;
	if (speed < target) {
		float k = 1.0f + (target - speed) * 0.12f;
		entity->xd *= k;
		entity->zd *= k;
	}
}

void QuicksoilTile::entityInside(Level* level, int x, int y, int z, Entity* entity) {
	// entityInside is called for blocks overlapping the bounding box, which
	// for full cubes never happens while standing on top - kept for
	// completeness (e.g. quicksoil inside water).
}

void QuicksoilTile::stepOn(Level* level, int x, int y, int z, Entity* entity) {
	quicksoilBoost(entity);
}

// ======================================================================
// AercloudTile
// ======================================================================
AercloudTile::AercloudTile(int id, int tex, Kind kind)
:	super(id, tex, Material::cloth, false), kind(kind)
{
	setLightBlock(0);
	friction = 0.6f;
	setTicking(false);
}

int AercloudTile::getRenderLayer() {
	return Tile::RENDERLAYER_BLEND;
}

bool AercloudTile::blocksLight() {
	return false;
}

int AercloudTile::getResourceCount(Random* random) {
	return 1;
}

int AercloudTile::getColor(int data) {
	return 0xffffff;
}

void AercloudTile::entityInside(Level* level, int x, int y, int z, Entity* entity) {
	// Only reached for non-cube shapes; aerclouds are cubes so stepOn/fallOn do the work.
}

void AercloudTile::fallOn(Level* level, int x, int y, int z, Entity* entity, float fallDistance) {
	if (!entity) return;
	switch (kind) {
	case Cold:
	case Golden:
	case Green:
		entity->fallDistance = 0; // no fall damage at all
		break;
	case Blue:
		entity->fallDistance = 0;
		// bounce straight up, higher the further you fell
		entity->yd = 0.9f + Mth::Min(fallDistance, 20.0f) * 0.03f;
		entity->onGround = false;
		if (!level->isClientSide) level->playSound(entity, "mob.slime", 0.6f, 1.2f);
		break;
	case Purple:
		entity->fallDistance = 0;
		// flings you sideways in your looking direction
		{
			float rr = entity->yRot * Mth::DEGRAD;
			entity->xd -= Mth::sin(rr) * 1.2f;
			entity->zd += Mth::cos(rr) * 1.2f;
			entity->yd = 0.4f;
			entity->onGround = false;
		}
		break;
	case Storm:
		entity->fallDistance = 0;
		if (!level->isClientSide && entity->isMob()) {
			Mob* mob = (Mob*)entity;
			if (mob->invulnerableTime <= 0) {
				mob->hurt(NULL, 2); // shock
				level->addParticle(PARTICLETYPE(largesmoke), entity->x, entity->y, entity->z, 0, 0.1f, 0);
			}
		}
		entity->yd = 0.6f;
		entity->onGround = false;
		break;
	}
}

void AercloudTile::stepOn(Level* level, int x, int y, int z, Entity* entity) {
	if (!entity) return;
	switch (kind) {
	case Golden:
		// Golden clouds let you "climb": every step is a small hop upward.
		if (entity->onGround && entity->isMob()) {
			entity->yd = 0.45f;
			entity->onGround = false;
			entity->fallDistance = 0;
		}
		break;
	case Green:
		// Green clouds launch you forward hard.
		{
			float rr = entity->yRot * Mth::DEGRAD;
			entity->xd -= Mth::sin(rr) * 0.9f;
			entity->zd += Mth::cos(rr) * 0.9f;
			entity->yd = 0.35f;
			entity->onGround = false;
			entity->fallDistance = 0;
		}
		break;
	case Cold:
		entity->fallDistance = 0;
		break;
	default:
		break;
	}
}

void AercloudTile::animateTick(Level* level, int x, int y, int z, Random* random) {
	if (kind == Storm && random->nextInt(12) == 0) {
		level->addParticle(PARTICLETYPE(largesmoke), x + random->nextFloat(), y + 1.05f, z + random->nextFloat(), 0, 0.02f, 0);
	}
}

// ======================================================================
// AerogelTile
// ======================================================================
AerogelTile::AerogelTile(int id, int tex)
:	super(id, tex, Material::glass, false)
{
	setLightBlock(0);
}

int AerogelTile::getRenderLayer() {
	return Tile::RENDERLAYER_BLEND;
}

bool AerogelTile::blocksLight() {
	return false;
}

void AerogelTile::fallOn(Level* level, int x, int y, int z, Entity* entity, float fallDistance) {
	if (!entity) return;
	entity->fallDistance = 0;
	if (fallDistance > 2.0f && !entity->isSneaking()) {
		entity->yd = Mth::Min(fallDistance, 12.0f) * 0.05f;
		entity->onGround = false;
	}
}

float AerogelTile::getExplosionResistance(Entity* source) {
	return 2000.0f / 5.0f;
}

// ======================================================================
// AetherLogTile
// ======================================================================
AetherLogTile::AetherLogTile(int id, int sideTex, int topTex, bool golden)
:	super(id, sideTex, Material::wood), golden(golden)
{
	setTopBottom(topTex, topTex);
}

int AetherLogTile::getResource(int data, Random* random) {
	// Golden oak drops skyroot logs (amber comes from the axe check in playerDestroy)
	return Aether::skyrootLog->id;
}

int AetherLogTile::getResourceCount(Random* random) {
	return 1;
}

void AetherLogTile::playerDestroy(Level* level, Player* player, int x, int y, int z, int data) {
	if (!level->isClientSide) {
		ItemInstance* item = player->inventory->getSelected();
		int tier = item ? Aether::axeTier(item->id) : 0;
		if (golden && tier >= 2) {
			// Zanite / gravitite axe: golden amber
			int count = 1 + level->random.nextInt(tier == 3 ? 3 : 2);
			for (int i = 0; i < count; ++i) popResource(level, x, y, z, ItemInstance(Aether::goldenAmber, 1, 0));
		}
		if (tier == 1 && level->random.nextInt(2) == 0) {
			// Skyroot tools double-drop
			popResource(level, x, y, z, ItemInstance(Aether::skyrootLog, 1, 0));
		}
	}
	super::playerDestroy(level, player, x, y, z, data);
}

void AetherLogTile::onRemove(Level* level, int x, int y, int z) {
	int r = LeafTile::REQUIRED_WOOD_RANGE;
	int r2 = r + 1;
	if (level->hasChunksAt(x - r2, y - r2, z - r2, x + r2, y + r2, z + r2)) {
		for (int xo = -r; xo <= r; xo++)
		for (int yo = -r; yo <= r; yo++)
		for (int zo = -r; zo <= r; zo++) {
			int t = level->getTile(x + xo, y + yo, z + zo);
			if (t == Aether::skyrootLeaves->id || t == Aether::goldenOakLeaves->id) {
				int currentData = level->getData(x + xo, y + yo, z + zo);
				if ((currentData & LeafTile::UPDATE_LEAF_BIT) == 0) {
					level->setDataNoUpdate(x + xo, y + yo, z + zo, currentData | LeafTile::UPDATE_LEAF_BIT);
				}
			}
		}
	}
	super::onRemove(level, x, y, z);
}

// ======================================================================
// AetherLeafTile
// ======================================================================
AetherLeafTile::AetherLeafTile(int id, int tex, bool golden)
:	super(id, tex), golden(golden)
{
}

int AetherLeafTile::getColor(LevelSource* level, int x, int y, int z) {
	return 0xffffff; // textures are pre-colored
}

int AetherLeafTile::getColor(int data) {
	return 0xffffff;
}

int AetherLeafTile::getTexture(int face, int data) {
	return tex;
}

bool AetherLeafTile::isSolidRender() {
	return false;
}

int AetherLeafTile::getRenderLayer() {
	return Tile::RENDERLAYER_ALPHATEST;
}

int AetherLeafTile::getResource(int data, Random* random) {
	return golden ? Aether::goldenOakSapling->id : Aether::skyrootSapling->id;
}

void AetherLeafTile::spawnResources(Level* level, int x, int y, int z, int data, float odds) {
	if (level->isClientSide) return;
	if (level->random.nextInt(golden ? 40 : 20) == 0) {
		popResource(level, x, y, z, ItemInstance(getResource(data, &level->random), 1, 0));
	}
	if (level->random.nextInt(50) == 0) {
		popResource(level, x, y, z, ItemInstance(Item::stick, 1, 0));
	}
}

void AetherLeafTile::playerDestroy(Level* level, Player* player, int x, int y, int z, int data) {
	if (!level->isClientSide) {
		ItemInstance* item = player->inventory->getSelected();
		if (item && item->id == ((Item*)Item::shears)->id) {
			popResource(level, x, y, z, ItemInstance(this, 1, 0));
			return;
		}
	}
	Tile::playerDestroy(level, player, x, y, z, data);
}

bool AetherLeafTile::hasTrunkNearby(Level* level, int x, int y, int z) {
	const int r = LeafTile::REQUIRED_WOOD_RANGE;
	int skyId = Aether::skyrootLog->id;
	int goldId = Aether::goldenOakLog->id;
	for (int xo = -r; xo <= r; xo++)
	for (int yo = -r; yo <= r; yo++)
	for (int zo = -r; zo <= r; zo++) {
		int t = level->getTile(x + xo, y + yo, z + zo);
		if (t == skyId || t == goldId) return true;
	}
	return false;
}

void AetherLeafTile::tick(Level* level, int x, int y, int z, Random* random) {
	if (level->isClientSide) return;
	int currentData = level->getData(x, y, z);
	if ((currentData & LeafTile::UPDATE_LEAF_BIT) != 0 && (currentData & LeafTile::PERSISTENT_LEAF_BIT) == 0) {
		const int r2 = LeafTile::REQUIRED_WOOD_RANGE + 1;
		if (!level->hasChunksAt(x - r2, y - r2, z - r2, x + r2, y + r2, z + r2)) return;
		if (hasTrunkNearby(level, x, y, z)) {
			level->setDataNoUpdate(x, y, z, currentData & ~LeafTile::UPDATE_LEAF_BIT);
		} else {
			spawnResources(level, x, y, z, currentData, 0);
			level->setTile(x, y, z, 0);
		}
	}
}

void AetherLeafTile::onRemove(Level* level, int x, int y, int z) {
	int r = 1;
	int r2 = r + 1;
	if (level->hasChunksAt(x - r2, y - r2, z - r2, x + r2, y + r2, z + r2)) {
		for (int xo = -r; xo <= r; xo++)
		for (int yo = -r; yo <= r; yo++)
		for (int zo = -r; zo <= r; zo++) {
			int t = level->getTile(x + xo, y + yo, z + zo);
			if (t == Aether::skyrootLeaves->id || t == Aether::goldenOakLeaves->id) {
				int currentData = level->getData(x + xo, y + yo, z + zo);
				level->setDataNoUpdate(x + xo, y + yo, z + zo, currentData | LeafTile::UPDATE_LEAF_BIT);
			}
		}
	}
}

// ======================================================================
// AetherBush / AetherSapling / BerryBushTile
// ======================================================================
AetherBush::AetherBush(int id, int tex)
:	super(id, tex)
{
}

bool AetherBush::mayPlaceOn(int tile) {
	return tile == Aether::aetherGrass->id || tile == Aether::enchantedGrass->id || tile == Aether::aetherDirt->id
		|| tile == ((Tile*)Tile::grass)->id || tile == Tile::dirt->id || tile == Tile::farmland->id;
}

AetherSapling::AetherSapling(int id, int tex, bool golden)
:	super(id, tex), golden(golden)
{
	float ss = 0.4f;
	setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, ss * 2, 0.5f + ss);
}

void AetherSapling::tick(Level* level, int x, int y, int z, Random* random) {
	if (level->isClientSide) return;
	super::tick(level, x, y, z, random);
	if (level->getTile(x, y, z) != id) return;
	if (level->getRawBrightness(x, y + 1, z) >= Level::MAX_BRIGHTNESS - 6 && random->nextInt(7) == 0) {
		int data = level->getData(x, y, z);
		if ((data & 8) == 0) {
			level->setDataNoUpdate(x, y, z, data | 8);
		} else {
			growTree(level, x, y, z, random);
		}
	}
}

void AetherSapling::growTree(Level* level, int x, int y, int z, Random* random) {
	level->setTile(x, y, z, 0);
	AetherTreeFeature f(true, golden);
	if (!f.place(level, random, x, y, z)) {
		level->setTile(x, y, z, id);
	}
}

bool AetherSapling::use(Level* level, int x, int y, int z, Player* player) {
	// Bone meal / ambrosium shard grows the tree instantly
	ItemInstance* item = player->inventory->getSelected();
	if (item == NULL || item->isNull()) return false;
	bool boneMeal = (item->id == Item::dye_powder->id && item->getAuxValue() == DyePowderItem::WHITE);
	if (!boneMeal && item->id != Aether::ambrosiumShard->id) return false;
	if (!level->isClientSide) growTree(level, x, y, z, &level->random);
	if (!player->abilities.instabuild) {
		item->count--;
		if (item->count <= 0) player->inventory->removeItem(item);
	}
	return true;
}

BerryBushTile::BerryBushTile(int id, int tex)
:	super(id, tex)
{
	setShape(0.1f, 0, 0.1f, 0.9f, 0.8f, 0.9f);
	setTicking(true);
}

int BerryBushTile::getTexture(int face, int data) {
	return (data & 1) ? AetherTex::BERRY_BUSH_RIPE : AetherTex::BERRY_BUSH;
}

AABB* BerryBushTile::getAABB(Level* level, int x, int y, int z) {
	// Ripe bushes block movement a little (like a hedge), unripe ones don't
	return NULL;
}

void BerryBushTile::tick(Level* level, int x, int y, int z, Random* random) {
	super::tick(level, x, y, z, random);
	if (level->isClientSide || level->getTile(x, y, z) != id) return;
	int data = level->getData(x, y, z);
	if ((data & 1) == 0 && random->nextInt(12) == 0 && level->getRawBrightness(x, y + 1, z) >= 9) {
		level->setData(x, y, z, data | 1);
	}
}

bool BerryBushTile::use(Level* level, int x, int y, int z, Player* player) {
	int data = level->getData(x, y, z);
	if ((data & 1) == 0) return false;
	if (!level->isClientSide) {
		int count = 1 + level->random.nextInt(3);
		int below = level->getTile(x, y - 1, z);
		if (below == Aether::enchantedGrass->id) count += 2;
		for (int i = 0; i < count; ++i) popResource(level, x, y, z, ItemInstance(Aether::blueberry, 1, 0));
		level->setData(x, y, z, data & ~1);
	}
	return true;
}

int BerryBushTile::getResource(int data, Random* random) {
	return (data & 1) ? Aether::blueberry->id : id;
}

int BerryBushTile::getResourceCount(Random* random) {
	return 1;
}

void BerryBushTile::spawnResources(Level* level, int x, int y, int z, int data, float odds) {
	if (level->isClientSide) return;
	if (data & 1) {
		int count = 1 + level->random.nextInt(3);
		for (int i = 0; i < count; ++i) popResource(level, x, y, z, ItemInstance(Aether::blueberry, 1, 0));
	}
	// the bush itself always comes back as an unripe bush
	popResource(level, x, y, z, ItemInstance(this, 1, 0));
}

// ======================================================================
// AetherOreTile
// ======================================================================
AetherOreTile::AetherOreTile(int id, int tex, Kind kind)
:	super(id, tex), kind(kind)
{
	if (kind == Gravitite) setTicking(true);
}

int AetherOreTile::getResource(int data, Random* random) {
	switch (kind) {
	case Ambrosium: return Aether::ambrosiumShard->id;
	case Zanite:    return Aether::zaniteGem->id;
	default:        return id;
	}
}

int AetherOreTile::getResourceCount(Random* random) {
	if (kind == Ambrosium) return 1 + random->nextInt(2);
	return 1;
}

int AetherOreTile::getTickDelay() {
	return 3;
}

void AetherOreTile::onPlace(Level* level, int x, int y, int z) {
	super::onPlace(level, x, y, z);
	if (kind == Gravitite && !level->isClientSide) level->addToTickNextTick(x, y, z, id, getTickDelay());
}

void AetherOreTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	super::neighborChanged(level, x, y, z, type);
	if (kind == Gravitite && !level->isClientSide) level->addToTickNextTick(x, y, z, id, getTickDelay());
}

void AetherOreTile::tick(Level* level, int x, int y, int z, Random* random) {
	if (kind == Gravitite) tryFloat(level, x, y, z);
}

void AetherOreTile::tryFloat(Level* level, int x, int y, int z) {
	if (level->isClientSide) return;
	// Gravitite floats upward once it is uncovered
	if (y + 1 < Level::DEPTH && level->isEmptyTile(x, y + 1, z) && level->getTile(x, y, z) == id) {
		bool exposed = level->isEmptyTile(x - 1, y, z) || level->isEmptyTile(x + 1, y, z)
			|| level->isEmptyTile(x, y, z - 1) || level->isEmptyTile(x, y, z + 1) || level->isEmptyTile(x, y - 1, z);
		if (!exposed) return;
		level->setTile(x, y, z, 0);
		level->setTile(x, y + 1, z, id);
		level->addToTickNextTick(x, y + 1, z, id, getTickDelay());
	}
}

// ======================================================================
// EnchantedGravititeTile
// ======================================================================
EnchantedGravititeTile::EnchantedGravititeTile(int id, int tex)
:	super(id, tex, Material::metal)
{
	setTicking(true);
}

int EnchantedGravititeTile::getTickDelay() {
	return 2;
}

void EnchantedGravititeTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	super::neighborChanged(level, x, y, z, type);
	if (!level->isClientSide) level->addToTickNextTick(x, y, z, id, getTickDelay());
}

void EnchantedGravititeTile::tick(Level* level, int x, int y, int z, Random* random) {
	if (level->isClientSide) return;
	if (level->hasNeighborSignal(x, y, z) && y + 1 < Level::DEPTH && level->isEmptyTile(x, y + 1, z)) {
		level->setTile(x, y, z, 0);
		level->setTile(x, y + 1, z, id);
		level->addToTickNextTick(x, y + 1, z, id, getTickDelay());
	}
}

bool EnchantedGravititeTile::isSignalSource() {
	return true;
}

bool EnchantedGravititeTile::getSignal(LevelSource* level, int x, int y, int z, int dir) {
	// Emits a signal while it is floating (nothing solid below it)
	return !level->isSolidBlockingTile(x, y - 1, z);
}

bool EnchantedGravititeTile::getDirectSignal(Level* level, int x, int y, int z, int dir) {
	return getSignal(level, x, y, z, dir);
}

// ======================================================================
// Dungeon stones
// ======================================================================
DungeonStoneTile::DungeonStoneTile(int id, int tex, int dungeonTier, bool light)
:	super(id, tex), dungeonTier(dungeonTier), light(light)
{
	if (light) setLightEmission(11.0f / 16.0f);
}

float DungeonStoneTile::getDestroyProgress(Player* player) {
	if (player && player->level && !player->abilities.instabuild) {
		int x = Mth::floor(player->x), y = Mth::floor(player->y), z = Mth::floor(player->z);
		if (Aether::isDungeonGuarded(player->level, dungeonTier, x, y, z)) return 0;
	}
	return super::getDestroyProgress(player);
}

float DungeonStoneTile::getExplosionResistance(Entity* source) {
	if (source && source->level) {
		int x = Mth::floor(source->x), y = Mth::floor(source->y), z = Mth::floor(source->z);
		if (Aether::isDungeonGuarded(source->level, dungeonTier, x, y, z)) return 6000000.0f / 5.0f;
	}
	return super::getExplosionResistance(source);
}

TrappedSentryStoneTile::TrappedSentryStoneTile(int id, int tex)
:	super(id, tex, Aether::Dungeon::Bronze, false)
{
}

void TrappedSentryStoneTile::stepOn(Level* level, int x, int y, int z, Entity* entity) {
	if (entity && entity->isPlayer()) trigger(level, x, y, z);
}

void TrappedSentryStoneTile::entityInside(Level* level, int x, int y, int z, Entity* entity) {
	if (entity && entity->isPlayer()) trigger(level, x, y, z);
}

void TrappedSentryStoneTile::trigger(Level* level, int x, int y, int z) {
	if (level->isClientSide) return;
	// becomes a plain sentry stone and releases two sentries
	level->setTile(x, y, z, Aether::sentryStone->id);
	level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.click", 1.0f, 0.6f);
	for (int i = 0; i < 2; ++i) {
		float sx = x + 0.5f + (level->random.nextFloat() - 0.5f) * 3.0f;
		float sz = z + 0.5f + (level->random.nextFloat() - 0.5f) * 3.0f;
		Aether::spawnMob(level, MobTypes::Sentry, sx, y + 1.0f, sz);
	}
}

LockedStoneTile::LockedStoneTile(int id, int tex, int dungeonTier)
:	super(id, tex, dungeonTier, false)
{
}

float LockedStoneTile::getDestroyProgress(Player* player) {
	if (player && !player->abilities.instabuild) return 0; // only keys open these
	return super::getDestroyProgress(player);
}

bool LockedStoneTile::use(Level* level, int x, int y, int z, Player* player) {
	ItemInstance* item = player->inventory->getSelected();
	if (item == NULL || item->isNull()) return false;
	Item* needed = (getDungeonTier() == Aether::Dungeon::Bronze) ? Aether::bronzeKey
		: (getDungeonTier() == Aether::Dungeon::Silver ? Aether::silverKey : Aether::goldKey);
	if (item->id != needed->id) {
		if (player) player->displayClientMessage("This door needs a key");
		return true;
	}
	if (!level->isClientSide) {
		unlockConnected(level, x, y, z, 0);
		level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.door_open", 1.0f, 1.0f);
	}
	if (!player->abilities.instabuild) {
		item->count--;
		if (item->count <= 0) player->inventory->removeItem(item);
	}
	return true;
}

void LockedStoneTile::unlockConnected(Level* level, int x, int y, int z, int depth) {
	if (depth > 12 || level->getTile(x, y, z) != id) return;
	level->setTile(x, y, z, 0);
	unlockConnected(level, x - 1, y, z, depth + 1);
	unlockConnected(level, x + 1, y, z, depth + 1);
	unlockConnected(level, x, y - 1, z, depth + 1);
	unlockConnected(level, x, y + 1, z, depth + 1);
	unlockConnected(level, x, y, z - 1, depth + 1);
	unlockConnected(level, x, y, z + 1, depth + 1);
}

PillarTile::PillarTile(int id, int sideTex, int topTex)
:	super(id, sideTex, Material::stone)
{
	setTopBottom(topTex, topTex);
}

int PillarTile::getTexture(int face, int data) {
	int axis = data & 3;
	if (axis == 1) return (face == Facing::WEST || face == Facing::EAST) ? topTex : tex;
	if (axis == 2) return (face == Facing::NORTH || face == Facing::SOUTH) ? topTex : tex;
	return (face <= 1) ? topTex : tex;
}

int PillarTile::getTexture(LevelSource* level, int x, int y, int z, int face) {
	return getTexture(face, level->getData(x, y, z));
}

int PillarTile::getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue) {
	if (face == Facing::WEST || face == Facing::EAST) return 1;
	if (face == Facing::NORTH || face == Facing::SOUTH) return 2;
	return 0;
}

// ======================================================================
// AetherSlabTile
// ======================================================================
AetherSlabTile::AetherSlabTile(int id, int tex, const Material* material, bool fullSize, int halfId, int fullId)
:	super(id, tex, material), fullSize(fullSize), halfId(halfId), fullId(fullId)
{
	if (!fullSize) setShape(0, 0, 0, 1, 0.5f, 1);
	setLightBlock(255);
}

bool AetherSlabTile::isSolidRender() { return fullSize; }
bool AetherSlabTile::isCubeShaped() { return fullSize; }

void AetherSlabTile::updateShape(LevelSource* level, int x, int y, int z) {
	if (fullSize) setShape(0, 0, 0, 1, 1, 1);
	else if (level->getData(x, y, z) & TOP_SLOT_BIT) setShape(0, 0.5f, 0, 1, 1, 1);
	else setShape(0, 0, 0, 1, 0.5f, 1);
}

void AetherSlabTile::updateDefaultShape() {
	if (fullSize) setShape(0, 0, 0, 1, 1, 1);
	else setShape(0, 0, 0, 1, 0.5f, 1);
}

void AetherSlabTile::addAABBs(Level* level, int x, int y, int z, const AABB* box, std::vector<AABB>& boxes) {
	updateShape(level, x, y, z);
	super::addAABBs(level, x, y, z, box, boxes);
}

int AetherSlabTile::getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue) {
	if (fullSize) return 0;
	// Placing a slab onto the exposed half of another half-slab makes a double slab
	if (face == Facing::DOWN || (face != Facing::UP && clickY > 0.5f)) return TOP_SLOT_BIT;
	return 0;
}

int AetherSlabTile::getResource(int data, Random* random) { return halfId; }
int AetherSlabTile::getResourceCount(Random* random) { return fullSize ? 2 : 1; }

bool AetherSlabTile::shouldRenderFace(LevelSource* level, int x, int y, int z, int face) {
	if (fullSize) return super::shouldRenderFace(level, x, y, z, face);
	if (face != Facing::UP && face != Facing::DOWN && !super::shouldRenderFace(level, x, y, z, face)) return false;
	int ox = x + Facing::STEP_X[Facing::OPPOSITE_FACING[face]];
	int oy = y + Facing::STEP_Y[Facing::OPPOSITE_FACING[face]];
	int oz = z + Facing::STEP_Z[Facing::OPPOSITE_FACING[face]];
	bool upper = (level->getData(ox, oy, oz) & TOP_SLOT_BIT) != 0;
	if (upper) {
		if (face == Facing::DOWN) return true;
		if (face == Facing::UP && super::shouldRenderFace(level, x, y, z, face)) return true;
		return level->getTile(x, y, z) != id || (level->getData(x, y, z) & TOP_SLOT_BIT) == 0;
	}
	if (face == Facing::UP) return true;
	if (face == Facing::DOWN && super::shouldRenderFace(level, x, y, z, face)) return true;
	return level->getTile(x, y, z) != id || (level->getData(x, y, z) & TOP_SLOT_BIT) != 0;
}

// ======================================================================
// AetherPressurePlateTile
// ======================================================================
AetherPressurePlateTile::AetherPressurePlateTile(int id, int tex, const Material* material, bool mobsOnly)
:	super(id, tex, material), mobsOnly(mobsOnly)
{
	setTicking(true);
	updateDefaultShape();
}

int AetherPressurePlateTile::getTickDelay() { return 20; }
AABB* AetherPressurePlateTile::getAABB(Level* level, int x, int y, int z) { return NULL; }
bool AetherPressurePlateTile::isSolidRender() { return false; }
bool AetherPressurePlateTile::isCubeShaped() { return false; }
bool AetherPressurePlateTile::blocksLight() { return false; }
int AetherPressurePlateTile::getRenderLayer() { return Tile::RENDERLAYER_ALPHATEST; }

bool AetherPressurePlateTile::mayPlace(Level* level, int x, int y, int z, unsigned char face) {
	return level->isSolidBlockingTile(x, y - 1, z);
}
bool AetherPressurePlateTile::mayPlace(Level* level, int x, int y, int z) {
	return level->isSolidBlockingTile(x, y - 1, z);
}

void AetherPressurePlateTile::updateShape(LevelSource* level, int x, int y, int z) {
	bool pressed = (level->getData(x, y, z) & 1) != 0;
	float r = 1 / 16.0f;
	setShape(r, 0, r, 1 - r, pressed ? 1 / 32.0f : 1 / 16.0f, 1 - r);
}

void AetherPressurePlateTile::updateDefaultShape() {
	float r = 1 / 16.0f;
	setShape(r, 0, r, 1 - r, 1 / 16.0f, 1 - r);
}

void AetherPressurePlateTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	if (!level->isSolidBlockingTile(x, y - 1, z)) {
		spawnResources(level, x, y, z, level->getData(x, y, z));
		level->setTile(x, y, z, 0);
	}
}

void AetherPressurePlateTile::tick(Level* level, int x, int y, int z, Random* random) {
	if (level->isClientSide) return;
	if (level->getData(x, y, z) & 1) checkPressed(level, x, y, z);
}

void AetherPressurePlateTile::entityInside(Level* level, int x, int y, int z, Entity* entity) {
	if (level->isClientSide) return;
	if ((level->getData(x, y, z) & 1) == 0) checkPressed(level, x, y, z);
}

void AetherPressurePlateTile::stepOn(Level* level, int x, int y, int z, Entity* entity) {
	if (level->isClientSide) return;
	if ((level->getData(x, y, z) & 1) == 0) checkPressed(level, x, y, z);
}

void AetherPressurePlateTile::checkPressed(Level* level, int x, int y, int z) {
	bool wasPressed = (level->getData(x, y, z) & 1) != 0;
	bool pressed = false;
	AABB box(x + 0.125f, y, z + 0.125f, x + 0.875f, y + 0.25f, z + 0.875f);
	EntityList& list = level->getEntities(NULL, box);
	for (unsigned int i = 0; i < list.size(); ++i) {
		Entity* e = list[i];
		if (!e || e->removed) continue;
		if (mobsOnly && !e->isMob()) continue;
		pressed = true;
		break;
	}
	if (pressed != wasPressed) {
		level->setData(x, y, z, pressed ? 1 : 0);
		level->updateNeighborsAt(x, y, z, id);
		level->updateNeighborsAt(x, y - 1, z, id);
		level->playSound(x + 0.5f, y + 0.1f, z + 0.5f, "random.click", 0.3f, pressed ? 0.6f : 0.5f);
	}
	if (pressed) level->addToTickNextTick(x, y, z, id, getTickDelay());
}

bool AetherPressurePlateTile::isSignalSource() { return true; }
bool AetherPressurePlateTile::getSignal(LevelSource* level, int x, int y, int z, int dir) {
	return (level->getData(x, y, z) & 1) != 0;
}
bool AetherPressurePlateTile::getDirectSignal(Level* level, int x, int y, int z, int dir) {
	return (level->getData(x, y, z) & 1) != 0 && dir == Facing::UP;
}

// ======================================================================
// AetherButtonTile
// ======================================================================
AetherButtonTile::AetherButtonTile(int id, int tex)
:	super(id, tex, Material::decoration)
{
	setTicking(true);
	updateDefaultShape();
}

int AetherButtonTile::getTickDelay() { return 20; }
AABB* AetherButtonTile::getAABB(Level* level, int x, int y, int z) { return NULL; }
bool AetherButtonTile::isSolidRender() { return false; }
bool AetherButtonTile::isCubeShaped() { return false; }
bool AetherButtonTile::blocksLight() { return false; }
int AetherButtonTile::getRenderLayer() { return Tile::RENDERLAYER_ALPHATEST; }

bool AetherButtonTile::mayPlace(Level* level, int x, int y, int z, unsigned char face) {
	if (face == Facing::NORTH && level->isSolidBlockingTile(x, y, z + 1)) return true;
	if (face == Facing::SOUTH && level->isSolidBlockingTile(x, y, z - 1)) return true;
	if (face == Facing::WEST && level->isSolidBlockingTile(x + 1, y, z)) return true;
	if (face == Facing::EAST && level->isSolidBlockingTile(x - 1, y, z)) return true;
	return mayPlace(level, x, y, z);
}

bool AetherButtonTile::mayPlace(Level* level, int x, int y, int z) {
	return level->isSolidBlockingTile(x - 1, y, z) || level->isSolidBlockingTile(x + 1, y, z)
		|| level->isSolidBlockingTile(x, y, z - 1) || level->isSolidBlockingTile(x, y, z + 1);
}

int AetherButtonTile::getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue) {
	// data 1..4 = attached to west/east/north/south wall (same convention as torches)
	if (face == Facing::EAST && level->isSolidBlockingTile(x - 1, y, z)) return 1;
	if (face == Facing::WEST && level->isSolidBlockingTile(x + 1, y, z)) return 2;
	if (face == Facing::SOUTH && level->isSolidBlockingTile(x, y, z - 1)) return 3;
	if (face == Facing::NORTH && level->isSolidBlockingTile(x, y, z + 1)) return 4;
	if (level->isSolidBlockingTile(x - 1, y, z)) return 1;
	if (level->isSolidBlockingTile(x + 1, y, z)) return 2;
	if (level->isSolidBlockingTile(x, y, z - 1)) return 3;
	return 4;
}

void AetherButtonTile::setShapeForData(int data) {
	int dir = data & 7;
	bool pressed = (data & 8) != 0;
	float d = pressed ? 1 / 16.0f : 2 / 16.0f;
	float w = 3 / 16.0f, h = 4 / 16.0f;
	if (dir == 1) setShape(0, 0.5f - h / 2, 0.5f - w, d, 0.5f + h / 2, 0.5f + w);
	else if (dir == 2) setShape(1 - d, 0.5f - h / 2, 0.5f - w, 1, 0.5f + h / 2, 0.5f + w);
	else if (dir == 3) setShape(0.5f - w, 0.5f - h / 2, 0, 0.5f + w, 0.5f + h / 2, d);
	else setShape(0.5f - w, 0.5f - h / 2, 1 - d, 0.5f + w, 0.5f + h / 2, 1);
}

void AetherButtonTile::updateShape(LevelSource* level, int x, int y, int z) {
	setShapeForData(level->getData(x, y, z));
}

void AetherButtonTile::updateDefaultShape() {
	setShapeForData(1);
}

void AetherButtonTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	int dir = level->getData(x, y, z) & 7;
	bool ok = (dir == 1 && level->isSolidBlockingTile(x - 1, y, z))
		|| (dir == 2 && level->isSolidBlockingTile(x + 1, y, z))
		|| (dir == 3 && level->isSolidBlockingTile(x, y, z - 1))
		|| (dir == 4 && level->isSolidBlockingTile(x, y, z + 1));
	if (!ok) {
		spawnResources(level, x, y, z, level->getData(x, y, z));
		level->setTile(x, y, z, 0);
	}
}

bool AetherButtonTile::use(Level* level, int x, int y, int z, Player* player) {
	int data = level->getData(x, y, z);
	if (data & 8) return true;
	level->setData(x, y, z, data | 8);
	level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.click", 0.3f, 0.6f);
	level->updateNeighborsAt(x, y, z, id);
	int dir = data & 7;
	if (dir == 1) level->updateNeighborsAt(x - 1, y, z, id);
	else if (dir == 2) level->updateNeighborsAt(x + 1, y, z, id);
	else if (dir == 3) level->updateNeighborsAt(x, y, z - 1, id);
	else level->updateNeighborsAt(x, y, z + 1, id);
	level->addToTickNextTick(x, y, z, id, getTickDelay());
	return true;
}

void AetherButtonTile::attack(Level* level, int x, int y, int z, Player* player) {
	use(level, x, y, z, player);
}

void AetherButtonTile::tick(Level* level, int x, int y, int z, Random* random) {
	if (level->isClientSide) return;
	int data = level->getData(x, y, z);
	if (data & 8) {
		level->setData(x, y, z, data & 7);
		level->updateNeighborsAt(x, y, z, id);
		int dir = data & 7;
		if (dir == 1) level->updateNeighborsAt(x - 1, y, z, id);
		else if (dir == 2) level->updateNeighborsAt(x + 1, y, z, id);
		else if (dir == 3) level->updateNeighborsAt(x, y, z - 1, id);
		else level->updateNeighborsAt(x, y, z + 1, id);
		level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.click", 0.3f, 0.5f);
	}
}

bool AetherButtonTile::isSignalSource() { return true; }
bool AetherButtonTile::getSignal(LevelSource* level, int x, int y, int z, int dir) {
	return (level->getData(x, y, z) & 8) != 0;
}
bool AetherButtonTile::getDirectSignal(Level* level, int x, int y, int z, int dir) {
	int data = level->getData(x, y, z);
	if ((data & 8) == 0) return false;
	int d = data & 7;
	// powers the block it is attached to
	return (d == 1 && dir == Facing::EAST) || (d == 2 && dir == Facing::WEST)
		|| (d == 3 && dir == Facing::SOUTH) || (d == 4 && dir == Facing::NORTH);
}

// ======================================================================
// Quicksoil glass / panes
// ======================================================================
QuicksoilGlassTile::QuicksoilGlassTile(int id, int tex)
:	super(id, tex, Material::glass, false)
{
	friction = 1.1f;
}

int QuicksoilGlassTile::getRenderLayer() { return Tile::RENDERLAYER_BLEND; }
int QuicksoilGlassTile::getResourceCount(Random* random) { return 0; }
void QuicksoilGlassTile::stepOn(Level* level, int x, int y, int z, Entity* entity) { quicksoilBoost(entity); }
void QuicksoilGlassTile::entityInside(Level* level, int x, int y, int z, Entity* entity) {}

QuicksoilGlassPaneTile::QuicksoilGlassPaneTile(int id, int tex, int edgeTex)
:	super(id, tex, edgeTex, Material::glass, false)
{
	friction = 1.1f;
}

void QuicksoilGlassPaneTile::entityInside(Level* level, int x, int y, int z, Entity* entity) {}

// ======================================================================
// Cloudwool / carpet / banner
// ======================================================================
static int clothColor(int data) {
	// data is a dye color index (0..15, DyePowderItem order). 15 = white.
	return DyePowderItem::COLOR_RGB[data & 15];
}

CloudwoolTile::CloudwoolTile(int id, int tex)
:	super(id, tex, Material::cloth)
{
}

int CloudwoolTile::getColor(int data) { return clothColor(data); }
int CloudwoolTile::getColor(LevelSource* level, int x, int y, int z) { return clothColor(level->getData(x, y, z)); }
int CloudwoolTile::getSpawnResourcesAuxValue(int data) { return data; }

void CloudwoolTile::fallOn(Level* level, int x, int y, int z, Entity* entity, float fallDistance) {
	if (entity) entity->fallDistance = Mth::Max(0.0f, fallDistance - 6.0f);
}

void CloudwoolTile::entityInside(Level* level, int x, int y, int z, Entity* entity) {}

CloudwoolCarpetTile::CloudwoolCarpetTile(int id, int tex)
:	super(id, tex, Material::cloth)
{
	setShape(0, 0, 0, 1, 1 / 16.0f, 1);
}

int CloudwoolCarpetTile::getColor(int data) { return clothColor(data); }
int CloudwoolCarpetTile::getColor(LevelSource* level, int x, int y, int z) { return clothColor(level->getData(x, y, z)); }
int CloudwoolCarpetTile::getSpawnResourcesAuxValue(int data) { return data; }
bool CloudwoolCarpetTile::isSolidRender() { return false; }
bool CloudwoolCarpetTile::isCubeShaped() { return false; }
bool CloudwoolCarpetTile::blocksLight() { return false; }
int CloudwoolCarpetTile::getRenderLayer() { return Tile::RENDERLAYER_ALPHATEST; }
AABB* CloudwoolCarpetTile::getAABB(Level* level, int x, int y, int z) { return NULL; }
bool CloudwoolCarpetTile::mayPlace(Level* level, int x, int y, int z, unsigned char face) { return level->isSolidBlockingTile(x, y - 1, z); }
bool CloudwoolCarpetTile::mayPlace(Level* level, int x, int y, int z) { return level->isSolidBlockingTile(x, y - 1, z); }

void CloudwoolCarpetTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	if (!level->isSolidBlockingTile(x, y - 1, z)) {
		spawnResources(level, x, y, z, level->getData(x, y, z));
		level->setTile(x, y, z, 0);
	}
}

AetherBannerTile::AetherBannerTile(int id, int tex)
:	super(id, tex, Material::cloth)
{
	setShape(0.25f, 0, 0.25f, 0.75f, 1, 0.75f);
}

int AetherBannerTile::getColor(int data) { return clothColor(data); }
int AetherBannerTile::getColor(LevelSource* level, int x, int y, int z) { return clothColor(level->getData(x, y, z)); }
int AetherBannerTile::getSpawnResourcesAuxValue(int data) { return data; }
bool AetherBannerTile::isSolidRender() { return false; }
bool AetherBannerTile::isCubeShaped() { return false; }
bool AetherBannerTile::blocksLight() { return false; }
int AetherBannerTile::getRenderLayer() { return Tile::RENDERLAYER_ALPHATEST; }
int AetherBannerTile::getRenderShape() { return Tile::SHAPE_CROSS_TEXTURE; }
AABB* AetherBannerTile::getAABB(Level* level, int x, int y, int z) { return NULL; }
bool AetherBannerTile::mayPlace(Level* level, int x, int y, int z, unsigned char face) { return level->isSolidBlockingTile(x, y - 1, z); }
bool AetherBannerTile::mayPlace(Level* level, int x, int y, int z) { return level->isSolidBlockingTile(x, y - 1, z); }

void AetherBannerTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	if (!level->isSolidBlockingTile(x, y - 1, z)) {
		spawnResources(level, x, y, z, level->getData(x, y, z));
		level->setTile(x, y, z, 0);
	}
}

// ======================================================================
// Skyroot door / trapdoor / fence
// ======================================================================
SkyrootDoorTile::SkyrootDoorTile(int id)
:	super(id, Material::wood)
{
	tex = AetherTex::SKYROOT_DOOR_LOWER;
}

int SkyrootDoorTile::getResource(int data, Random* random) {
	if ((data & C_IS_UPPER_MASK) != 0) return 0;
	return Aether::skyrootDoorItem->id;
}

void SkyrootDoorTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	// DoorTile pops the vanilla door item for the upper half when the lower
	// half disappears; route that through our own item instead.
	int data = level->getData(x, y, z);
	if ((data & C_IS_UPPER_MASK) != 0 && level->getTile(x, y - 1, z) != id) {
		level->setTile(x, y, z, 0);
		popResource(level, x, y, z, ItemInstance(Aether::skyrootDoorItem, 1, 0));
		return;
	}
	super::neighborChanged(level, x, y, z, type);
}

SkyrootTrapdoorTile::SkyrootTrapdoorTile(int id)
:	super(id, Material::wood)
{
	tex = AetherTex::SKYROOT_TRAPDOOR;
}

SkyrootFenceTile::SkyrootFenceTile(int id, int tex)
:	super(id, tex)
{
}

// ======================================================================
// AetherFurnaceTile (Altar / Freezer / Incubator)
// ======================================================================
bool AetherFurnaceTile::noDrop = false;

AetherFurnaceTile::AetherFurnaceTile(int id, Kind kind, bool lit, int topTex, int sideTex, int litTex)
:	super(id, sideTex, Material::stone), kind(kind), lit(lit), topTex(topTex), sideTex(sideTex), litTex(litTex)
{
	if (lit) setLightEmission(13.0f / 16.0f);
}

int AetherFurnaceTile::getTexture(int face) {
	if (face <= 1) return topTex;
	if (face == 3) return lit ? litTex : sideTex;
	return sideTex;
}

int AetherFurnaceTile::getTexture(LevelSource* level, int x, int y, int z, int face) {
	if (face <= 1) return topTex;
	int lockDir = level->getData(x, y, z);
	if (face != lockDir) return sideTex;
	return lit ? litTex : sideTex;
}

int AetherFurnaceTile::getResource(int data, Random* random) {
	return tileFor(kind, false)->id;
}

/*static*/ Tile* AetherFurnaceTile::tileFor(Kind kind, bool lit) {
	switch (kind) {
	case AltarKind:     return lit ? Aether::altarLit : Aether::altar;
	case FreezerKind:   return lit ? Aether::freezerLit : Aether::freezer;
	default:            return lit ? Aether::incubatorLit : Aether::incubator;
	}
}

/*static*/ int AetherFurnaceTile::tileEntityTypeFor(Kind kind) {
	switch (kind) {
	case AltarKind:     return TileEntityType::AetherAltar;
	case FreezerKind:   return TileEntityType::AetherFreezer;
	default:            return TileEntityType::AetherIncubator;
	}
}

TileEntity* AetherFurnaceTile::newTileEntity() {
	return TileEntityFactory::createTileEntity(tileEntityTypeFor(kind));
}

bool AetherFurnaceTile::use(Level* level, int x, int y, int z, Player* player) {
	if (level->isClientSide) return true;
	AetherFurnaceTileEntity* te = dynamic_cast<AetherFurnaceTileEntity*>(level->getTileEntity(x, y, z));
	if (te != NULL) player->openFurnace(te);
	return true;
}

/*static*/ void AetherFurnaceTile::setLit(Kind kind, bool lit, Level* level, int x, int y, int z) {
	int data = level->getData(x, y, z);
	TileEntity* te = level->getTileEntity(x, y, z);
	noDrop = true;
	level->setTile(x, y, z, tileFor(kind, lit)->id);
	noDrop = false;
	level->setData(x, y, z, data);
	if (te != NULL) {
		te->clearRemoved();
		level->setTileEntity(x, y, z, te);
	}
}

void AetherFurnaceTile::setPlacedBy(Level* level, int x, int y, int z, Mob* by) {
	int dir = (Mth::floor(by->yRot * 4 / 360.0f + 0.5f)) & 3;
	if (dir == 0) level->setData(x, y, z, Facing::NORTH);
	if (dir == 1) level->setData(x, y, z, Facing::EAST);
	if (dir == 2) level->setData(x, y, z, Facing::SOUTH);
	if (dir == 3) level->setData(x, y, z, Facing::WEST);
}

void AetherFurnaceTile::onRemove(Level* level, int x, int y, int z) {
	if (!noDrop && !level->isClientSide) {
		AetherFurnaceTileEntity* container = dynamic_cast<AetherFurnaceTileEntity*>(level->getTileEntity(x, y, z));
		if (container != NULL) {
			for (int i = 0; i < container->getContainerSize(); i++) {
				ItemInstance* item = container->getItem(i);
				if (item && !item->isNull()) {
					popResource(level, x, y, z, ItemInstance(item->id, item->count, item->getAuxValue()));
					item->setNull();
				}
			}
		}
	}
	super::onRemove(level, x, y, z);
}

void AetherFurnaceTile::animateTick(Level* level, int xt, int yt, int zt, Random* random) {
	if (!lit) return;
	int dir = level->getData(xt, yt, zt);
	float x = xt + 0.5f;
	float y = yt + 0.2f + random->nextFloat() * 0.4f;
	float z = zt + 0.5f;
	float r = 0.52f;
	float ss = random->nextFloat() * 0.6f - 0.3f;
	const char* p = (kind == FreezerKind) ? "snowballpoof" : (kind == AltarKind ? "crit" : "flame");
	ParticleType::Id pid = (kind == FreezerKind) ? ParticleType::snowballpoof : (kind == AltarKind ? ParticleType::crit : ParticleType::flame);
	(void)p;
	if (dir == 4) level->addParticle(pid, x - r, y, z + ss, 0, 0, 0);
	else if (dir == 5) level->addParticle(pid, x + r, y, z + ss, 0, 0, 0);
	else if (dir == 2) level->addParticle(pid, x + ss, y, z - r, 0, 0, 0);
	else if (dir == 3) level->addParticle(pid, x + ss, y, z + r, 0, 0, 0);
}

// ======================================================================
// SunAltarTile
// ======================================================================
SunAltarTile::SunAltarTile(int id, int topTex, int sideTex)
:	super(id, sideTex, Material::stone)
{
	setTopBottom(topTex, sideTex);
	setLightEmission(8.0f / 16.0f);
}

bool SunAltarTile::use(Level* level, int x, int y, int z, Player* player) {
	if (!Aether::isAetherLevel(level)) {
		if (player) player->displayClientMessage("The Sun Altar only answers in the Aether");
		return true;
	}
	if (level->isClientSide) return true;
	// Cycle: sneaking -> midnight, otherwise advance to the next quarter of the day
	long t = level->getTime();
	long day = Level::TICKS_PER_DAY;
	long inDay = t % day;
	long target;
	if (player->isSneaking()) target = t - inDay + day / 2;
	else {
		long quarter = day / 4;
		target = t - inDay + ((inDay / quarter) + 1) * quarter;
	}
	level->setTime(target);
	level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.levelup", 1.0f, 1.0f);
	level->addParticle(PARTICLETYPE(hugeexplosion), x + 0.5f, y + 1.2f, z + 0.5f, 0, 0, 0);
	return true;
}

// ======================================================================
// SkyrootWorkbenchTile
// ======================================================================
SkyrootWorkbenchTile::SkyrootWorkbenchTile(int id, int topTex, int sideTex, int frontTex)
:	super(id, sideTex, Material::wood), topTex(topTex), sideTex(sideTex), frontTex(frontTex)
{
}

int SkyrootWorkbenchTile::getTexture(int face) {
	if (face == 1) return topTex;
	if (face == 0) return AetherTex::SKYROOT_PLANKS;
	if (face == 2 || face == 4) return frontTex;
	return sideTex;
}

bool SkyrootWorkbenchTile::use(Level* level, int x, int y, int z, Player* player) {
	player->startCrafting(x, y, z, Recipe::SIZE_3X3);
	return true;
}

// ======================================================================
// SkyrootBedTile
// ======================================================================
SkyrootBedTile::SkyrootBedTile(int id, int tex)
:	super(id)
{
	this->tex = tex; // foot top texture
}

int SkyrootBedTile::getTexture(int face, int data) {
	if (face == Facing::DOWN) return AetherTex::SKYROOT_PLANKS;
	int direction = getDirection(data);
	int tileFacing = Direction::RELATIVE_DIRECTION_FACING[direction][face];
	if (isHeadPiece(data)) {
		if (tileFacing == Facing::NORTH) return AetherTex::SKYROOT_BED_END;
		if (tileFacing == Facing::EAST || tileFacing == Facing::WEST) return AetherTex::SKYROOT_BED_SIDE;
		return AetherTex::SKYROOT_BED_TOP_HEAD;
	}
	if (tileFacing == Facing::SOUTH) return AetherTex::SKYROOT_BED_END;
	if (tileFacing == Facing::EAST || tileFacing == Facing::WEST) return AetherTex::SKYROOT_BED_SIDE;
	return AetherTex::SKYROOT_BED_TOP_FOOT;
}

int SkyrootBedTile::getResource(int data, Random* random) {
	if (isHeadPiece(data)) return 0;
	return Aether::skyrootBedItem->id;
}

void SkyrootBedTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	int data = level->getData(x, y, z);
	int direction = getDirection(data);
	if (isHeadPiece(data)) {
		if (level->getTile(x - HEAD_DIRECTION_OFFSETS[direction][0], y, z - HEAD_DIRECTION_OFFSETS[direction][1]) != id)
			level->setTile(x, y, z, 0);
	} else {
		if (level->getTile(x + HEAD_DIRECTION_OFFSETS[direction][0], y, z + HEAD_DIRECTION_OFFSETS[direction][1]) != id) {
			level->setTile(x, y, z, 0);
			if (!level->isClientSide) popResource(level, x, y, z, ItemInstance(Aether::skyrootBedItem, 1, 0));
		}
	}
}

bool SkyrootBedTile::use(Level* level, int x, int y, int z, Player* player) {
	// Player::startSleepInBed refuses beds in dimensions where isNaturalDimension()
	// is true (overworld). The Aether reports false there, so the vanilla bed
	// logic works as-is; BedTile::use also needs mayRespawn(), which the
	// Aether dimension grants. Skyroot beds are therefore just beds that also
	// exist in the Aether creative/survival palette.
	return super::use(level, x, y, z, player);
}

// ======================================================================
// SkyrootChestTile
// ======================================================================
SkyrootChestTile::SkyrootChestTile(int id, int tex)
:	super(id)
{
	this->tex = tex; // side texture; top = SKYROOT_CHEST_TOP, front = SKYROOT_CHEST_FRONT
}

int SkyrootChestTile::getTexture(int face) {
	if (face <= 1) return AetherTex::SKYROOT_CHEST_TOP;
	if (face == 3) return AetherTex::SKYROOT_CHEST_FRONT;
	return tex;
}

int SkyrootChestTile::getTexture(LevelSource* level, int x, int y, int z, int face) {
	if (face <= 1) return AetherTex::SKYROOT_CHEST_TOP;
	int lockDir = level->getData(x, y, z);
	if (lockDir < 2 || lockDir > 5) lockDir = 3;
	return (face == lockDir) ? AetherTex::SKYROOT_CHEST_FRONT : tex;
}

int SkyrootChestTile::getResource(int data, Random* random) {
	return id;
}

// ======================================================================
// ChestMimicTile
// ======================================================================
ChestMimicTile::ChestMimicTile(int id, int tex)
:	super(id, tex, Material::wood)
{
	const float m = 0.025f;
	setShape(m, 0, m, 1 - m, 1 - m - m, 1 - m);
}

int ChestMimicTile::getTexture(int face) {
	if (face <= 1) return AetherTex::SKYROOT_CHEST_TOP;
	if (face == 3) return AetherTex::SKYROOT_CHEST_FRONT;
	return tex;
}

int ChestMimicTile::getTexture(LevelSource* level, int x, int y, int z, int face) {
	if (face <= 1) return AetherTex::SKYROOT_CHEST_TOP;
	int lockDir = level->getData(x, y, z);
	if (lockDir < 2 || lockDir > 5) lockDir = 3;
	return (face == lockDir) ? AetherTex::SKYROOT_CHEST_FRONT : tex;
}

bool ChestMimicTile::isSolidRender() { return false; }
bool ChestMimicTile::isCubeShaped() { return false; }
int ChestMimicTile::getResourceCount(Random* random) { return 0; }

void ChestMimicTile::setPlacedBy(Level* level, int x, int y, int z, Mob* by) {
	int dir = (Mth::floor(by->yRot * 4 / 360.0f + 0.5f)) & 3;
	if (dir == 0) level->setData(x, y, z, Facing::NORTH);
	if (dir == 1) level->setData(x, y, z, Facing::EAST);
	if (dir == 2) level->setData(x, y, z, Facing::SOUTH);
	if (dir == 3) level->setData(x, y, z, Facing::WEST);
}

bool ChestMimicTile::use(Level* level, int x, int y, int z, Player* player) {
	awaken(level, x, y, z);
	return true;
}

void ChestMimicTile::attack(Level* level, int x, int y, int z, Player* player) {
	awaken(level, x, y, z);
}

void ChestMimicTile::awaken(Level* level, int x, int y, int z) {
	if (level->isClientSide) return;
	level->setTile(x, y, z, 0);
	level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.chestopen", 1.0f, 0.6f);
	Aether::spawnMob(level, MobTypes::Mimic, x + 0.5f, (float)y, z + 0.5f);
}

// ======================================================================
// AmbrosiumTorchTile
// ======================================================================
AmbrosiumTorchTile::AmbrosiumTorchTile(int id, int tex)
:	super(id, tex)
{
}

// ======================================================================
// AetherPortalTile
// ======================================================================
AetherPortalTile::AetherPortalTile(int id, int tex)
:	super(id, tex)
{
}

/*static*/ bool AetherPortalTile::isFrame(int tileId) {
	return tileId == Tile::lightGem->id;
}

int AetherPortalTile::getColor(int data) {
	return 0xffffff;
}

void AetherPortalTile::neighborChanged(Level* level, int x, int y, int z, int type) {
	int data = level->getData(x, y, z);
	bool broken = false;
	if (data == 1) {
		int left = level->getTile(x - 1, y, z), right = level->getTile(x + 1, y, z);
		int down = level->getTile(x, y - 1, z), up = level->getTile(x, y + 1, z);
		if ((!isFrame(left) && left != id) || (!isFrame(right) && right != id) || (!isFrame(down) && down != id) || (!isFrame(up) && up != id)) broken = true;
	} else {
		int back = level->getTile(x, y, z - 1), front = level->getTile(x, y, z + 1);
		int down = level->getTile(x, y - 1, z), up = level->getTile(x, y + 1, z);
		if ((!isFrame(back) && back != id) || (!isFrame(front) && front != id) || (!isFrame(down) && down != id) || (!isFrame(up) && up != id)) broken = true;
	}
	if (broken) level->setTile(x, y, z, 0);
}

void AetherPortalTile::entityInside(Level* level, int x, int y, int z, Entity* entity) {
	if (entity != NULL) {
		entity->inPortal = true;
		entity->inPortalDim = Dimension::AETHER;
	}
}

void AetherPortalTile::animateTick(Level* level, int x, int y, int z, Random* random) {
	for (int i = 0; i < 3; i++) {
		float px = (float)x + random->nextFloat();
		float py = (float)y + random->nextFloat();
		float pz = (float)z + random->nextFloat();
		level->addParticle(ParticleType::bubble, px, py, pz, (random->nextFloat() - 0.5f) * 0.1f, 0.05f + random->nextFloat() * 0.1f, (random->nextFloat() - 0.5f) * 0.1f);
	}
}

/*static*/ bool AetherPortalTile::trySpawnPortal(Level* level, int x, int y, int z) {
	if (!level || !Aether::aetherPortal || !Tile::lightGem) return false;
	if (!Aether::isAvailable(level)) return false;

	int portId = Aether::aetherPortal->id;
	int waterId = Tile::water->id, calmWaterId = Tile::calmWater->id;

	auto isSpace = [&](int bx, int by, int bz) -> bool {
		int t = level->getTile(bx, by, bz);
		return (t == 0 || t == portId || t == waterId || t == calmWaterId);
	};

	// X-aligned frame (2 wide along X, 3 high)
	for (int cx = x - 1; cx <= x; cx++) {
		for (int cy = y - 2; cy <= y; cy++) {
			bool valid = true;
			if (!isFrame(level->getTile(cx, cy - 1, z)) || !isFrame(level->getTile(cx + 1, cy - 1, z))) valid = false;
			if (valid && (!isFrame(level->getTile(cx, cy + 3, z)) || !isFrame(level->getTile(cx + 1, cy + 3, z)))) valid = false;
			for (int j = 0; valid && j < 3; j++) if (!isFrame(level->getTile(cx - 1, cy + j, z))) valid = false;
			for (int j = 0; valid && j < 3; j++) if (!isFrame(level->getTile(cx + 2, cy + j, z))) valid = false;
			for (int i = 0; valid && i < 2; i++) for (int j = 0; valid && j < 3; j++) if (!isSpace(cx + i, cy + j, z)) valid = false;
			if (valid) {
				bool old = level->noNeighborUpdate;
				level->noNeighborUpdate = true;
				for (int i = 0; i < 2; i++) for (int j = 0; j < 3; j++) level->setTileAndData(cx + i, cy + j, z, portId, 1);
				level->noNeighborUpdate = old;
				return true;
			}
		}
	}
	// Z-aligned frame
	for (int cz = z - 1; cz <= z; cz++) {
		for (int cy = y - 2; cy <= y; cy++) {
			bool valid = true;
			if (!isFrame(level->getTile(x, cy - 1, cz)) || !isFrame(level->getTile(x, cy - 1, cz + 1))) valid = false;
			if (valid && (!isFrame(level->getTile(x, cy + 3, cz)) || !isFrame(level->getTile(x, cy + 3, cz + 1)))) valid = false;
			for (int j = 0; valid && j < 3; j++) if (!isFrame(level->getTile(x, cy + j, cz - 1))) valid = false;
			for (int j = 0; valid && j < 3; j++) if (!isFrame(level->getTile(x, cy + j, cz + 2))) valid = false;
			for (int k = 0; valid && k < 2; k++) for (int j = 0; valid && j < 3; j++) if (!isSpace(x, cy + j, cz + k)) valid = false;
			if (valid) {
				bool old = level->noNeighborUpdate;
				level->noNeighborUpdate = true;
				for (int k = 0; k < 2; k++) for (int j = 0; j < 3; j++) level->setTileAndData(x, cy + j, cz + k, portId, 2);
				level->noNeighborUpdate = old;
				return true;
			}
		}
	}
	return false;
}

// ======================================================================
// AetherSignTile
// ======================================================================
AetherSignTile::AetherSignTile(int id, bool onGround)
:	super(id, TileEntityType::Sign, onGround)
{
	tex = AetherTex::SKYROOT_PLANKS;
}

int AetherSignTile::getResource(int data, Random* random) {
	return Aether::aetherSignItem->id;
}
