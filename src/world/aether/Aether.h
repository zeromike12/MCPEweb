#ifndef NET_MINECRAFT_WORLD_AETHER__Aether_H__
#define NET_MINECRAFT_WORLD_AETHER__Aether_H__

//
// The Aether: a sky dimension made of floating islands, reachable through a
// glowstone portal lit with a water bucket. Available in Survival and Creative
// only - RPG worlds never get the portal (see Aether::isAvailable).
//
// All Aether tiles / items are created by Aether::initTiles() / initItems(),
// which Tile::initTiles() / Item::initItems() call at the end of their own
// registration so the generic TileItem autofill picks the new tiles up.
//

#include <string>

class Level;
class Tile;
class Item;
class LeafTile;
class Recipes;
class FurnaceRecipes;
class Player;
class Random;
class Mob;

namespace Aether {

	// ------------------------------------------------------------------
	// Mode gate
	// ------------------------------------------------------------------
	// True when the Aether may be entered from this level (Survival/Creative).
	bool isAvailable(const Level* level);
	// True when `level` is the Aether dimension itself.
	bool isAetherLevel(const Level* level);

	// ------------------------------------------------------------------
	// Tile ids (160..239 are free in this code base)
	// ------------------------------------------------------------------
	namespace TileId {
		const int AetherGrass       = 160;
		const int EnchantedGrass    = 161;
		const int AetherDirt        = 162;
		const int Holystone         = 163;
		const int MossyHolystone    = 164;
		const int Icestone          = 165;
		const int Quicksoil         = 166;
		const int AercloudCold      = 167;
		const int AercloudBlue      = 168;
		const int AercloudGolden    = 169;
		const int AercloudPurple    = 170;
		const int AercloudGreen     = 171;
		const int AercloudStorm     = 172;
		const int Aerogel           = 173;
		const int SkyrootLog        = 174;
		const int GoldenOakLog      = 175;
		const int SkyrootPlanks     = 176;
		const int SkyrootLeaves     = 177;
		const int GoldenOakLeaves   = 178;
		const int SkyrootSapling    = 179;
		const int GoldenOakSapling  = 180;
		const int WhiteFlower       = 181;
		const int PurpleFlower      = 182;
		const int BerryBush         = 183;
		const int AmbrosiumOre      = 184;
		const int ZaniteOre         = 185;
		const int GravititeOre      = 186;
		const int AmbrosiumBlock    = 187;
		const int ZaniteBlock       = 188;
		const int EnchantedGravitite= 189;
		const int CarvedStone       = 190;
		const int SentryStone       = 191;
		const int LightSentryStone  = 192;
		const int TrappedSentryStone= 193;
		const int AngelicStone      = 194;
		const int LightAngelicStone = 195;
		const int HellfireStone     = 196;
		const int LightHellfireStone= 197;
		const int LockedCarved      = 198;
		const int LockedAngelic     = 199;
		const int LockedHellfire    = 200;
		const int Pillar            = 201;
		const int SkyrootStairs     = 202;
		const int SkyrootSlab       = 203;
		const int SkyrootSlabDouble = 204;
		const int SkyrootFence      = 205;
		const int SkyrootFenceGate  = 206;
		const int SkyrootDoor       = 207;
		const int SkyrootTrapdoor   = 208;
		const int SkyrootPressurePlate = 209;
		const int SkyrootButton     = 210;
		const int HolystoneBricks   = 211;
		const int HolystoneStairs   = 212;
		const int HolystoneSlab     = 213;
		const int HolystoneSlabDouble = 214;
		const int HolystonePressurePlate = 215;
		const int QuicksoilGlass    = 216;
		const int QuicksoilGlassPane= 217;
		const int Cloudwool         = 218;
		const int CloudwoolCarpet   = 219;
		const int AetherBanner      = 220;
		const int AetherSign        = 221;
		const int AetherWallSign    = 222;
		const int Altar             = 223;
		const int AltarLit          = 224;
		const int Freezer           = 225;
		const int FreezerLit        = 226;
		const int Incubator         = 227;
		const int IncubatorLit      = 228;
		const int SunAltar          = 229;
		const int SkyrootWorkbench  = 230;
		const int SkyrootBed        = 231;
		const int ChestMimic        = 232;
		const int SkyrootChest      = 233;
		const int AmbrosiumTorch    = 234;
		const int AetherPortal      = 235;
		const int HolystoneBrickStairs = 236;
	}

	// ------------------------------------------------------------------
	// Item ids (Item(id) stores 256+id). 111..126, 128..148, 151..199 are free.
	// ------------------------------------------------------------------
	namespace ItemId {
		const int AmbrosiumShard  = 111;
		const int ZaniteGem       = 112;
		const int GravititePlate  = 113;
		const int GoldenAmber     = 114;
		const int Blueberry       = 115;
		const int EnchantedBlueberry = 116;
		const int MoaEgg          = 117;
		const int BronzeKey       = 118;
		const int SilverKey       = 119;
		const int GoldKey         = 120;
		const int SkyrootDoor     = 121;
		const int SkyrootBed      = 122;
		const int AetherSign      = 123;
		const int SkyrootPickaxe  = 128;
		const int SkyrootAxe      = 129;
		const int SkyrootShovel   = 130;
		const int SkyrootSword    = 131;
		const int ZanitePickaxe   = 132;
		const int ZaniteAxe       = 133;
		const int ZaniteShovel    = 134;
		const int ZaniteSword     = 135;
		const int GravititePickaxe= 136;
		const int GravititeAxe    = 137;
		const int GravititeShovel = 138;
		const int GravititeSword  = 139;
		const int AetherPortalItem= 140; // creative-only: places a lit portal block
	}

