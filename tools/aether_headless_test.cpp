// Headless smoke test for the Aether dimension (not part of the game build).
// Build via tools/aether_headless_test.sh (links against the emscripten object cache).
#include <cstdio>
#include <cstring>
#include <map>
#include "world/level/Level.h"
#include "world/level/LevelSettings.h"
#include "world/level/storage/MemoryLevelStorage.h"
#include "world/level/dimension/Dimension.h"
#include "world/level/material/Material.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/entity/TileEntity.h"
#include "world/level/biome/Biome.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/chunk/ChunkSource.h"
#include "world/level/portal/PortalForcer.h"
#include "world/entity/MobCategory.h"
#include "world/entity/player/Player.h"
#include "world/entity/player/Inventory.h"
#include "world/entity/MobFactory.h"
#include "world/item/Item.h"
#include "world/item/ItemInstance.h"
#include "world/item/crafting/Recipes.h"
#include "world/item/crafting/FurnaceRecipes.h"
#include "world/aether/Aether.h"
#include "world/aether/AetherTiles.h"
#include "world/aether/AetherItems.h"
#include "world/aether/AetherTileEntity.h"
#include "world/aether/AetherDungeons.h"
#include "world/rpg/Rpg.h"
#include "network/RakNetInstance.h"
#include "server/ServerLevel.h"

static int fails = 0;
#define CHECK(cond, ...) do { if (!(cond)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } else { printf("ok: " __VA_ARGS__); printf("\n"); } } while (0)

static Level* makeLevel(int gameType, int dimId) {
	Dimension* dim = dimId == Dimension::NORMAL ? NULL : Dimension::getNew(dimId);
	Level* level = new ServerLevel(new MemoryLevelStorage(), "test", LevelSettings(1234567, gameType), 3, dim);
	level->raknetInstance = new IRakNetInstance();
	return level;
}

