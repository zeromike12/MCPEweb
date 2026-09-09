#include "AetherDimension.h"
#include "Aether.h"
#include "AetherRandomLevelSource.h"
#include "../level/Level.h"
#include "../level/biome/BiomeSource.h"
#include "../level/tile/Tile.h"
#include "../../util/Mth.h"

AetherDimension::AetherDimension()
:	super()
{
	this->id = Dimension::AETHER;
	this->foggy = false;
	this->ultraWarm = false;
	this->hasCeiling = false;
}

AetherDimension::~AetherDimension() {
}

void AetherDimension::init() {
	super::init();
}

ChunkSource* AetherDimension::createRandomLevelSource() {
	return new AetherRandomLevelSource(level, level->getSeed());
}

void AetherDimension::updateLightRamp() {
	// Brighter nights than the overworld: the Aether floats above the clouds.
	float ambientLight = 0.15f;
	for (int i = 0; i <= 15; i++) {
		float v = (1.0f - (float)i / 15.0f);
		brightnessRamp[i] = ((1.0f - v) / (v * 3.0f + 1.0f)) * (1.0f - ambientLight) + ambientLight;
	}
}

bool AetherDimension::isValidSpawn(int x, int z) {
	// Needs an island surface with head room. Scan the island band.
	for (int y = AetherRandomLevelSource::ISLAND_MAX_Y; y >= AetherRandomLevelSource::ISLAND_MIN_Y; y--) {
		int t = level->getTile(x, y, z);
		if (t == Aether::aetherGrass->id || t == Aether::enchantedGrass->id) {
			if (level->isEmptyTile(x, y + 1, z) && level->isEmptyTile(x, y + 2, z)) return true;
		}
	}
	return false;
}

float AetherDimension::getTimeOfDay(long time, float a) {
	int dayStep = (int)(time % Level::TICKS_PER_DAY);
	float td = (dayStep + a) / Level::TICKS_PER_DAY - 0.25f;
	if (td < 0) td += 1;
	if (td > 1) td -= 1;
	float tdo = td;
	td = 1 - (Mth::cos(td * Mth::PI) + 1) * 0.5f;
	return tdo + (td - tdo) / 3.0f;
}

Vec3 AetherDimension::getFogColor(float td, float a) {
	float br = Mth::cos(td * Mth::PI * 2) * 2 + 0.5f;
	if (br < 0) br = 0;
	if (br > 1.f) br = 1.f;
	// pale sky blue / white haze
	float r = 0.78f, g = 0.86f, b = 1.0f;
	r *= br * 0.94f + 0.06f;
	g *= br * 0.94f + 0.06f;
	b *= br * 0.91f + 0.09f;
	return Vec3(r, g, b);
}

bool AetherDimension::mayRespawn() {
	return true;
}

bool AetherDimension::isNaturalDimension() {
	// Player::startSleepInBed() refuses to sleep when this returns true (the
	// check is inverted in this code base), so the Aether says false: beds work.
	return false;
}
