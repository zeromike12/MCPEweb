#include "Aether.h"
#include "AetherTex.h"
#include "AetherTiles.h"
#include "AetherItems.h"
#include "AetherTileEntity.h"

#include "../level/Level.h"
#include "../level/LevelSettings.h"
#include "../level/dimension/Dimension.h"
#include "../level/storage/LevelData.h"
#include "../level/tile/Tile.h"
#include "../level/tile/entity/TileEntity.h"
#include "../level/material/Material.h"
#include "../item/Item.h"
#include "../item/ItemCategory.h"
#include "../item/ItemInstance.h"
#include "../item/TileItem.h"
#include "../item/AuxDataTileItem.h"
#include "../item/DyePowderItem.h"
#include "../item/crafting/Recipes.h"
#include "../item/crafting/FurnaceRecipes.h"
#include "../entity/Mob.h"
#include "../entity/player/Player.h"
#include "../entity/player/Inventory.h"
#include "../entity/MobFactory.h"
#include "../entity/EntityTypes.h"
#include "../entity/animal/AetherMobs.h"
#include "../rpg/Rpg.h"
#include "../phys/AABB.h"
#include "../../platform/log.h"

namespace Aether {

// ----------------------------------------------------------------------
// Mode gate
// ----------------------------------------------------------------------
bool isAvailable(const Level* level) {
	if (!level) return false;
	if (Rpg::isEnabled(level)) return false;
	LevelData* data = const_cast<Level*>(level)->getLevelData();
	if (!data) return false;
	int gt = data->getGameType();
	return gt == GameType::Survival || gt == GameType::Creative;
}

bool isAetherLevel(const Level* level) {
	return level && level->dimension && level->dimension->id == Dimension::AETHER;
}

// ----------------------------------------------------------------------
// Tile / item pointers
// ----------------------------------------------------------------------
Tile* aetherGrass = NULL;
Tile* enchantedGrass = NULL;
Tile* aetherDirt = NULL;
Tile* holystone = NULL;
Tile* mossyHolystone = NULL;
Tile* icestone = NULL;
Tile* quicksoil = NULL;
Tile* aercloudCold = NULL;
Tile* aercloudBlue = NULL;
Tile* aercloudGolden = NULL;
Tile* aercloudPurple = NULL;
Tile* aercloudGreen = NULL;
Tile* aercloudStorm = NULL;
Tile* aerogel = NULL;
Tile* skyrootLog = NULL;
Tile* goldenOakLog = NULL;
Tile* skyrootPlanks = NULL;
LeafTile* skyrootLeaves = NULL;
LeafTile* goldenOakLeaves = NULL;
Tile* skyrootSapling = NULL;
Tile* goldenOakSapling = NULL;
Tile* whiteFlower = NULL;
Tile* purpleFlower = NULL;
Tile* berryBush = NULL;
Tile* ambrosiumOre = NULL;
Tile* zaniteOre = NULL;
Tile* gravititeOre = NULL;
Tile* ambrosiumBlock = NULL;
Tile* zaniteBlock = NULL;
Tile* enchantedGravitite = NULL;
Tile* carvedStone = NULL;
Tile* sentryStone = NULL;
Tile* lightSentryStone = NULL;
Tile* trappedSentryStone = NULL;
Tile* angelicStone = NULL;
Tile* lightAngelicStone = NULL;
Tile* hellfireStone = NULL;
Tile* lightHellfireStone = NULL;
Tile* lockedCarved = NULL;
Tile* lockedAngelic = NULL;
Tile* lockedHellfire = NULL;
Tile* pillar = NULL;
Tile* skyrootStairs = NULL;
Tile* skyrootSlab = NULL;
Tile* skyrootSlabDouble = NULL;
Tile* skyrootFence = NULL;
Tile* skyrootFenceGate = NULL;
Tile* skyrootDoor = NULL;
Tile* skyrootTrapdoor = NULL;
Tile* skyrootPressurePlate = NULL;
Tile* skyrootButton = NULL;
Tile* holystoneBricks = NULL;
Tile* holystoneStairs = NULL;
Tile* holystoneBrickStairs = NULL;
Tile* holystoneSlab = NULL;
Tile* holystoneSlabDouble = NULL;
Tile* holystonePressurePlate = NULL;
Tile* quicksoilGlass = NULL;
Tile* quicksoilGlassPane = NULL;
Tile* cloudwool = NULL;
Tile* cloudwoolCarpet = NULL;
Tile* aetherBanner = NULL;
Tile* aetherSign = NULL;
Tile* aetherWallSign = NULL;
Tile* altar = NULL;
Tile* altarLit = NULL;
Tile* freezer = NULL;
Tile* freezerLit = NULL;
Tile* incubator = NULL;
Tile* incubatorLit = NULL;
Tile* sunAltar = NULL;
Tile* skyrootWorkbench = NULL;
Tile* skyrootBed = NULL;
Tile* chestMimic = NULL;
Tile* skyrootChest = NULL;
Tile* ambrosiumTorch = NULL;
Tile* aetherPortal = NULL;

Item* ambrosiumShard = NULL;
Item* zaniteGem = NULL;
Item* gravititePlate = NULL;
Item* goldenAmber = NULL;
Item* blueberry = NULL;
Item* enchantedBlueberry = NULL;
Item* moaEgg = NULL;
Item* coldParachute = NULL;
Item* bronzeKey = NULL;
Item* silverKey = NULL;
Item* goldKey = NULL;
Item* skyrootDoorItem = NULL;
Item* skyrootBedItem = NULL;
Item* aetherSignItem = NULL;
Item* skyrootPickaxe = NULL;
Item* skyrootAxe = NULL;
Item* skyrootShovel = NULL;
Item* skyrootSword = NULL;
Item* zanitePickaxe = NULL;
Item* zaniteAxe = NULL;
Item* zaniteShovel = NULL;
Item* zaniteSword = NULL;
Item* gravititePickaxe = NULL;
Item* gravititeAxe = NULL;
Item* gravititeShovel = NULL;
Item* gravititeSword = NULL;

// Zanite sits between iron and diamond; gravitite is diamond-class.
static const Item::Tier ZANITE_TIER(2, 512, 7.0f, 2);
static const Item::Tier GRAVITITE_TIER(3, 1800, 9.0f, 3);

// ----------------------------------------------------------------------
// Tiles
// ----------------------------------------------------------------------
void initTiles() {
	using namespace AetherTex;
	const int S = ItemCategory::Structures;
	const int D = ItemCategory::Decorations;
	const int M = ItemCategory::Mechanisms;

	// Natural
	aetherGrass    = (new AetherGrassTile(TileId::AetherGrass, AETHER_GRASS_TOP, AETHER_GRASS_SIDE, false))->init()->setDestroyTime(0.6f)->setSoundType(Tile::SOUND_GRASS)->setCategory(S)->setDescriptionId("aetherGrass");
	enchantedGrass = (new AetherGrassTile(TileId::EnchantedGrass, ENCH_GRASS_TOP, ENCH_GRASS_SIDE, true))->init()->setDestroyTime(0.6f)->setSoundType(Tile::SOUND_GRASS)->setCategory(S)->setDescriptionId("enchantedAetherGrass");
	aetherDirt     = (new AetherBlock(TileId::AetherDirt, AETHER_DIRT, Material::dirt))->init()->setDestroyTime(0.5f)->setSoundType(Tile::SOUND_GRAVEL)->setCategory(S)->setDescriptionId("aetherDirt");
	holystone      = (new HolystoneTile(TileId::Holystone, HOLYSTONE))->init()->setDestroyTime(1.5f)->setExplodeable(10)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("holystone");
	mossyHolystone = (new HolystoneTile(TileId::MossyHolystone, MOSSY_HOLYSTONE))->init()->setDestroyTime(1.5f)->setExplodeable(10)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("mossyHolystone");
	icestone       = (new IcestoneTile(TileId::Icestone, ICESTONE))->init()->setDestroyTime(1.5f)->setExplodeable(10)->setSoundType(Tile::SOUND_GLASS)->setCategory(S)->setDescriptionId("icestone");
	quicksoil      = (new QuicksoilTile(TileId::Quicksoil, QUICKSOIL))->init()->setDestroyTime(0.5f)->setSoundType(Tile::SOUND_SAND)->setCategory(S)->setDescriptionId("quicksoil");
	aercloudCold   = (new AercloudTile(TileId::AercloudCold, AERCLOUD_COLD, AercloudTile::Cold))->init()->setDestroyTime(0.2f)->setSoundType(Tile::SOUND_CLOTH)->setCategory(S)->setDescriptionId("aercloudCold");
	aercloudBlue   = (new AercloudTile(TileId::AercloudBlue, AERCLOUD_BLUE, AercloudTile::Blue))->init()->setDestroyTime(0.2f)->setSoundType(Tile::SOUND_CLOTH)->setCategory(S)->setDescriptionId("aercloudBlue");
	aercloudGolden = (new AercloudTile(TileId::AercloudGolden, AERCLOUD_GOLDEN, AercloudTile::Golden))->init()->setDestroyTime(0.2f)->setSoundType(Tile::SOUND_CLOTH)->setCategory(S)->setDescriptionId("aercloudGolden");
	aercloudPurple = (new AercloudTile(TileId::AercloudPurple, AERCLOUD_PURPLE, AercloudTile::Purple))->init()->setDestroyTime(0.2f)->setSoundType(Tile::SOUND_CLOTH)->setCategory(S)->setDescriptionId("aercloudPurple");
	aercloudGreen  = (new AercloudTile(TileId::AercloudGreen, AERCLOUD_GREEN, AercloudTile::Green))->init()->setDestroyTime(0.2f)->setSoundType(Tile::SOUND_CLOTH)->setCategory(S)->setDescriptionId("aercloudGreen");
	aercloudStorm  = (new AercloudTile(TileId::AercloudStorm, AERCLOUD_STORM, AercloudTile::Storm))->init()->setDestroyTime(0.2f)->setSoundType(Tile::SOUND_CLOTH)->setCategory(S)->setDescriptionId("aercloudStorm");
	aerogel        = (new AerogelTile(TileId::Aerogel, AEROGEL))->init()->setDestroyTime(1.0f)->setExplodeable(2000)->setSoundType(Tile::SOUND_GLASS)->setCategory(S)->setDescriptionId("aerogel");
	skyrootLog     = (new AetherLogTile(TileId::SkyrootLog, SKYROOT_LOG_SIDE, SKYROOT_LOG_TOP, false))->init()->setDestroyTime(2.0f)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("skyrootLog");
	goldenOakLog   = (new AetherLogTile(TileId::GoldenOakLog, GOLDEN_OAK_LOG_SIDE, GOLDEN_OAK_LOG_TOP, true))->init()->setDestroyTime(2.0f)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("goldenOakLog");
	skyrootPlanks  = (new AetherBlock(TileId::SkyrootPlanks, SKYROOT_PLANKS, Material::wood))->init()->setDestroyTime(2.0f)->setExplodeable(5)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("skyrootPlanks");
	skyrootLeaves  = (LeafTile*)(new AetherLeafTile(TileId::SkyrootLeaves, SKYROOT_LEAVES, false))->init()->setDestroyTime(0.2f)->setLightBlock(1)->setSoundType(Tile::SOUND_GRASS)->setCategory(D)->setDescriptionId("skyrootLeaves");
	goldenOakLeaves= (LeafTile*)(new AetherLeafTile(TileId::GoldenOakLeaves, GOLDEN_OAK_LEAVES, true))->init()->setDestroyTime(0.2f)->setLightBlock(1)->setSoundType(Tile::SOUND_GRASS)->setCategory(D)->setDescriptionId("goldenOakLeaves");
	skyrootSapling = (new AetherSapling(TileId::SkyrootSapling, SKYROOT_SAPLING, false))->init()->setDestroyTime(0.0f)->setSoundType(Tile::SOUND_GRASS)->setCategory(D)->setDescriptionId("skyrootSapling");
	goldenOakSapling = (new AetherSapling(TileId::GoldenOakSapling, GOLDEN_OAK_SAPLING, true))->init()->setDestroyTime(0.0f)->setSoundType(Tile::SOUND_GRASS)->setCategory(D)->setDescriptionId("goldenOakSapling");
	whiteFlower    = (new AetherBush(TileId::WhiteFlower, WHITE_FLOWER))->init()->setDestroyTime(0.0f)->setSoundType(Tile::SOUND_GRASS)->setCategory(D)->setDescriptionId("whiteFlower");
	purpleFlower   = (new AetherBush(TileId::PurpleFlower, PURPLE_FLOWER))->init()->setDestroyTime(0.0f)->setSoundType(Tile::SOUND_GRASS)->setCategory(D)->setDescriptionId("purpleFlower");
	berryBush      = (new BerryBushTile(TileId::BerryBush, BERRY_BUSH))->init()->setDestroyTime(0.2f)->setSoundType(Tile::SOUND_GRASS)->setCategory(D)->setDescriptionId("berryBush");

	// Ores
	ambrosiumOre   = (new AetherOreTile(TileId::AmbrosiumOre, AMBROSIUM_ORE, AetherOreTile::Ambrosium))->init()->setDestroyTime(3.0f)->setExplodeable(5)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("ambrosiumOre");
	zaniteOre      = (new AetherOreTile(TileId::ZaniteOre, ZANITE_ORE, AetherOreTile::Zanite))->init()->setDestroyTime(3.0f)->setExplodeable(5)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("zaniteOre");
	gravititeOre   = (new AetherOreTile(TileId::GravititeOre, GRAVITITE_ORE, AetherOreTile::Gravitite))->init()->setDestroyTime(5.0f)->setExplodeable(5)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("gravititeOre");
	ambrosiumBlock = (new AetherBlock(TileId::AmbrosiumBlock, AMBROSIUM_BLOCK, Material::metal))->init()->setDestroyTime(3.0f)->setExplodeable(10)->setSoundType(Tile::SOUND_METAL)->setLightEmission(9.0f / 16.0f)->setCategory(D)->setDescriptionId("ambrosiumBlock");
	zaniteBlock    = (new AetherBlock(TileId::ZaniteBlock, ZANITE_BLOCK, Material::metal))->init()->setDestroyTime(4.0f)->setExplodeable(10)->setSoundType(Tile::SOUND_METAL)->setCategory(D)->setDescriptionId("zaniteBlock");
	enchantedGravitite = (new EnchantedGravititeTile(TileId::EnchantedGravitite, ENCH_GRAVITITE))->init()->setDestroyTime(5.0f)->setExplodeable(30)->setSoundType(Tile::SOUND_METAL)->setCategory(M)->setDescriptionId("enchantedGravitite");

	// Dungeon
	carvedStone    = (new DungeonStoneTile(TileId::CarvedStone, CARVED_STONE, Dungeon::Bronze, false))->init()->setDestroyTime(2.0f)->setExplodeable(15)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("carvedStone");
	sentryStone    = (new DungeonStoneTile(TileId::SentryStone, SENTRY_STONE, Dungeon::Bronze, false))->init()->setDestroyTime(2.0f)->setExplodeable(15)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("sentryStone");
	lightSentryStone = (new DungeonStoneTile(TileId::LightSentryStone, LIGHT_SENTRY_STONE, Dungeon::Bronze, true))->init()->setDestroyTime(2.0f)->setExplodeable(15)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("lightSentryStone");
	trappedSentryStone = (new TrappedSentryStoneTile(TileId::TrappedSentryStone, SENTRY_STONE))->init()->setDestroyTime(2.0f)->setExplodeable(15)->setSoundType(Tile::SOUND_STONE)->setCategory(M)->setDescriptionId("trappedSentryStone");
	angelicStone   = (new DungeonStoneTile(TileId::AngelicStone, ANGELIC_STONE, Dungeon::Silver, false))->init()->setDestroyTime(2.0f)->setExplodeable(15)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("angelicStone");
	lightAngelicStone = (new DungeonStoneTile(TileId::LightAngelicStone, LIGHT_ANGELIC_STONE, Dungeon::Silver, true))->init()->setDestroyTime(2.0f)->setExplodeable(15)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("lightAngelicStone");
	hellfireStone  = (new DungeonStoneTile(TileId::HellfireStone, HELLFIRE_STONE, Dungeon::Gold, false))->init()->setDestroyTime(2.0f)->setExplodeable(15)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("hellfireStone");
	lightHellfireStone = (new DungeonStoneTile(TileId::LightHellfireStone, LIGHT_HELLFIRE_STONE, Dungeon::Gold, true))->init()->setDestroyTime(2.0f)->setExplodeable(15)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("lightHellfireStone");
	lockedCarved   = (new LockedStoneTile(TileId::LockedCarved, LOCKED_CARVED, Dungeon::Bronze))->init()->setDestroyTime(-1.0f)->setExplodeable(6000000)->setSoundType(Tile::SOUND_STONE)->setCategory(M)->setDescriptionId("lockedCarvedStone");
	lockedAngelic  = (new LockedStoneTile(TileId::LockedAngelic, LOCKED_ANGELIC, Dungeon::Silver))->init()->setDestroyTime(-1.0f)->setExplodeable(6000000)->setSoundType(Tile::SOUND_STONE)->setCategory(M)->setDescriptionId("lockedAngelicStone");
	lockedHellfire = (new LockedStoneTile(TileId::LockedHellfire, LOCKED_HELLFIRE, Dungeon::Gold))->init()->setDestroyTime(-1.0f)->setExplodeable(6000000)->setSoundType(Tile::SOUND_STONE)->setCategory(M)->setDescriptionId("lockedHellfireStone");
	pillar         = (new PillarTile(TileId::Pillar, PILLAR_SIDE, PILLAR_TOP))->init()->setDestroyTime(2.0f)->setExplodeable(15)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("aetherPillar");

	// Construction: skyroot set
	skyrootStairs  = (new StairTile(TileId::SkyrootStairs, skyrootPlanks))->init()->setCategory(S)->setDescriptionId("skyrootStairs");
	skyrootSlab    = (new AetherSlabTile(TileId::SkyrootSlab, SKYROOT_PLANKS, Material::wood, false, TileId::SkyrootSlab, TileId::SkyrootSlabDouble))->init()->setDestroyTime(2.0f)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("skyrootSlab");
	skyrootSlabDouble = (new AetherSlabTile(TileId::SkyrootSlabDouble, SKYROOT_PLANKS, Material::wood, true, TileId::SkyrootSlab, TileId::SkyrootSlabDouble))->init()->setDestroyTime(2.0f)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("skyrootSlab");
	skyrootFence   = (new SkyrootFenceTile(TileId::SkyrootFence, SKYROOT_PLANKS))->init()->setDestroyTime(2.0f)->setExplodeable(5)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("skyrootFence");
	skyrootFenceGate = (new FenceGateTile(TileId::SkyrootFenceGate, SKYROOT_PLANKS))->init()->setDestroyTime(2.0f)->setExplodeable(5)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("skyrootFenceGate");
	skyrootDoor    = (new SkyrootDoorTile(TileId::SkyrootDoor))->init()->setDestroyTime(3.0f)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("skyrootDoor");
	skyrootTrapdoor= (new SkyrootTrapdoorTile(TileId::SkyrootTrapdoor))->init()->setDestroyTime(3.0f)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("skyrootTrapdoor");
	skyrootPressurePlate = (new AetherPressurePlateTile(TileId::SkyrootPressurePlate, SKYROOT_PLANKS, Material::wood, false))->init()->setDestroyTime(0.5f)->setSoundType(Tile::SOUND_WOOD)->setCategory(M)->setDescriptionId("skyrootPressurePlate");
	skyrootButton  = (new AetherButtonTile(TileId::SkyrootButton, SKYROOT_PLANKS))->init()->setDestroyTime(0.5f)->setSoundType(Tile::SOUND_WOOD)->setCategory(M)->setDescriptionId("skyrootButton");

	// Construction: holystone set
	holystoneBricks= (new HolystoneTile(TileId::HolystoneBricks, HOLYSTONE_BRICKS))->init()->setDestroyTime(2.0f)->setExplodeable(10)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("holystoneBricks");
	holystoneStairs= (new StairTile(TileId::HolystoneStairs, holystone))->init()->setCategory(S)->setDescriptionId("holystoneStairs");
	holystoneBrickStairs = (new StairTile(TileId::HolystoneBrickStairs, holystoneBricks))->init()->setCategory(S)->setDescriptionId("holystoneBrickStairs");
	holystoneSlab  = (new AetherSlabTile(TileId::HolystoneSlab, HOLYSTONE, Material::stone, false, TileId::HolystoneSlab, TileId::HolystoneSlabDouble))->init()->setDestroyTime(2.0f)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("holystoneSlab");
	holystoneSlabDouble = (new AetherSlabTile(TileId::HolystoneSlabDouble, HOLYSTONE, Material::stone, true, TileId::HolystoneSlab, TileId::HolystoneSlabDouble))->init()->setDestroyTime(2.0f)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("holystoneSlab");
	holystonePressurePlate = (new AetherPressurePlateTile(TileId::HolystonePressurePlate, HOLYSTONE, Material::stone, true))->init()->setDestroyTime(0.5f)->setSoundType(Tile::SOUND_STONE)->setCategory(M)->setDescriptionId("holystonePressurePlate");

	// Glass, wool, banners, signs
	quicksoilGlass = (new QuicksoilGlassTile(TileId::QuicksoilGlass, QUICKSOIL_GLASS))->init()->setDestroyTime(0.3f)->setSoundType(Tile::SOUND_GLASS)->setCategory(S)->setDescriptionId("quicksoilGlass");
	quicksoilGlassPane = (new QuicksoilGlassPaneTile(TileId::QuicksoilGlassPane, QUICKSOIL_GLASS, 4 + 9 * 16))->init()->setDestroyTime(0.3f)->setSoundType(Tile::SOUND_GLASS)->setCategory(S)->setDescriptionId("quicksoilGlassPane");
	cloudwool      = (new CloudwoolTile(TileId::Cloudwool, CLOUDWOOL))->init()->setDestroyTime(0.8f)->setSoundType(Tile::SOUND_CLOTH)->setCategory(S)->setDescriptionId("cloudwool");
	cloudwoolCarpet= (new CloudwoolCarpetTile(TileId::CloudwoolCarpet, CLOUDWOOL))->init()->setDestroyTime(0.1f)->setSoundType(Tile::SOUND_CLOTH)->setCategory(D)->setDescriptionId("cloudwoolCarpet");
	aetherBanner   = (new AetherBannerTile(TileId::AetherBanner, CLOUDWOOL))->init()->setDestroyTime(1.0f)->setSoundType(Tile::SOUND_CLOTH)->setCategory(D)->setDescriptionId("aetherBanner");
	aetherSign     = (new AetherSignTile(TileId::AetherSign, true))->init()->setDestroyTime(1.0f)->setSoundType(Tile::SOUND_WOOD)->setCategory(D)->setDescriptionId("aetherSign");
	aetherWallSign = (new AetherSignTile(TileId::AetherWallSign, false))->init()->setDestroyTime(1.0f)->setSoundType(Tile::SOUND_WOOD)->setCategory(D)->setDescriptionId("aetherSign");

	// Workstations
	altar          = (new AetherFurnaceTile(TileId::Altar, AetherFurnaceTile::AltarKind, false, ALTAR_TOP, ALTAR_SIDE, ALTAR_SIDE_LIT))->init()->setDestroyTime(3.5f)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("aetherAltar");
	altarLit       = (new AetherFurnaceTile(TileId::AltarLit, AetherFurnaceTile::AltarKind, true, ALTAR_TOP, ALTAR_SIDE, ALTAR_SIDE_LIT))->init()->setDestroyTime(3.5f)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("aetherAltar");
	freezer        = (new AetherFurnaceTile(TileId::Freezer, AetherFurnaceTile::FreezerKind, false, FREEZER_TOP, FREEZER_SIDE, FREEZER_SIDE_LIT))->init()->setDestroyTime(3.5f)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("aetherFreezer");
	freezerLit     = (new AetherFurnaceTile(TileId::FreezerLit, AetherFurnaceTile::FreezerKind, true, FREEZER_TOP, FREEZER_SIDE, FREEZER_SIDE_LIT))->init()->setDestroyTime(3.5f)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("aetherFreezer");
	incubator      = (new AetherFurnaceTile(TileId::Incubator, AetherFurnaceTile::IncubatorKind, false, INCUBATOR_TOP, INCUBATOR_SIDE, INCUBATOR_SIDE_LIT))->init()->setDestroyTime(3.5f)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("aetherIncubator");
	incubatorLit   = (new AetherFurnaceTile(TileId::IncubatorLit, AetherFurnaceTile::IncubatorKind, true, INCUBATOR_TOP, INCUBATOR_SIDE, INCUBATOR_SIDE_LIT))->init()->setDestroyTime(3.5f)->setSoundType(Tile::SOUND_STONE)->setCategory(S)->setDescriptionId("aetherIncubator");
	sunAltar       = (new SunAltarTile(TileId::SunAltar, SUN_ALTAR_TOP, SUN_ALTAR_SIDE))->init()->setDestroyTime(3.5f)->setExplodeable(30)->setSoundType(Tile::SOUND_STONE)->setCategory(M)->setDescriptionId("sunAltar");
	skyrootWorkbench = (new SkyrootWorkbenchTile(TileId::SkyrootWorkbench, SKYROOT_WORKBENCH_TOP, SKYROOT_WORKBENCH_SIDE, SKYROOT_WORKBENCH_FRONT))->init()->setDestroyTime(2.5f)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("skyrootWorkbench");
	skyrootBed     = (new SkyrootBedTile(TileId::SkyrootBed, SKYROOT_BED_TOP_FOOT))->init()->setDestroyTime(0.2f)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("skyrootBed");
	chestMimic     = (new ChestMimicTile(TileId::ChestMimic, SKYROOT_CHEST_SIDE))->init()->setDestroyTime(2.5f)->setSoundType(Tile::SOUND_WOOD)->setCategory(M)->setDescriptionId("chestMimic");
	skyrootChest   = (new SkyrootChestTile(TileId::SkyrootChest, SKYROOT_CHEST_SIDE))->init()->setDestroyTime(2.5f)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("skyrootChest");
	ambrosiumTorch = (new AmbrosiumTorchTile(TileId::AmbrosiumTorch, AMBROSIUM_TORCH))->init()->setDestroyTime(0.0f)->setLightEmission(15.0f / 16.0f)->setSoundType(Tile::SOUND_WOOD)->setCategory(S)->setDescriptionId("ambrosiumTorch");
	aetherPortal   = (new AetherPortalTile(TileId::AetherPortal, AETHER_PORTAL))->init()->setDestroyTime(-1.0f)->setSoundType(Tile::SOUND_GLASS)->setCategory(S)->setDescriptionId("aetherPortal");

	// Items with aux data (colored wool / carpet / banner)
	Item::items[cloudwool->id] = (new AuxDataTileItem(cloudwool->id - 256, cloudwool))->setCategory(S)->setDescriptionId("cloudwool");
	Item::items[cloudwoolCarpet->id] = (new AuxDataTileItem(cloudwoolCarpet->id - 256, cloudwoolCarpet))->setCategory(D)->setDescriptionId("cloudwoolCarpet");
	Item::items[aetherBanner->id] = (new AuxDataTileItem(aetherBanner->id - 256, aetherBanner))->setCategory(D)->setDescriptionId("aetherBanner");
	Item::items[pillar->id] = (new AuxDataTileItem(pillar->id - 256, pillar))->setCategory(S)->setDescriptionId("aetherPillar");
}

// ----------------------------------------------------------------------
// Items
// ----------------------------------------------------------------------
void initItems() {
	using namespace AetherIcon;
	const int T = ItemCategory::Tools;
	const int F = ItemCategory::FoodArmor;
	const int S = ItemCategory::Structures;
	const int D = ItemCategory::Decorations;

	// Buckets (previously disabled in Item::initItems)
	Item::bucket_empty = (new BucketItem(69, 0))->setIcon(10, 4)->setCategory(T)->setDescriptionId("bucket");
	Item::bucket_water = (new BucketItem(70, Tile::water->id))->setIcon(11, 4)->setCategory(T)->setDescriptionId("bucketWater");
	Item::bucket_lava  = (new BucketItem(71, Tile::lava->id))->setIcon(12, 4)->setCategory(T)->setDescriptionId("bucketLava");

	ambrosiumShard = (new AmbrosiumShardItem(ItemId::AmbrosiumShard))->setIcon(AMBROSIUM_SHARD)->setCategory(T)->setDescriptionId("ambrosiumShard");
	zaniteGem      = (new Item(ItemId::ZaniteGem))->setIcon(ZANITE_GEM)->setCategory(T)->setDescriptionId("zaniteGemstone");
	gravititePlate = (new Item(ItemId::GravititePlate))->setIcon(GRAVITITE_PLATE)->setCategory(T)->setDescriptionId("gravititePlate");
	goldenAmber    = (new Item(ItemId::GoldenAmber))->setIcon(GOLDEN_AMBER)->setCategory(T)->setDescriptionId("goldenAmber");
	blueberry      = (new FoodItem(ItemId::Blueberry, 2, false))->setIcon(BLUEBERRY)->setCategory(F)->setDescriptionId("blueberry");
	enchantedBlueberry = (new FoodItem(ItemId::EnchantedBlueberry, 8, false, 1.2f))->setIcon(BLUEBERRY)->setCategory(F)->setDescriptionId("enchantedBlueberry");
	moaEgg         = (new MoaEggItem(ItemId::MoaEgg))->setIcon(MOA_EGG)->setCategory(F)->setDescriptionId("moaEgg");
	coldParachute  = (new ColdParachuteItem(ItemId::ColdParachute))->setIcon(COLD_PARACHUTE)->setCategory(T)->setDescriptionId("coldParachute");
	bronzeKey      = (new DungeonKeyItem(ItemId::BronzeKey, Dungeon::Bronze))->setIcon(BRONZE_KEY)->setCategory(T)->setDescriptionId("bronzeKey");
	silverKey      = (new DungeonKeyItem(ItemId::SilverKey, Dungeon::Silver))->setIcon(SILVER_KEY)->setCategory(T)->setDescriptionId("silverKey");
	goldKey        = (new DungeonKeyItem(ItemId::GoldKey, Dungeon::Gold))->setIcon(GOLD_KEY)->setCategory(T)->setDescriptionId("goldKey");
	skyrootDoorItem= (new SkyrootDoorItem(ItemId::SkyrootDoor))->setIcon(SKYROOT_DOOR_ITEM)->setCategory(S)->setDescriptionId("skyrootDoor");
	skyrootBedItem = (new SkyrootBedItem(ItemId::SkyrootBed))->setIcon(SKYROOT_BED_ITEM)->setCategory(S)->setDescriptionId("skyrootBed");
	aetherSignItem = (new AetherSignItem(ItemId::AetherSign))->setIcon(SKYROOT_SIGN_ITEM)->setCategory(D)->setDescriptionId("aetherSign");

	skyrootPickaxe = (new SkyrootPickaxeItem(ItemId::SkyrootPickaxe))->setIcon(SKYROOT_PICKAXE)->setCategory(T)->setDescriptionId("skyrootPickaxe");
	skyrootAxe     = (new SkyrootAxeItem(ItemId::SkyrootAxe))->setIcon(SKYROOT_AXE)->setCategory(T)->setDescriptionId("skyrootAxe");
	skyrootShovel  = (new SkyrootShovelItem(ItemId::SkyrootShovel))->setIcon(SKYROOT_SHOVEL)->setCategory(T)->setDescriptionId("skyrootShovel");
	skyrootSword   = (new WeaponItem(ItemId::SkyrootSword, Item::Tier::WOOD))->setIcon(SKYROOT_SWORD)->setCategory(T)->setDescriptionId("skyrootSword");
	zanitePickaxe  = (new ZanitePickaxeItem(ItemId::ZanitePickaxe, ZANITE_TIER))->setIcon(ZANITE_PICKAXE)->setCategory(T)->setDescriptionId("zanitePickaxe");
	zaniteAxe      = (new ZaniteAxeItem(ItemId::ZaniteAxe, ZANITE_TIER))->setIcon(ZANITE_AXE)->setCategory(T)->setDescriptionId("zaniteAxe");
	zaniteShovel   = (new ZaniteShovelItem(ItemId::ZaniteShovel, ZANITE_TIER))->setIcon(ZANITE_SHOVEL)->setCategory(T)->setDescriptionId("zaniteShovel");
	zaniteSword    = (new WeaponItem(ItemId::ZaniteSword, ZANITE_TIER))->setIcon(ZANITE_SWORD)->setCategory(T)->setDescriptionId("zaniteSword");
	gravititePickaxe = (new GravititePickaxeItem(ItemId::GravititePickaxe))->setIcon(GRAVITITE_PICKAXE)->setCategory(T)->setDescriptionId("gravititePickaxe");
	gravititeAxe   = (new GravititeAxeItem(ItemId::GravititeAxe))->setIcon(GRAVITITE_AXE)->setCategory(T)->setDescriptionId("gravititeAxe");
	gravititeShovel= (new GravititeShovelItem(ItemId::GravititeShovel))->setIcon(GRAVITITE_SHOVEL)->setCategory(T)->setDescriptionId("gravititeShovel");
	gravititeSword = (new WeaponItem(ItemId::GravititeSword, GRAVITITE_TIER))->setIcon(GRAVITITE_SWORD)->setCategory(T)->setDescriptionId("gravititeSword");
}

// ----------------------------------------------------------------------
// Recipes
// ----------------------------------------------------------------------
void addRecipes(Recipes* r) {
	// Wood
	r->addShapelessRecipe(ItemInstance(skyrootPlanks, 4), definition('#', skyrootLog));
	r->addShapelessRecipe(ItemInstance(skyrootPlanks, 4), definition('#', goldenOakLog));
	r->addShapedRecipe(ItemInstance(Item::stick, 4), "#", "#", definition('#', skyrootPlanks));
	r->addShapedRecipe(ItemInstance(skyrootWorkbench, 1), "##", "##", definition('#', skyrootPlanks));
	r->addShapedRecipe(ItemInstance(skyrootChest, 1), "###", "# #", "###", definition('#', skyrootPlanks));
	r->addShapedRecipe(ItemInstance(skyrootStairs, 4), "#  ", "## ", "###", definition('#', skyrootPlanks));
	r->addShapedRecipe(ItemInstance(skyrootSlab, 6), "###", definition('#', skyrootPlanks));
	r->addShapedRecipe(ItemInstance(skyrootFence, 2), "###", "###", definition('#', Item::stick));
	r->addShapedRecipe(ItemInstance(skyrootFenceGate, 1), "#X#", "#X#", definition('#', Item::stick, 'X', skyrootPlanks));
	r->addShapedRecipe(ItemInstance(skyrootDoorItem, 1), "##", "##", "##", definition('#', skyrootPlanks));
	r->addShapedRecipe(ItemInstance(skyrootTrapdoor, 2), "###", "###", definition('#', skyrootPlanks));
	r->addShapedRecipe(ItemInstance(skyrootPressurePlate, 1), "##", definition('#', skyrootPlanks));
	r->addShapelessRecipe(ItemInstance(skyrootButton, 1), definition('#', skyrootPlanks));
	r->addShapedRecipe(ItemInstance(skyrootBedItem, 1), "###", "XXX", definition('#', ItemInstance(Item::items[Tile::cloth->id], 1, 0), 'X', skyrootPlanks));
	r->addShapedRecipe(ItemInstance(skyrootBedItem, 1), "###", "XXX", definition('#', ItemInstance(Item::items[cloudwool->id], 1, 0), 'X', skyrootPlanks));
	r->addShapedRecipe(ItemInstance(aetherSignItem, 1), "###", "###", " X ", definition('#', skyrootPlanks, 'X', Item::stick));
	r->addShapedRecipe(ItemInstance(coldParachute, 1), "###", "# #", " X ", definition('#', ItemInstance(Item::items[cloudwool->id], 1, Recipe::ANY_AUX_VALUE), 'X', Item::stick));

	// Stone
	r->addShapedRecipe(ItemInstance(holystoneBricks, 4), "##", "##", definition('#', holystone));
	r->addShapedRecipe(ItemInstance(holystoneStairs, 4), "#  ", "## ", "###", definition('#', holystone));
	r->addShapedRecipe(ItemInstance(holystoneBrickStairs, 4), "#  ", "## ", "###", definition('#', holystoneBricks));
	r->addShapedRecipe(ItemInstance(holystoneSlab, 6), "###", definition('#', holystone));
	r->addShapedRecipe(ItemInstance(holystonePressurePlate, 1), "##", definition('#', holystone));
	r->addShapedRecipe(ItemInstance(mossyHolystone, 1), "#X", definition('#', holystone, 'X', skyrootLeaves));
	r->addShapedRecipe(ItemInstance(pillar, 2), "#", "#", definition('#', holystoneBricks));
	r->addShapedRecipe(ItemInstance(carvedStone, 4), "##", "##", definition('#', holystoneBricks));

	// Glass / wool
	r->addShapedRecipe(ItemInstance(quicksoilGlassPane, 16), "###", "###", definition('#', quicksoilGlass));
	r->addShapedRecipe(ItemInstance(cloudwool, 1, 15), "##", "##", definition('#', aercloudCold));
	r->addShapedRecipe(ItemInstance(cloudwoolCarpet, 3, 15), "##", definition('#', ItemInstance(Item::items[cloudwool->id], 1, 15)));
	r->addShapedRecipe(ItemInstance(aetherBanner, 1, 15), "##", "##", "X ", definition('#', ItemInstance(Item::items[cloudwool->id], 1, 15), 'X', Item::stick));
	for (int i = 0; i < 16; i++) {
		if (i == 15) continue;
		r->addShapelessRecipe(ItemInstance(cloudwool, 1, i), definition(0, ItemInstance(Item::dye_powder, 1, i), 0, ItemInstance(Item::items[cloudwool->id], 1, 15)));
		r->addShapelessRecipe(ItemInstance(cloudwoolCarpet, 1, i), definition(0, ItemInstance(Item::dye_powder, 1, i), 0, ItemInstance(Item::items[cloudwoolCarpet->id], 1, 15)));
		r->addShapelessRecipe(ItemInstance(aetherBanner, 1, i), definition(0, ItemInstance(Item::dye_powder, 1, i), 0, ItemInstance(Item::items[aetherBanner->id], 1, 15)));
	}
	// Flowers -> dye
	r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::WHITE), definition(0, whiteFlower));
	r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::PURPLE), definition(0, purpleFlower));

	// Ambrosium / zanite / gravitite blocks
	r->addShapedRecipe(ItemInstance(ambrosiumBlock, 1), "###", "###", "###", definition('#', ambrosiumShard));
	r->addShapelessRecipe(ItemInstance(ambrosiumShard, 9), definition('#', ambrosiumBlock));
	r->addShapedRecipe(ItemInstance(zaniteBlock, 1), "###", "###", "###", definition('#', zaniteGem));
	r->addShapelessRecipe(ItemInstance(zaniteGem, 9), definition('#', zaniteBlock));
	r->addShapedRecipe(ItemInstance(ambrosiumTorch, 2), "#", "X", definition('#', ambrosiumShard, 'X', Item::stick));

	// Workstations
	r->addShapedRecipe(ItemInstance(altar, 1), "###", "#X#", "###", definition('#', holystone, 'X', zaniteGem));
	r->addShapedRecipe(ItemInstance(freezer, 1), "###", "#X#", "###", definition('#', holystone, 'X', icestone));
	r->addShapedRecipe(ItemInstance(incubator, 1), "###", "#X#", "###", definition('#', holystone, 'X', ambrosiumTorch));

	// Buckets
	r->addShapedRecipe(ItemInstance(Item::bucket_empty, 1), "# #", " # ", definition('#', Item::ironIngot));

	// Tools: skyroot
	r->addShapedRecipe(ItemInstance(skyrootPickaxe, 1), "###", " X ", " X ", definition('#', skyrootPlanks, 'X', Item::stick));
	r->addShapedRecipe(ItemInstance(skyrootAxe, 1), "##", "#X", " X", definition('#', skyrootPlanks, 'X', Item::stick));
	r->addShapedRecipe(ItemInstance(skyrootShovel, 1), "#", "X", "X", definition('#', skyrootPlanks, 'X', Item::stick));
	r->addShapedRecipe(ItemInstance(skyrootSword, 1), "#", "#", "X", definition('#', skyrootPlanks, 'X', Item::stick));
	// Tools: zanite
	r->addShapedRecipe(ItemInstance(zanitePickaxe, 1), "###", " X ", " X ", definition('#', zaniteGem, 'X', Item::stick));
	r->addShapedRecipe(ItemInstance(zaniteAxe, 1), "##", "#X", " X", definition('#', zaniteGem, 'X', Item::stick));
	r->addShapedRecipe(ItemInstance(zaniteShovel, 1), "#", "X", "X", definition('#', zaniteGem, 'X', Item::stick));
	r->addShapedRecipe(ItemInstance(zaniteSword, 1), "#", "#", "X", definition('#', zaniteGem, 'X', Item::stick));
	// Tools: gravitite
	r->addShapedRecipe(ItemInstance(gravititePickaxe, 1), "###", " X ", " X ", definition('#', gravititePlate, 'X', Item::stick));
	r->addShapedRecipe(ItemInstance(gravititeAxe, 1), "##", "#X", " X", definition('#', gravititePlate, 'X', Item::stick));
	r->addShapedRecipe(ItemInstance(gravititeShovel, 1), "#", "X", "X", definition('#', gravititePlate, 'X', Item::stick));
	r->addShapedRecipe(ItemInstance(gravititeSword, 1), "#", "#", "X", definition('#', gravititePlate, 'X', Item::stick));
}

