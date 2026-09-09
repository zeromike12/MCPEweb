#ifndef NET_MINECRAFT_WORLD_AETHER__AetherTiles_H__
#define NET_MINECRAFT_WORLD_AETHER__AetherTiles_H__

//
// Tile classes used by the Aether dimension. Registered by Aether::initTiles().
//

#include "../level/tile/Tile.h"
#include "../level/tile/EntityTile.h"
#include "../level/tile/Bush.h"
#include "../level/tile/LeafTile.h"
#include "../level/tile/TorchTile.h"
#include "../level/tile/WorkbenchTile.h"
#include "../level/tile/BedTile.h"
#include "../level/tile/ChestTile.h"
#include "../level/tile/DoorTile.h"
#include "../level/tile/TrapDoorTile.h"
#include "../level/tile/StairTile.h"
#include "../level/tile/FenceTile.h"
#include "../level/tile/FenceGateTile.h"
#include "../level/tile/ThinFenceTile.h"
#include "../level/tile/PortalTile.h"
#include "../level/tile/SignTile.h"
#include "../level/tile/TransparentTile.h"
#include "../level/tile/OreTile.h"
#include "../level/tile/StoneSlabTile.h"
#include "../level/material/Material.h"
#include "../level/Level.h"
#include "../level/LevelSource.h"
#include "../entity/Entity.h"
#include "../../util/Random.h"

class Player;
class Mob;

// ----------------------------------------------------------------------
// Simple full blocks
// ----------------------------------------------------------------------

// A plain cube that may have distinct top/side textures (or per-face textures).
class AetherBlock : public Tile {
	typedef Tile super;
public:
	AetherBlock(int id, int tex, const Material* material);
	AetherBlock* setTopBottom(int topTex, int bottomTex);
	int getTexture(int face);
	int getTexture(int face, int data);
protected:
	int topTex, bottomTex;
};

// Aether grass: spreads on Aether dirt, may be fertilized into enchanted grass.
class AetherGrassTile : public AetherBlock {
	typedef AetherBlock super;
public:
	AetherGrassTile(int id, int topTex, int sideTex, bool enchanted);
	int getTexture(LevelSource* level, int x, int y, int z, int face);
	int getTexture(int face, int data);
	void tick(Level* level, int x, int y, int z, Random* random);
	int getResource(int data, Random* random);
	bool use(Level* level, int x, int y, int z, Player* player);
private:
	bool enchanted;
};

// Holystone / bricks / dungeon stones. Drops itself (or a special drop).
class HolystoneTile : public AetherBlock {
	typedef AetherBlock super;
public:
	HolystoneTile(int id, int tex);
	HolystoneTile* setDrop(Tile* drop);
	int getResource(int data, Random* random);
private:
	Tile* drop;
};

// Icestone: freezes adjacent water into ice and lava into obsidian.
class IcestoneTile : public AetherBlock {
	typedef AetherBlock super;
public:
	IcestoneTile(int id, int tex);
	void onPlace(Level* level, int x, int y, int z);
	void neighborChanged(Level* level, int x, int y, int z, int type);
	void tick(Level* level, int x, int y, int z, Random* random);
	static void freezeAround(Level* level, int x, int y, int z);
	static void freezeAt(Level* level, int x, int y, int z);
};

// Quicksoil: extremely slippery, boosts horizontal speed.
class QuicksoilTile : public AetherBlock {
	typedef AetherBlock super;
public:
	QuicksoilTile(int id, int tex);
	void entityInside(Level* level, int x, int y, int z, Entity* entity);
	void stepOn(Level* level, int x, int y, int z, Entity* entity);
};

// Aerclouds: translucent, walk-through-able-ish blocks with special physics.
class AercloudTile : public TransparentTile {
	typedef TransparentTile super;
public:
	enum Kind { Cold, Blue, Golden, Purple, Green, Storm };
	AercloudTile(int id, int tex, Kind kind);
	int getRenderLayer();
	void entityInside(Level* level, int x, int y, int z, Entity* entity);
	void fallOn(Level* level, int x, int y, int z, Entity* entity, float fallDistance);
	void stepOn(Level* level, int x, int y, int z, Entity* entity);
	void animateTick(Level* level, int x, int y, int z, Random* random);
	int getResourceCount(Random* random);
	bool blocksLight();
	int getColor(int data);
	Kind getKind() const { return kind; }
private:
	Kind kind;
};

