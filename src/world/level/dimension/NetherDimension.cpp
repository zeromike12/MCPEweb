#include "NetherDimension.h"
#include "../levelgen/NetherRandomLevelSource.h"
#include "../biome/BiomeSource.h"
#include "../tile/Tile.h"
#include "../../../util/Mth.h"

NetherDimension::NetherDimension()
:	super()
{
	this->id = Dimension::NETHER;
	this->foggy = true;
	this->ultraWarm = true;
	this->hasCeiling = true;
}

NetherDimension::~NetherDimension() {
}

void NetherDimension::init() {
	super::init();
}

ChunkSource* NetherDimension::createRandomLevelSource() {
	return new NetherRandomLevelSource(level, level->getSeed());
}

void NetherDimension::updateLightRamp() {
	float ambientLight = 0.1f;
	for (int i = 0; i <= 15; i++) {
		float v = (1.0f - (float)i / 15.0f);
		brightnessRamp[i] = ((1.0f - v) / (v * 3.0f + 1.0f)) * (1.0f - ambientLight) + ambientLight;
	}
}

bool NetherDimension::isValidSpawn(int x, int z) {
	for (int y = 35; y < 100; y++) {
		int t = level->getTile(x, y, z);
		if (t == Tile::netherrack->id && level->isEmptyTile(x, y + 1, z) && level->isEmptyTile(x, y + 2, z)) {
			return true;
		}
	}
	return false;
}

float NetherDimension::getTimeOfDay(long time, float a) {
	return 0.5f;
}

Vec3 NetherDimension::getFogColor(float td, float a) {
	return Vec3(0.20f, 0.03f, 0.03f);
}

bool NetherDimension::mayRespawn() {
	return false;
}

bool NetherDimension::isNaturalDimension() {
	return false;
}
