#ifndef NET_MINECRAFT_WORLD_LEVEL_PORTAL__PortalForcer_H__
#define NET_MINECRAFT_WORLD_LEVEL_PORTAL__PortalForcer_H__

//package net.minecraft.world.level.portal;

class Level;

class PortalForcer
{
public:
	static bool findPortal(Level* level, int startX, int startY, int startZ, int radius, int& outX, int& outY, int& outZ);
	static bool createPortal(Level* level, int x, int z, int targetDim, int& outX, int& outY, int& outZ);
	static bool findOrCreatePortal(Level* level, int startX, int startZ, int targetDim, float& outX, float& outY, float& outZ);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_PORTAL__PortalForcer_H__*/
