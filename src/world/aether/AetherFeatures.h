#ifndef NET_MINECRAFT_WORLD_AETHER__AetherFeatures_H__
#define NET_MINECRAFT_WORLD_AETHER__AetherFeatures_H__

//
// World-gen features for the Aether: skyroot / golden oak trees, flower
// patches and berry bushes.
//

#include "../level/levelgen/feature/Feature.h"

class AetherTreeFeature : public Feature {
	typedef Feature super;
public:
	AetherTreeFeature(bool doUpdate, bool golden);
	bool place(Level* level, Random* random, int x, int y, int z);
private:
	bool golden;
};

// Scatters `count` plants of the given tile around (x, y, z) on Aether grass.
class AetherPlantPatchFeature : public Feature {
	typedef Feature super;
public:
	AetherPlantPatchFeature(int tileId, int count);
	bool place(Level* level, Random* random, int x, int y, int z);
private:
	int tileId;
	int count;
};

#endif /*NET_MINECRAFT_WORLD_AETHER__AetherFeatures_H__*/
