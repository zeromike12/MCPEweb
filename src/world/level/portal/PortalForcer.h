#ifndef NET_MINECRAFT_WORLD_LEVEL_PORTAL__PortalForcer_H__
#define NET_MINECRAFT_WORLD_LEVEL_PORTAL__PortalForcer_H__

//package net.minecraft.world.level.portal;

class Level;

class PortalForcer
{
public:
	static bool findPortal(Level* level, int startX, int startY, int startZ, int radius, int& outX, int& outY, int& outZ);
	static bool findPortal(Level* level, int startX, int startY, int startZ, int radius, int& outX, int& outY, int& outZ, int portalTileId);
	// Set by Minecraft::switchDimension when the player leaves the Aether, so the trip
	// back to the overworld looks for / builds an Aether portal instead of a Nether one.
	static bool lastTripWasAether;
	static bool createPortal(Level* level, int x, int z, int targetDim, int& outX, int& outY, int& outZ);
	static bool findOrCreatePortal(Level* level, int startX, int startZ, int targetDim, float& outX, float& outY, float& outZ);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_PORTAL__PortalForcer_H__*/