void addFurnaceRecipes(FurnaceRecipes* recipes) {
	// Furnace recipes are added through the FurnaceRecipes ctor patch (see FurnaceRecipes.cpp)
	(void)recipes;
}

// ----------------------------------------------------------------------
// Creative menu
// ----------------------------------------------------------------------
void addCreativeItems(ItemInstanceSink& sink) {
	Tile* tiles[] = {
		aetherGrass, enchantedGrass, aetherDirt, holystone, mossyHolystone, holystoneBricks, icestone, quicksoil,
		aercloudCold, aercloudBlue, aercloudGolden, aercloudPurple, aercloudGreen, aercloudStorm, aerogel,
		skyrootLog, goldenOakLog, skyrootPlanks, skyrootLeaves, goldenOakLeaves, skyrootSapling, goldenOakSapling,
		whiteFlower, purpleFlower, berryBush,
		ambrosiumOre, zaniteOre, gravititeOre, ambrosiumBlock, zaniteBlock, enchantedGravitite,
		carvedStone, sentryStone, lightSentryStone, trappedSentryStone, angelicStone, lightAngelicStone,
		hellfireStone, lightHellfireStone, lockedCarved, lockedAngelic, lockedHellfire,
		skyrootStairs, skyrootSlab, skyrootFence, skyrootFenceGate, skyrootTrapdoor, skyrootPressurePlate, skyrootButton,
		holystoneStairs, holystoneBrickStairs, holystoneSlab, holystonePressurePlate,
		quicksoilGlass, quicksoilGlassPane,
		altar, freezer, incubator, sunAltar, skyrootWorkbench, chestMimic, skyrootChest, ambrosiumTorch, aetherPortal,
		NULL
	};
	for (int i = 0; tiles[i]; ++i) sink.add(tiles[i]->id, 1, 0);
	sink.add(pillar->id, 1, 0);
	for (int i = 0; i < 16; ++i) sink.add(cloudwool->id, 1, i);
	for (int i = 0; i < 16; ++i) sink.add(cloudwoolCarpet->id, 1, i);
	for (int i = 0; i < 16; ++i) sink.add(aetherBanner->id, 1, i);

	Item* items[] = {
		Item::bucket_water, Item::bucket_lava, Item::bucket_empty,
		ambrosiumShard, zaniteGem, gravititePlate, goldenAmber, blueberry, enchantedBlueberry, moaEgg, coldParachute,
		bronzeKey, silverKey, goldKey, skyrootDoorItem, skyrootBedItem, aetherSignItem,
		skyrootPickaxe, skyrootAxe, skyrootShovel, skyrootSword,
		zanitePickaxe, zaniteAxe, zaniteShovel, zaniteSword,
		gravititePickaxe, gravititeAxe, gravititeShovel, gravititeSword,
		NULL
	};
	for (int i = 0; items[i]; ++i) sink.add(items[i]->id, 1, 0);
	sink.add(Item::mobEgg->id, 1, MobTypes::Moa);
	sink.add(Item::mobEgg->id, 1, MobTypes::Sentry);
	sink.add(Item::mobEgg->id, 1, MobTypes::Mimic);
}