// Aerogel: transparent, blast resistant, bouncy, absorbs fall damage.
class AerogelTile : public TransparentTile {
	typedef TransparentTile super;
public:
	AerogelTile(int id, int tex);
	int getRenderLayer();
	void fallOn(Level* level, int x, int y, int z, Entity* entity, float fallDistance);
	float getExplosionResistance(Entity* source);
	bool blocksLight();
};

// Skyroot / golden oak trunks.
class AetherLogTile : public AetherBlock {
	typedef AetherBlock super;
public:
	AetherLogTile(int id, int sideTex, int topTex, bool golden);
	int getResource(int data, Random* random);
	int getResourceCount(Random* random);
	void playerDestroy(Level* level, Player* player, int x, int y, int z, int data);
	void onRemove(Level* level, int x, int y, int z);
private:
	bool golden;
};

// Skyroot / golden oak leaves: decay near their own trunk type.
class AetherLeafTile : public LeafTile {
	typedef LeafTile super;
public:
	AetherLeafTile(int id, int tex, bool golden);
	int getColor(LevelSource* level, int x, int y, int z);
	int getColor(int data);
	int getTexture(int face, int data);
	int getResource(int data, Random* random);
	void spawnResources(Level* level, int x, int y, int z, int data, float odds);
	void tick(Level* level, int x, int y, int z, Random* random);
	void onRemove(Level* level, int x, int y, int z);
	void playerDestroy(Level* level, Player* player, int x, int y, int z, int data);
	bool isSolidRender();
	int getRenderLayer();
private:
	bool golden;
	bool hasTrunkNearby(Level* level, int x, int y, int z);
};

// Cross-shaped plants that grow on Aether grass/dirt.
class AetherBush : public Bush {
	typedef Bush super;
public:
	AetherBush(int id, int tex);
protected:
	bool mayPlaceOn(int tile);
};

// Skyroot / golden oak saplings.
class AetherSapling : public AetherBush {
	typedef AetherBush super;
public:
	AetherSapling(int id, int tex, bool golden);
	void tick(Level* level, int x, int y, int z, Random* random);
	void growTree(Level* level, int x, int y, int z, Random* random);
	bool use(Level* level, int x, int y, int z, Player* player);
private:
	bool golden;
};

// Berry bush: data 0 unripe, 1 ripe (drops blueberries).
class BerryBushTile : public AetherBush {
	typedef AetherBush super;
public:
	BerryBushTile(int id, int tex);
	int getTexture(int face, int data);
	void tick(Level* level, int x, int y, int z, Random* random);
	bool use(Level* level, int x, int y, int z, Player* player);
	int getResource(int data, Random* random);
	int getResourceCount(Random* random);
	void spawnResources(Level* level, int x, int y, int z, int data, float odds);
	AABB* getAABB(Level* level, int x, int y, int z);
};

// Ores: ambrosium (drops shards), zanite (drops gems), gravitite (floats).
class AetherOreTile : public OreTile {
	typedef OreTile super;
public:
	enum Kind { Ambrosium, Zanite, Gravitite };
	AetherOreTile(int id, int tex, Kind kind);
	int getResource(int data, Random* random);
	int getResourceCount(Random* random);
	void onPlace(Level* level, int x, int y, int z);
	void neighborChanged(Level* level, int x, int y, int z, int type);
	void tick(Level* level, int x, int y, int z, Random* random);
	int getTickDelay();
private:
	Kind kind;
	void tryFloat(Level* level, int x, int y, int z);
};

// Enchanted gravitite: floats upward when powered, emits a redstone signal.
class EnchantedGravititeTile : public AetherBlock {
	typedef AetherBlock super;
public:
	EnchantedGravititeTile(int id, int tex);
	void neighborChanged(Level* level, int x, int y, int z, int type);
	void tick(Level* level, int x, int y, int z, Random* random);
	bool isSignalSource();
	bool getSignal(LevelSource* level, int x, int y, int z, int dir);
	bool getDirectSignal(Level* level, int x, int y, int z, int dir);
	int getTickDelay();
};

