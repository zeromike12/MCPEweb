#ifndef NET_MINECRAFT_CLIENT_PARTICLE__PortalParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__PortalParticle_H__

//package net.minecraft.client.particle;

#include "Particle.h"
#include "../renderer/Tesselator.h"
#include "../../world/level/Level.h"

class PortalParticle: public Particle
{
	typedef Particle super;
	float oSize;
	float startX;
	float startY;
	float startZ;
public:
	PortalParticle(Level* level, float x, float y, float z, float xa, float ya, float za)
	:	super(level, x, y, z, xa, ya, za)
	{
		this->xd = xa;
		this->yd = ya;
		this->zd = za;
		this->startX = this->x = x;
		this->startY = this->y = y;
		this->startZ = this->z = z;

		float colVar = sharedRandom.nextFloat() * 0.6f + 0.4f;
		this->rCol = colVar * 0.9f;
		this->gCol = colVar * 0.3f;
		this->bCol = colVar * 1.0f;

		this->size = (sharedRandom.nextFloat() * 0.5f + 0.2f) * 1.2f;
		this->oSize = this->size;

		this->lifetime = (int)(Mth::random() * 10.0f) + 20;
		this->noPhysics = true;
		this->tex = (int)(sharedRandom.nextFloat() * 8.0f);
	}

	void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {
		float progress = ((float)age + a) / (float)lifetime;
		progress = 1.0f - progress;
		progress *= progress;
		progress = 1.0f - progress;
		size = oSize * (1.0f - progress * 0.5f);
		super::render(t, a, xa, ya, za, xa2, za2);
	}

	float getBrightness(float a) {
		float b = super::getBrightness(a);
		float progress = (float)age / (float)lifetime;
		progress = progress * progress * progress * progress;
		return b * (1.0f - progress) + progress;
	}

	void tick() {
		xo = x;
		yo = y;
		zo = z;

		float progress = (float)age / (float)lifetime;
		float invProgress = 1.0f - progress;

		if (age++ >= lifetime) {
			remove();
			return;
		}

		x = startX + xd * invProgress;
		y = startY + yd * invProgress;
		z = startZ + zd * invProgress;
	}
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__PortalParticle_H__*/
