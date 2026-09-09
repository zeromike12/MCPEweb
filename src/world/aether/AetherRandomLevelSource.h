#ifndef NET_MINECRAFT_WORLD_AETHER__AetherRandomLevelSource_H__
#define NET_MINECRAFT_WORLD_AETHER__AetherRandomLevelSource_H__

//
// Chunk generator for the Aether: floating islands of holystone capped with
// Aether grass, dotted with lakes, quicksoil, aerclouds, skyroot / golden oak
// trees, ores and the three dungeon types.
//

#include <unordered_map>
#include <cstdint>
#include "../level/chunk/ChunkSource.h"
#include "../level/levelgen/synth/PerlinNoise.h"
#include "../level/biome/Biome.h"

class Level;
class LevelChunk;

class AetherRandomLevelSource : public ChunkSource {
public:
	static const int CHUNK_HEIGHT = 8;
	static const int CHUNK_WIDTH = 4;
	static const int ISLAND_MIN_Y = 40;
	static const int ISLAND_MAX_Y = 112;

	AetherRandomLevelSource(Level* level, long seed);
	virtual ~AetherRandomLevelSource();

	virtual bool hasChunk(int x, int z);
	virtual LevelChunk* create(int x, int z);
	virtual LevelChunk* getChunk(int x, int z);
	virtual void postProcess(ChunkSource* parent, int xt, int zt);
	virtual bool tick();
	virtual Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z);
	virtual bool shouldSave();
	virtual std::string gatherStats();

private:
	void prepareHeights(int xOffs, int zOffs, unsigned char* blocks);
	void buildSurfaces(int xOffs, int zOffs, unsigned char* blocks);
	float* getHeights(float* buffer, int x, int y, int z, int xSize, int ySize, int zSize);
	void placeDungeons(int xt, int zt);
	void placeCloudLayer(int xo, int zo);

	typedef std::unordered_map<int64_t, LevelChunk*> ChunkMap;
	ChunkMap chunkMap;
	Random random;
	PerlinNoise lperlinNoise1;
	PerlinNoise lperlinNoise2;
	PerlinNoise selectorNoise;
	PerlinNoise islandNoise;   // 2D: which columns belong to islands
	PerlinNoise surfaceNoise;  // 2D: surface detail (quicksoil patches etc.)
	PerlinNoise depthNoise;

	Level* level;
	float* buffer;
	float* pnr;
	float* ar;
	float* br;
	float* sr;
	float* dr;
	float islandBuffer[16 * 16];
	float surfaceBuffer[16 * 16];

	Biome::MobList _aetherCreatures;
	Biome::MobList _aetherMonsters;
	Biome::MobList _emptyMobList;
};

#endif /*NET_MINECRAFT_WORLD_AETHER__AetherRandomLevelSource_H__*/