// Dungeon stone: indestructible while the dungeon's boss lives.
class DungeonStoneTile : public HolystoneTile {
	typedef HolystoneTile super;
public:
	DungeonStoneTile(int id, int tex, int dungeonTier, bool light);
	float getDestroyProgress(Player* player);
	float getExplosionResistance(Entity* source);
	int getDungeonTier() const { return dungeonTier; }
private:
	int dungeonTier;
	bool light;
};

// Trapped sentry stone: looks like sentry stone, spawns sentries when stepped on.
class TrappedSentryStoneTile : public DungeonStoneTile {
	typedef DungeonStoneTile super;
public:
	TrappedSentryStoneTile(int id, int tex);
	void stepOn(Level* level, int x, int y, int z, Entity* entity);
	void entityInside(Level* level, int x, int y, int z, Entity* entity);
	void trigger(Level* level, int x, int y, int z);
};

// Locked dungeon stone: opened with the matching key.
class LockedStoneTile : public DungeonStoneTile {
	typedef DungeonStoneTile super;
public:
	LockedStoneTile(int id, int tex, int dungeonTier);
	bool use(Level* level, int x, int y, int z, Player* player);
	float getDestroyProgress(Player* player);
private:
	void unlockConnected(Level* level, int x, int y, int z, int depth);
};

// Pillar: rotated column with side/top textures; data = axis (0 y, 1 x, 2 z).
class PillarTile : public AetherBlock {
	typedef AetherBlock super;
public:
	PillarTile(int id, int sideTex, int topTex);
	int getTexture(int face, int data);
	int getTexture(LevelSource* level, int x, int y, int z, int face);
	int getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue);
};

// ----------------------------------------------------------------------
// Construction
// ----------------------------------------------------------------------

// Half slab with an explicit texture and its own double-slab tile.
class AetherSlabTile : public Tile {
	typedef Tile super;
public:
	AetherSlabTile(int id, int tex, const Material* material, bool fullSize, int halfId, int fullId);
	bool isSolidRender();
	bool isCubeShaped();
	void updateShape(LevelSource* level, int x, int y, int z);
	void updateDefaultShape();
	void addAABBs(Level* level, int x, int y, int z, const AABB* box, std::vector<AABB>& boxes);
	int getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue);
	int getResource(int data, Random* random);
	int getResourceCount(Random* random);
	bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face);
	static const int TOP_SLOT_BIT = 8;
private:
	bool fullSize;
	int halfId, fullId;
};

// Pressure plate: emits a signal while an entity stands on it.
class AetherPressurePlateTile : public Tile {
	typedef Tile super;
public:
	AetherPressurePlateTile(int id, int tex, const Material* material, bool mobsOnly);
	AABB* getAABB(Level* level, int x, int y, int z);
	bool isSolidRender();
	bool isCubeShaped();
	bool blocksLight();
	int getRenderLayer();
	bool mayPlace(Level* level, int x, int y, int z, unsigned char face);
	bool mayPlace(Level* level, int x, int y, int z);
	void neighborChanged(Level* level, int x, int y, int z, int type);
	void tick(Level* level, int x, int y, int z, Random* random);
	void entityInside(Level* level, int x, int y, int z, Entity* entity);
	void stepOn(Level* level, int x, int y, int z, Entity* entity);
	void updateShape(LevelSource* level, int x, int y, int z);
	void updateDefaultShape();
	bool isSignalSource();
	bool getSignal(LevelSource* level, int x, int y, int z, int dir);
	bool getDirectSignal(Level* level, int x, int y, int z, int dir);
	int getTickDelay();
private:
	void checkPressed(Level* level, int x, int y, int z);
	bool mobsOnly;
};

// Button: momentary signal.
class AetherButtonTile : public Tile {
	typedef Tile super;
public:
	AetherButtonTile(int id, int tex);
	AABB* getAABB(Level* level, int x, int y, int z);
	bool isSolidRender();
	bool isCubeShaped();
	bool blocksLight();
	int getRenderLayer();
	bool mayPlace(Level* level, int x, int y, int z, unsigned char face);
	bool mayPlace(Level* level, int x, int y, int z);
	int getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue);
	void neighborChanged(Level* level, int x, int y, int z, int type);
	bool use(Level* level, int x, int y, int z, Player* player);
	void attack(Level* level, int x, int y, int z, Player* player);
	void tick(Level* level, int x, int y, int z, Random* random);
	void updateShape(LevelSource* level, int x, int y, int z);
	void updateDefaultShape();
	bool isSignalSource();
	bool getSignal(LevelSource* level, int x, int y, int z, int dir);
	bool getDirectSignal(Level* level, int x, int y, int z, int dir);
	int getTickDelay();
