#ifndef NET_MINECRAFT_WORLD_LEVEL_DIMENSION__NetherDimension_H__
#define NET_MINECRAFT_WORLD_LEVEL_DIMENSION__NetherDimension_H__

//package net.minecraft.world.level.dimension;

#include "Dimension.h"
#include "../Level.h"
#include "../../../util/Mth.h"

class NetherDimension: public Dimension
{
	typedef Dimension super;
public:
	NetherDimension();
	virtual ~NetherDimension();

	virtual void init();
	virtual ChunkSource* createRandomLevelSource();
	virtual void updateLightRamp();
	virtual bool isValidSpawn(int x, int z);
	virtual float getTimeOfDay(long time, float a);
	virtual Vec3 getFogColor(float td, float a);
	virtual bool mayRespawn();
	virtual bool isNaturalDimension();
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_DIMENSION__NetherDimension_H__*/
