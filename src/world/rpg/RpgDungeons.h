#ifndef NET_MINECRAFT_WORLD_RPG__RpgDungeons_H__
#define NET_MINECRAFT_WORLD_RPG__RpgDungeons_H__

//
// RPG mode dungeons: hand-built structures scattered around the world,
// guarded by persistent high-level mobs and stocked with good loot chests.
//

class Level;

namespace RpgDungeons {

	// Minimum / maximum number of dungeons placed in a new world.
	const int MIN_DUNGEONS = 12;
	const int MAX_DUNGEONS = 16;

	// Generates all dungeons for a freshly created level. Must be called after
	// the terrain has been generated (all chunks present) and before the level
	// is first saved. Does nothing outside RPG mode or on clients.
	void generate(Level* level);
}

#endif /*NET_MINECRAFT_WORLD_RPG__RpgDungeons_H__*/
