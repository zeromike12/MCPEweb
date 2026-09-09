#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__AetherRenderers_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__AetherRenderers_H__

//
// Renderers for the Aether mobs. They reuse existing models scaled to the
// mob's size (the skins in data/images/mob/{moa,sentry,mimic}.png follow the
// chicken / zombie UV layouts).
//

#include "MobRenderer.h"
#include "HumanoidMobRenderer.h"

class Mob;
class Model;
class HumanoidModel;

// Moa: chicken model, 2.3x, flapping bob.
class MoaRenderer : public MobRenderer {
	typedef MobRenderer super;
public:
	MoaRenderer(Model* model, float shadow);
protected:
	float getBob(Mob* mob, float a);
	void scale(Mob* mob, float a);
};

// Sentry: creeper-like body (cube on legs), tinted by the sentry skin.
class SentryRenderer : public MobRenderer {
	typedef MobRenderer super;
public:
	SentryRenderer(Model* model, float shadow);
protected:
	void scale(Mob* mob, float a);
};

// Mimic: humanoid model with the mimic (chest-coloured) skin.
class MimicRenderer : public HumanoidMobRenderer {
	typedef HumanoidMobRenderer super;
public:
	MimicRenderer(HumanoidModel* model, float shadow);
protected:
	void scale(Mob* mob, float a);
};

// Boss: humanoid model scaled by tier.
class AetherBossRenderer : public HumanoidMobRenderer {
	typedef HumanoidMobRenderer super;
public:
	AetherBossRenderer(HumanoidModel* model, float shadow);
protected:
	void scale(Mob* mob, float a);
	void renderName(Mob* mob, float x, float y, float z);
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__AetherRenderers_H__*/