// ----------------------------------------------------------------------
// Tool helpers
// ----------------------------------------------------------------------
bool isSkyrootTool(int itemId) {
	return (skyrootPickaxe && itemId == skyrootPickaxe->id) || (skyrootAxe && itemId == skyrootAxe->id)
		|| (skyrootShovel && itemId == skyrootShovel->id) || (skyrootSword && itemId == skyrootSword->id);
}
bool isZaniteTool(int itemId) {
	return (zanitePickaxe && itemId == zanitePickaxe->id) || (zaniteAxe && itemId == zaniteAxe->id)
		|| (zaniteShovel && itemId == zaniteShovel->id) || (zaniteSword && itemId == zaniteSword->id);
}
bool isGravititeTool(int itemId) {
	return (gravititePickaxe && itemId == gravititePickaxe->id) || (gravititeAxe && itemId == gravititeAxe->id)
		|| (gravititeShovel && itemId == gravititeShovel->id) || (gravititeSword && itemId == gravititeSword->id);
}
int axeTier(int itemId) {
	if (skyrootAxe && itemId == skyrootAxe->id) return 1;
	if (zaniteAxe && itemId == zaniteAxe->id) return 2;
	if (gravititeAxe && itemId == gravititeAxe->id) return 3;
	return 0;
}

