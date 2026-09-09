#ifndef NET_MINECRAFT_WORLD_AETHER__AetherDungeons_H__
#define NET_MINECRAFT_WORLD_AETHER__AetherDungeons_H__

//
// The three Aether dungeons:
//   Bronze - carved / sentry stone maze inside an island, guarded by sentries
//            and a Slider-style boss; locked doors need Bronze Keys.
//   Silver - angelic stone temple on top of an island; the Valkyrie Queen
//            waits behind Silver Key doors.
//   Gold   - hellfire stone vault; the Sun Spirit drops the Sun Altar.
//
// Each world gets one of each. Their positions are derived from the level
// seed, so every chunk can ask "does a dungeon start here?" independently.
//

class Level;

namespace AetherDungeons {
	// Called from AetherRandomLevelSource::postProcess for every chunk.
	void placeForChunk(Level* level, int xt, int zt);

	// Chunk coordinates of the dungeon of the given tier for this level.
	void dungeonChunk(Level* level, int tier, int& outXt, int& outZt);
}

#endif /*NET_MINECRAFT_WORLD_AETHER__AetherDungeons_H__*/