private:
	void setShapeForData(int data);
};

// Quicksoil glass: transparent AND slippery.
class QuicksoilGlassTile : public TransparentTile {
	typedef TransparentTile super;
public:
	QuicksoilGlassTile(int id, int tex);
	int getRenderLayer();
	int getResourceCount(Random* random);
	void stepOn(Level* level, int x, int y, int z, Entity* entity);
	void entityInside(Level* level, int x, int y, int z, Entity* entity);
};

// Quicksoil glass pane.
class QuicksoilGlassPaneTile : public ThinFenceTile {
	typedef ThinFenceTile super;
public:
	QuicksoilGlassPaneTile(int id, int tex, int edgeTex);
	void entityInside(Level* level, int x, int y, int z, Entity* entity);
};

// Cloudwool: dyeable cloth block; stepping on it slows falls (parachute-ish).
class CloudwoolTile : public Tile {
	typedef Tile super;
public:
	CloudwoolTile(int id, int tex);
	int getColor(int data);
	int getColor(LevelSource* level, int x, int y, int z);
	void fallOn(Level* level, int x, int y, int z, Entity* entity, float fallDistance);
	void entityInside(Level* level, int x, int y, int z, Entity* entity);
protected:
	int getSpawnResourcesAuxValue(int data);
};

// Cloudwool carpet: thin, dyeable.
class CloudwoolCarpetTile : public Tile {
	typedef Tile super;
public:
	CloudwoolCarpetTile(int id, int tex);
	int getColor(int data);
	int getColor(LevelSource* level, int x, int y, int z);
	bool isSolidRender();
	bool isCubeShaped();
	bool blocksLight();
	int getRenderLayer();
	AABB* getAABB(Level* level, int x, int y, int z);
	bool mayPlace(Level* level, int x, int y, int z, unsigned char face);
	bool mayPlace(Level* level, int x, int y, int z);
	void neighborChanged(Level* level, int x, int y, int z, int type);
protected:
	int getSpawnResourcesAuxValue(int data);
};

// Aether banner: tall dyeable decoration (two-thirds cloth block, rendered as a thin cross).
class AetherBannerTile : public Tile {
	typedef Tile super;
public:
	AetherBannerTile(int id, int tex);
	int getColor(int data);
	int getColor(LevelSource* level, int x, int y, int z);
	bool isSolidRender();
	bool isCubeShaped();
	bool blocksLight();
	int getRenderLayer();
	int getRenderShape();
	AABB* getAABB(Level* level, int x, int y, int z);
	bool mayPlace(Level* level, int x, int y, int z, unsigned char face);
	bool mayPlace(Level* level, int x, int y, int z);
	void neighborChanged(Level* level, int x, int y, int z, int type);
protected:
	int getSpawnResourcesAuxValue(int data);
};

// Skyroot door / trapdoor: wooden variants with their own textures.
class SkyrootDoorTile : public DoorTile {
	typedef DoorTile super;
public:
	SkyrootDoorTile(int id);
	int getResource(int data, Random* random);
	void neighborChanged(Level* level, int x, int y, int z, int type);
};

class SkyrootTrapdoorTile : public TrapDoorTile {
	typedef TrapDoorTile super;
public:
	SkyrootTrapdoorTile(int id);
};

// Skyroot fence / fence gate with their own textures.
class SkyrootFenceTile : public FenceTile {
	typedef FenceTile super;
public:
	SkyrootFenceTile(int id, int tex);
};

// ----------------------------------------------------------------------
// Workstations
// ----------------------------------------------------------------------

