#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__NetherRandomLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__NetherRandomLevelSource_H__

//package net.minecraft.world.level.levelgen;

#include <unordered_map>
#include <cstdint>
#include "../chunk/ChunkSource.h"
#include "LargeCaveFeature.h"
#include "synth/PerlinNoise.h"
#include "../../../SharedConstants.h"
#include "../biome/Biome.h"

class Level;
class LevelChunk;

typedef std::unordered_map<int64_t, LevelChunk*> ChunkMap;

class NetherRandomLevelSource: public ChunkSource
{
public:
	static const int CHUNK_HEIGHT = 8;
	static const int CHUNK_WIDTH = 4;

	NetherRandomLevelSource(Level* level, long seed);
	virtual ~NetherRandomLevelSource();

	virtual bool hasChunk(int x, int y);
	virtual LevelChunk* create(int x, int z);
	virtual LevelChunk* getChunk(int xOffs, int zOffs);

	void prepareHeights(int xOffs, int zOffs, unsigned char* blocks);
	void buildSurfaces(int xOffs, int zOffs, unsigned char* blocks);
	virtual void postProcess(ChunkSource* parent, int xt, int zt);

	virtual bool tick();
	virtual Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z);
	virtual bool shouldSave();
	virtual std::string gatherStats();

private:
	float* getHeights(float* buffer, int x, int y, int z, int xSize, int ySize, int zSize);

public:
	LargeCaveFeature caveFeature;

private:
	ChunkMap chunkMap;
	Random random;
	PerlinNoise lperlinNoise1;
	PerlinNoise lperlinNoise2;
	PerlinNoise perlinNoise1;
	PerlinNoise perlinNoise2;
	PerlinNoise perlinNoise3;
	PerlinNoise scaleNoise;
	PerlinNoise depthNoise;

	Level* level;

	float* buffer;
	float soulSandBuffer[16 * 16];
	float gravelBuffer[16 * 16];
	float depthBuffer[16 * 16];
	float* pnr;
	float* ar;
	float* br;
	float* sr;
	float* dr;

	Biome::MobList _netherEnemies;
	Biome::MobList _emptyMobList;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__NetherRandomLevelSource_H__*/