int main() {
	Material::initMaterials();
	MobCategory::initMobCategories();
	Tile::initTiles();
	Item::initItems();
	Biome::initBiomes();
	TileEntity::initTileEntities();

	// --- Registry sanity ---
	CHECK(Aether::holystone && Aether::holystone->id == Aether::TileId::Holystone, "holystone registered (%d)", Aether::holystone ? Aether::holystone->id : -1);
	CHECK(Item::items[Aether::holystone->id] != NULL, "holystone tile item exists");
	CHECK(Aether::ambrosiumShard && Item::items[Aether::ambrosiumShard->id] == Aether::ambrosiumShard, "ambrosium shard item registered");
	CHECK(Item::bucket_water != NULL && Item::bucket_lava != NULL && Item::bucket_empty != NULL, "bucket items registered");
	int missingCat = 0;
	for (int i = 0; i < 256; i++) if (Tile::tiles[i] && Tile::tiles[i]->category == 0) { missingCat++; printf("  tile %d has no category\n", i); }
	CHECK(missingCat == 0, "all tiles have a category (%d missing)", missingCat);

	// --- Recipes ---
	Recipes* r = Recipes::getInstance();
	int aetherRecipes = 0;
	for (size_t i = 0; i < r->getRecipes().size(); i++) {
		ItemInstance res = r->getRecipes()[i]->getResultItem();
		if (res.isNull()) continue;
		int id = res.id;
		if (id >= 256 + 111 && id <= 256 + 140) aetherRecipes++;
		else if (id >= 160 && id <= 236) aetherRecipes++;
	}
	CHECK(aetherRecipes > 20, "Aether crafting recipes registered (%d)", aetherRecipes);

	// --- Creative menu (survival/creative world gets Aether + water bucket, RPG does not) ---
	{
		Level* lvl = makeLevel(GameType::Creative, Dimension::NORMAL);
		CHECK(Aether::isAvailable(lvl), "Aether available in creative world");
		Player* p = new Player(lvl, true);
		bool hasWater = false, hasHolystone = false, hasFreezer = false;
		for (int i = 0; i < p->inventory->getContainerSize(); i++) {
			ItemInstance* it = p->inventory->getItem(i);
			if (!it) continue;
			if (it->id == Item::bucket_water->id) hasWater = true;
			if (it->id == Aether::holystone->id) hasHolystone = true;
			if (it->id == Aether::freezer->id) hasFreezer = true;
		}
		CHECK(hasWater, "water bucket in creative menu");
		CHECK(hasHolystone && hasFreezer, "Aether blocks in creative menu");
	}
	{
		Level* lvl = makeLevel(GameType::RPG, Dimension::NORMAL);
		CHECK(!Aether::isAvailable(lvl), "Aether NOT available in RPG world");
		Player* p = new Player(lvl, true);
		bool hasHolystone = false, hasWater = false;
		for (int i = 0; i < p->inventory->getContainerSize(); i++) {
			ItemInstance* it = p->inventory->getItem(i);
			if (!it) continue;
			if (it->id == Aether::holystone->id) hasHolystone = true;
			if (it->id == Item::bucket_water->id) hasWater = true;
		}
		CHECK(!hasHolystone, "no Aether blocks in RPG creative menu");
		CHECK(hasWater, "water bucket still in RPG creative menu");
	}

	// --- Aether terrain generation ---
	{
		Level* aether = makeLevel(GameType::Survival, Dimension::AETHER);
		CHECK(aether->dimension && aether->dimension->id == Dimension::AETHER, "Aether level has dimension id 1");
		std::map<int, int> counts;
		int islandCols = 0, cols = 0;
		for (int cx = -3; cx <= 3; cx++) for (int cz = -3; cz <= 3; cz++) {
			aether->getChunk(cx, cz);
		}
		for (int cx = -2; cx <= 2; cx++) for (int cz = -2; cz <= 2; cz++) {
			for (int x = 0; x < 16; x++) for (int z = 0; z < 16; z++) {
				cols++;
				bool any = false;
				for (int y = 0; y < 128; y++) {
					int t = aether->getTile(cx * 16 + x, y, cz * 16 + z);
					if (t) { counts[t]++; any = true; }
				}
				if (any) islandCols++;
			}
		}
		printf("  island cover: %d/%d columns (%.1f%%)\n", islandCols, cols, 100.0 * islandCols / cols);
		for (std::map<int, int>::iterator it = counts.begin(); it != counts.end(); ++it) {
			Tile* t = Tile::tiles[it->first];
			printf("  tile %3d %-24s %d\n", it->first, t ? t->getDescriptionId().c_str() : "?", it->second);
		}
		CHECK(islandCols > cols / 10 && islandCols < cols * 6 / 10, "island coverage in sane range");
		CHECK(counts[Aether::holystone->id] > 1000, "holystone generated");
		CHECK(counts[Aether::aetherGrass->id] > 50, "aether grass generated");
		CHECK(counts[Aether::skyrootLog->id] > 0, "skyroot trees generated");
		CHECK(counts[Aether::ambrosiumOre->id] > 0 && counts[Aether::zaniteOre->id] > 0 && counts[Aether::gravititeOre->id] > 0, "ores generated");
		CHECK(counts[Aether::aercloudCold->id] > 0, "aerclouds generated");
		CHECK(counts[Tile::rock->id] == 0 && counts[Tile::dirt->id] == 0 && counts[Tile::grass->id] == 0, "no overworld stone/dirt/grass in Aether");
		CHECK(counts[Tile::water->id] >= 0, "water lakes ok (%d)", counts[Tile::water->id]);

		// Spawn validation / portal creation
		float sx = 0, sy = 64, sz = 0;
		bool made = PortalForcer::findOrCreatePortal(aether, 0, 0, Dimension::AETHER, sx, sy, sz);
		printf("  portal at %.1f %.1f %.1f\n", sx, sy, sz);
		CHECK(made, "Aether portal created / found");
		int px = (int)sx, py = (int)sy, pz = (int)sz;
		bool foundPortalTile = false;
		for (int dx = -3; dx <= 3 && !foundPortalTile; dx++) for (int dy = -3; dy <= 3 && !foundPortalTile; dy++) for (int dz = -3; dz <= 3 && !foundPortalTile; dz++)
			if (aether->getTile(px + dx, py + dy, pz + dz) == Aether::aetherPortal->id) foundPortalTile = true;
		CHECK(foundPortalTile, "portal tile present near spawn point");
		bool solidBelow = false;
		for (int dy = 0; dy <= 6; dy++) if (aether->isSolidBlockingTile(px, py - dy, pz)) solidBelow = true;
		CHECK(solidBelow, "ground beneath portal spawn");

		// Dungeons
		for (int tier = 0; tier < 3; tier++) {
			int dcx, dcz;
			AetherDungeons::dungeonChunk(aether, tier, dcx, dcz);
			printf("  dungeon tier %d at chunk %d,%d\n", tier, dcx, dcz);
			aether->getChunk(dcx, dcz);
			for (int ox = -1; ox <= 1; ox++) for (int oz = -1; oz <= 1; oz++) aether->getChunk(dcx + ox, dcz + oz);
			int want = tier == 0 ? Aether::carvedStone->id : tier == 1 ? Aether::angelicStone->id : Aether::hellfireStone->id;
			int n = 0;
			for (int x = -16; x < 32; x++) for (int z = -16; z < 32; z++) for (int y = 0; y < 128; y++)
				if (aether->getTile(dcx * 16 + x, y, dcz * 16 + z) == want) n++;
			CHECK(n > 100, "dungeon tier %d built (%d stone blocks)", tier, n);
		}

		// Overworld portal from Aether side
		Level* over = makeLevel(GameType::Survival, Dimension::NORMAL);
		PortalForcer::lastTripWasAether = true;
		float ox = 0, oy = 64, oz = 0;
		bool madeOver = PortalForcer::findOrCreatePortal(over, 0, 0, Dimension::NORMAL, ox, oy, oz);
		CHECK(madeOver, "return portal created in overworld at %.0f %.0f %.0f", ox, oy, oz);
		bool foundGlow = false;
		for (int dx = -3; dx <= 3 && !foundGlow; dx++) for (int dy = -3; dy <= 4 && !foundGlow; dy++) for (int dz = -3; dz <= 3 && !foundGlow; dz++)
			if (over->getTile((int)ox + dx, (int)oy + dy, (int)oz + dz) == Aether::aetherPortal->id) foundGlow = true;
		CHECK(foundGlow, "overworld return portal uses Aether portal tile");

		// Freezer / altar recipes via tile entity hooks
		AetherFurnaceTileEntity* fr = new AetherFurnaceTileEntity(TileEntityType::AetherFreezer);
		ItemInstance res = fr->getRecipeResult(Item::bucket_water->id);
		CHECK(!res.isNull() && res.id == Tile::ice->id, "freezer: water bucket -> ice");
		ItemInstance ice(Aether::icestone, 1, 0);
		CHECK(fr->isFuelItem(ice), "freezer accepts icestone fuel");
		AetherFurnaceTileEntity* al = new AetherFurnaceTileEntity(TileEntityType::AetherAltar);
		ItemInstance res2 = al->getRecipeResult(Aether::gravititeOre->id);
		CHECK(!res2.isNull() && res2.id == Aether::enchantedGravitite->id, "altar: gravitite ore -> enchanted gravitite");
		ItemInstance shard(Aether::ambrosiumShard, 1, 0);
		CHECK(al->isFuelItem(shard), "altar accepts ambrosium shard fuel");
		AetherFurnaceTileEntity* inc = new AetherFurnaceTileEntity(TileEntityType::AetherIncubator);
		ItemInstance res3 = inc->getRecipeResult(Aether::moaEgg->id);
		CHECK(!res3.isNull() && res3.id == Item::mobEgg->id && res3.getAuxValue() == MobTypes::Moa, "incubator: moa egg -> moa spawn egg");

		// Mobs
		Mob* moa = MobFactory::CreateMob(MobTypes::Moa, aether);
		Mob* sentry = MobFactory::CreateMob(MobTypes::Sentry, aether);
		Mob* mimic = MobFactory::CreateMob(MobTypes::Mimic, aether);
		Mob* boss = MobFactory::CreateMob(MobTypes::AetherBoss, aether);
		CHECK(moa && sentry && mimic && boss, "MobFactory creates Aether mobs");
	}

	printf("\n%s (%d failures)\n", fails ? "SOME TESTS FAILED" : "ALL TESTS PASSED", fails);
	return fails ? 1 : 0;
}
