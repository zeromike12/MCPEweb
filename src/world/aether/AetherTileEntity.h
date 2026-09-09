#ifndef NET_MINECRAFT_WORLD_AETHER__AetherTileEntity_H__
#define NET_MINECRAFT_WORLD_AETHER__AetherTileEntity_H__

//
// Tile entity shared by the three Aether workstations. They behave like a
// furnace (ingredient / fuel / result slots, same screen) but each has its
// own fuel and its own recipe table:
//
//   Altar      fuel Ambrosium Shard    - enchants: gravitite ore -> enchanted
//                                        gravitite, blueberry -> enchanted
//                                        blueberry, repairs damaged tools.
//   Freezer    fuel Icestone           - water bucket -> ice, lava bucket ->
//                                        obsidian, aerclouds -> colder variant.
//   Incubator  fuel Ambrosium Torch    - moa egg -> a tame, rideable Moa.
//

#include "../level/tile/entity/FurnaceTileEntity.h"

class AetherFurnaceTileEntity : public FurnaceTileEntity {
	typedef FurnaceTileEntity super;
public:
	AetherFurnaceTileEntity(int tileEntityType);

	std::string getName() const;

	int getFuelDuration(const ItemInstance& itemInstance) const;
	ItemInstance getRecipeResult(int itemId) const;
	bool isIngredient(int itemId) const;
	std::string getFuelHint() const;

	// Static helpers usable without an instance (recipe book, tooltips).
	static int fuelDurationFor(int tileEntityType, const ItemInstance& item);
	static ItemInstance recipeResultFor(int tileEntityType, int itemId);

	void burn();
protected:
	void updateLitTile(bool lit);
};

#endif /*NET_MINECRAFT_WORLD_AETHER__AetherTileEntity_H__*/