// ----------------------------------------------------------------------
// Dungeons / mobs
// ----------------------------------------------------------------------
bool isDungeonGuarded(Level* level, int tier, int x, int y, int z) {
	if (!level) return false;
	const float r = 40.0f;
	EntityList list;
	level->getEntitiesOfClass(MobTypes::BaseEnemy, AABB(x - r, y - r, z - r, x + r, y + r, z + r), list);
	for (unsigned int i = 0; i < list.size(); ++i) {
		AetherBoss* boss = dynamic_cast<AetherBoss*>(list[i]);
		if (boss && boss->isAlive() && boss->getTier() == tier) return true;
	}
	return false;
}

Mob* spawnMob(Level* level, int mobType, float x, float y, float z) {
	if (!level || level->isClientSide) return NULL;
	Mob* mob = MobFactory::CreateMob(mobType, level);
	if (!mob) return NULL;
	mob->moveTo(x, y, z, level->random.nextFloat() * 360.0f, 0);
	level->addEntity(mob);
	if (AetherBoss* boss = dynamic_cast<AetherBoss*>(mob)) {
		boss->homeX = (int)x; boss->homeY = (int)y; boss->homeZ = (int)z;
	}
	return mob;
}

// ----------------------------------------------------------------------
// Cold Parachute
// ----------------------------------------------------------------------
void tickParachute(Player* player) {
	if (!player || !coldParachute || !player->level) return;
	if (player->onGround || player->yd >= -0.1f) return;
	if (player->abilities.flying || player->isInWater()) return;
	ItemInstance* held = player->inventory->getSelected();
	if (!held || held->isNull() || held->id != coldParachute->id) return;

	// Drift down gently and never take fall damage while the chute is out
	if (player->yd < -0.12f) player->yd = -0.12f;
	player->fallDistance = 0;
	// Wear the chute out slowly (survival only)
	if (!player->level->isClientSide && !player->abilities.instabuild && (player->tickCount % 10) == 0) {
		held->hurt(1);
		if (held->getDamageValue() >= held->getMaxDamage()) player->inventory->removeItem(held);
	}
	if ((player->tickCount % 4) == 0)
		player->level->addParticle(PARTICLETYPE(snowballpoof), player->x + (player->random.nextFloat() - 0.5f), player->y + 1.2f, player->z + (player->random.nextFloat() - 0.5f), 0, 0, 0);
}

} // namespace Aether
