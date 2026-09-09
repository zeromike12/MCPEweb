#include "AetherTileEntity.h"
#include "Aether.h"
#include "AetherTiles.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"
#include "../item/Item.h"
#include "../item/ItemInstance.h"

static AetherFurnaceTile::Kind kindFor(int type) {
	if (type == TileEntityType::AetherAltar) return AetherFurnaceTile::AltarKind;
	if (type == TileEntityType::AetherFreezer) return AetherFurnaceTile::FreezerKind;
	return AetherFurnaceTile::IncubatorKind;
}

AetherFurnaceTileEntity::AetherFurnaceTileEntity(int tileEntityType)
:	super(tileEntityType)
{
}

std::string AetherFurnaceTileEntity::getName() const {
	switch (type) {
	case TileEntityType::AetherAltar: return "Altar";
	case TileEntityType::AetherFreezer: return "Freezer";
	default: return "Incubator";
	}
}

/*static*/ int AetherFurnaceTileEntity::fuelDurationFor(int tileEntityType, const ItemInstance& item) {
	if (item.isNull() || !item.getItem()) return 0;
	int id = item.getItem()->id;
	const int BURN = 10 * 20;
	switch (tileEntityType) {
	case TileEntityType::AetherAltar:
		if (Aether::ambrosiumShard && id == Aether::ambrosiumShard->id) return BURN * 2;
		if (Aether::ambrosiumBlock && id == Aether::ambrosiumBlock->id) return BURN * 20;
		return 0;
	case TileEntityType::AetherFreezer:
		if (Aether::icestone && id == Aether::icestone->id) return BURN * 3;
		return 0;
	default:
		if (Aether::ambrosiumTorch && id == Aether::ambrosiumTorch->id) return BURN * 5;
		return 0;
	}
}

/*static*/ ItemInstance AetherFurnaceTileEntity::recipeResultFor(int tileEntityType, int itemId) {
	switch (tileEntityType) {
	case TileEntityType::AetherAltar:
		if (Aether::gravititeOre && itemId == Aether::gravititeOre->id) return ItemInstance(Aether::enchantedGravitite, 1, 0);
		if (Aether::blueberry && itemId == Aether::blueberry->id) return ItemInstance(Aether::enchantedBlueberry, 1, 0);
		if (Aether::zaniteGem && itemId == Aether::zaniteGem->id) return ItemInstance(Aether::gravititePlate, 1, 0); // refine
		if (Aether::quicksoil && itemId == Aether::quicksoil->id) return ItemInstance(Aether::quicksoilGlass, 1, 0);
		if (Aether::holystone && itemId == Aether::holystone->id) return ItemInstance(Aether::holystoneBricks, 1, 0);
		// Repair: any damageable Aether tool comes out fully repaired
		if (Aether::isSkyrootTool(itemId) || Aether::isZaniteTool(itemId) || Aether::isGravititeTool(itemId)) {
			return ItemInstance(itemId, 1, 0);
		}
		return ItemInstance();
	case TileEntityType::AetherFreezer:
		if (Item::bucket_water && itemId == Item::bucket_water->id) return ItemInstance(Tile::ice, 1, 0);
		if (Item::bucket_lava && itemId == Item::bucket_lava->id) return ItemInstance(Tile::obsidian, 1, 0);
		if (Tile::water && itemId == Tile::water->id) return ItemInstance(Tile::ice, 1, 0);
		if (Aether::aercloudCold && itemId == Aether::aercloudCold->id) return ItemInstance(Aether::aercloudBlue, 1, 0);
		if (Aether::aercloudBlue && itemId == Aether::aercloudBlue->id) return ItemInstance(Aether::aercloudPurple, 1, 0);
		if (Aether::aercloudGolden && itemId == Aether::aercloudGolden->id) return ItemInstance(Aether::aercloudGreen, 1, 0);
		if (Aether::aercloudPurple && itemId == Aether::aercloudPurple->id) return ItemInstance(Aether::aercloudStorm, 1, 0);
		if (Aether::aerogel && Tile::obsidian && itemId == Tile::obsidian->id) return ItemInstance(Aether::aerogel, 1, 0);
		return ItemInstance();
	default:
		if (Aether::moaEgg && itemId == Aether::moaEgg->id) return ItemInstance(Item::mobEgg, 1, 14 /* MobTypes::Moa */);
		return ItemInstance();
	}
}

int AetherFurnaceTileEntity::getFuelDuration(const ItemInstance& itemInstance) const {
	return fuelDurationFor(type, itemInstance);
}

ItemInstance AetherFurnaceTileEntity::getRecipeResult(int itemId) const {
	return recipeResultFor(type, itemId);
}

bool AetherFurnaceTileEntity::isIngredient(int itemId) const {
	return !getRecipeResult(itemId).isNull();
}

std::string AetherFurnaceTileEntity::getFuelHint() const {
	switch (type) {
	case TileEntityType::AetherAltar: return "Ambrosium Shard";
	case TileEntityType::AetherFreezer: return "Icestone";
	default: return "Ambrosium Torch";
	}
}

void AetherFurnaceTileEntity::burn() {
	// Altar repairs: keep the original item but reset its damage.
	if (type == TileEntityType::AetherAltar && !items[SLOT_INGREDIENT].isNull()) {
		int id = items[SLOT_INGREDIENT].id;
		if (Aether::isSkyrootTool(id) || Aether::isZaniteTool(id) || Aether::isGravititeTool(id)) {
			if (items[SLOT_RESULT].isNull()) {
				items[SLOT_RESULT] = ItemInstance(id, 1, 0);
				items[SLOT_INGREDIENT].setNull();
			}
			return;
		}
	}
	// Buckets in the freezer leave an empty bucket behind
	if (type == TileEntityType::AetherFreezer && !items[SLOT_INGREDIENT].isNull() && Item::bucket_empty
		&& ((Item::bucket_water && items[SLOT_INGREDIENT].id == Item::bucket_water->id) || (Item::bucket_lava && items[SLOT_INGREDIENT].id == Item::bucket_lava->id))) {
		ItemInstance result = getRecipeResult(items[SLOT_INGREDIENT].id);
		if (items[SLOT_RESULT].isNull()) items[SLOT_RESULT] = result;
		else if (items[SLOT_RESULT].id == result.id) items[SLOT_RESULT].count++;
		items[SLOT_INGREDIENT] = ItemInstance(Item::bucket_empty, 1, 0);
		return;
	}
	super::burn();
}

void AetherFurnaceTileEntity::updateLitTile(bool lit) {
	AetherFurnaceTile::setLit(kindFor(type), lit, level, x, y, z);
}