// Base for the three furnace-like blocks. `kind` selects the fuel / recipes.
class AetherFurnaceTile : public EntityTile {
	typedef EntityTile super;
public:
	enum Kind { AltarKind, FreezerKind, IncubatorKind };
	AetherFurnaceTile(int id, Kind kind, bool lit, int topTex, int sideTex, int litTex);
	int getTexture(int face);
	int getTexture(LevelSource* level, int x, int y, int z, int face);
	bool use(Level* level, int x, int y, int z, Player* player);
	int getResource(int data, Random* random);
	TileEntity* newTileEntity();
	void setPlacedBy(Level* level, int x, int y, int z, Mob* by);
	void onRemove(Level* level, int x, int y, int z);
	void animateTick(Level* level, int x, int y, int z, Random* random);
	Kind getKind() const { return kind; }
	bool isLit() const { return lit; }
	// Swaps the lit/unlit tile while keeping the tile entity.
	static void setLit(Kind kind, bool lit, Level* level, int x, int y, int z);
	static Tile* tileFor(Kind kind, bool lit);
	static int tileEntityTypeFor(Kind kind);
private:
	Kind kind;
	bool lit;
	int topTex, sideTex, litTex;
	static bool noDrop;
};

// Sun altar: right-click cycles the time of day (Aether only; drops from the Sun Spirit).
class SunAltarTile : public AetherBlock {
	typedef AetherBlock super;
public:
	SunAltarTile(int id, int topTex, int sideTex);
	bool use(Level* level, int x, int y, int z, Player* player);
};

// Skyroot crafting table.
class SkyrootWorkbenchTile : public Tile {
	typedef Tile super;
public:
	SkyrootWorkbenchTile(int id, int topTex, int sideTex, int frontTex);
	int getTexture(int face);
	bool use(Level* level, int x, int y, int z, Player* player);
private:
	int topTex, sideTex, frontTex;
};

// Skyroot bed: works like a bed but also in the Aether.
class SkyrootBedTile : public BedTile {
	typedef BedTile super;
public:
	SkyrootBedTile(int id, int tex);
	int getTexture(int face, int data);
	int getResource(int data, Random* random);
	void neighborChanged(Level* level, int x, int y, int z, int type);
	bool use(Level* level, int x, int y, int z, Player* player);
};

// Skyroot chest: a chest with its own textures.
class SkyrootChestTile : public ChestTile {
	typedef ChestTile super;
public:
	SkyrootChestTile(int id, int tex);
	int getTexture(int face);
	int getTexture(LevelSource* level, int x, int y, int z, int face);
	int getResource(int data, Random* random);
};

// Chest mimic: looks like a skyroot chest, attacks when opened.
class ChestMimicTile : public Tile {
	typedef Tile super;
public:
	ChestMimicTile(int id, int tex);
	int getTexture(int face);
	int getTexture(LevelSource* level, int x, int y, int z, int face);
	bool isSolidRender();
	bool isCubeShaped();
	bool use(Level* level, int x, int y, int z, Player* player);
	void attack(Level* level, int x, int y, int z, Player* player);
	int getResourceCount(Random* random);
	void setPlacedBy(Level* level, int x, int y, int z, Mob* by);
	void awaken(Level* level, int x, int y, int z);
};

// Ambrosium torch: the Aether light source (normal torches don't stay lit there).
class AmbrosiumTorchTile : public TorchTile {
	typedef TorchTile super;
public:
	AmbrosiumTorchTile(int id, int tex);
	bool staysLitInAether() { return true; }
};

// Aether portal block: glowstone frame lit with a water bucket.
class AetherPortalTile : public PortalTile {
	typedef PortalTile super;
public:
	AetherPortalTile(int id, int tex);
	void neighborChanged(Level* level, int x, int y, int z, int type);
	void entityInside(Level* level, int x, int y, int z, Entity* entity);
	void animateTick(Level* level, int x, int y, int z, Random* random);
	int getColor(int data);
	static bool trySpawnPortal(Level* level, int x, int y, int z);
	static bool isFrame(int tileId);
};

// Aether sign: a sign made of skyroot (separate tile so the sign tile entity works).
class AetherSignTile : public SignTile {
	typedef SignTile super;
public:
	AetherSignTile(int id, bool onGround);
	int getResource(int data, Random* random);
};

#endif /*NET_MINECRAFT_WORLD_AETHER__AetherTiles_H__*/