	// ------------------------------------------------------------------
	// Tiles (valid after Tile::initTiles())
	// ------------------------------------------------------------------
	extern Tile* aetherGrass;
	extern Tile* enchantedGrass;
	extern Tile* aetherDirt;
	extern Tile* holystone;
	extern Tile* mossyHolystone;
	extern Tile* icestone;
	extern Tile* quicksoil;
	extern Tile* aercloudCold;
	extern Tile* aercloudBlue;
	extern Tile* aercloudGolden;
	extern Tile* aercloudPurple;
	extern Tile* aercloudGreen;
	extern Tile* aercloudStorm;
	extern Tile* aerogel;
	extern Tile* skyrootLog;
	extern Tile* goldenOakLog;
	extern Tile* skyrootPlanks;
	extern LeafTile* skyrootLeaves;
	extern LeafTile* goldenOakLeaves;
	extern Tile* skyrootSapling;
	extern Tile* goldenOakSapling;
	extern Tile* whiteFlower;
	extern Tile* purpleFlower;
	extern Tile* berryBush;
	extern Tile* ambrosiumOre;
	extern Tile* zaniteOre;
	extern Tile* gravititeOre;
	extern Tile* ambrosiumBlock;
	extern Tile* zaniteBlock;
	extern Tile* enchantedGravitite;
	extern Tile* carvedStone;
	extern Tile* sentryStone;
	extern Tile* lightSentryStone;
	extern Tile* trappedSentryStone;
	extern Tile* angelicStone;
	extern Tile* lightAngelicStone;
	extern Tile* hellfireStone;
	extern Tile* lightHellfireStone;
	extern Tile* lockedCarved;
	extern Tile* lockedAngelic;
	extern Tile* lockedHellfire;
	extern Tile* pillar;
	extern Tile* skyrootStairs;
	extern Tile* skyrootSlab;
	extern Tile* skyrootSlabDouble;
	extern Tile* skyrootFence;
	extern Tile* skyrootFenceGate;
	extern Tile* skyrootDoor;
	extern Tile* skyrootTrapdoor;
	extern Tile* skyrootPressurePlate;
	extern Tile* skyrootButton;
	extern Tile* holystoneBricks;
	extern Tile* holystoneStairs;
	extern Tile* holystoneBrickStairs;
	extern Tile* holystoneSlab;
	extern Tile* holystoneSlabDouble;
	extern Tile* holystonePressurePlate;
	extern Tile* quicksoilGlass;
	extern Tile* quicksoilGlassPane;
	extern Tile* cloudwool;
	extern Tile* cloudwoolCarpet;
	extern Tile* aetherBanner;
	extern Tile* aetherSign;
	extern Tile* aetherWallSign;
	extern Tile* altar;
	extern Tile* altarLit;
	extern Tile* freezer;
	extern Tile* freezerLit;
	extern Tile* incubator;
	extern Tile* incubatorLit;
	extern Tile* sunAltar;
	extern Tile* skyrootWorkbench;
	extern Tile* skyrootBed;
	extern Tile* chestMimic;
	extern Tile* skyrootChest;
	extern Tile* ambrosiumTorch;
	extern Tile* aetherPortal;

	// ------------------------------------------------------------------
	// Items (valid after Item::initItems())
	// ------------------------------------------------------------------
	extern Item* ambrosiumShard;
	extern Item* zaniteGem;
	extern Item* gravititePlate;
	extern Item* goldenAmber;
	extern Item* blueberry;
	extern Item* enchantedBlueberry;
	extern Item* moaEgg;
	extern Item* bronzeKey;
	extern Item* silverKey;
	extern Item* goldKey;
	extern Item* skyrootDoorItem;
	extern Item* skyrootBedItem;
	extern Item* aetherSignItem;
	extern Item* skyrootPickaxe;
	extern Item* skyrootAxe;
	extern Item* skyrootShovel;
	extern Item* skyrootSword;
	extern Item* zanitePickaxe;
	extern Item* zaniteAxe;
	extern Item* zaniteShovel;
	extern Item* zaniteSword;
	extern Item* gravititePickaxe;
	extern Item* gravititeAxe;
	extern Item* gravititeShovel;
	extern Item* gravititeSword;

	// ------------------------------------------------------------------
	// Registration (called from Tile::initTiles / Item::initItems / Recipes)
	// ------------------------------------------------------------------
	void initTiles();
	void initItems();
	void addRecipes(Recipes* recipes);
	void addFurnaceRecipes(FurnaceRecipes* recipes);

	// Creative menu contribution (Inventory::setupDefault)
	class ItemInstanceSink {
	public:
		virtual ~ItemInstanceSink() {}
		virtual void add(int id, int count, int aux) = 0;
	};
	void addCreativeItems(ItemInstanceSink& sink);

	// ------------------------------------------------------------------
	// Tool helpers
	// ------------------------------------------------------------------
	bool isSkyrootTool(int itemId);
	bool isZaniteTool(int itemId);
	bool isGravititeTool(int itemId);
	// Tier of an Aether axe: 0 none, 1 skyroot, 2 zanite, 3 gravitite
	int axeTier(int itemId);

	// ------------------------------------------------------------------
	// Dungeons
	// ------------------------------------------------------------------
	// Dungeon tiers used by the boss mob, keys and locked doors.
	namespace Dungeon {
		const int Bronze = 0;
		const int Silver = 1;
		const int Gold   = 2;
	}
	// True while a living boss of the given tier is close to (x, y, z), which
	// keeps its dungeon indestructible.
	bool isDungeonGuarded(Level* level, int tier, int x, int y, int z);

	// Spawns a mob of the given type at the block position (server side only).
	Mob* spawnMob(Level* level, int mobType, float x, float y, float z);
}

#endif /*NET_MINECRAFT_WORLD_AETHER__Aether_H__*/
