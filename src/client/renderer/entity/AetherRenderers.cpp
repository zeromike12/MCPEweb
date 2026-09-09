#include "AetherRenderers.h"
#include "../gles.h"
#include "../../model/Model.h"
#include "../../model/HumanoidModel.h"
#include "../../../world/entity/animal/AetherMobs.h"
#include "../../../util/Mth.h"

// ----------------------------------------------------------------------
MoaRenderer::MoaRenderer(Model* model, float shadow)
:	super(model, shadow)
{
}

float MoaRenderer::getBob(Mob* mob_, float a) {
	Moa* mob = (Moa*)mob_;
	float flap = mob->oFlap + (mob->flap - mob->oFlap) * a;
	float flapSpeed = mob->oFlapSpeed + (mob->flapSpeed - mob->oFlapSpeed) * a;
	return (Mth::sin(flap) + 1) * flapSpeed;
}

void MoaRenderer::scale(Mob* mob, float a) {
	const float s = 2.3f;
	glScalef2(s, s, s);
}

// ----------------------------------------------------------------------
SentryRenderer::SentryRenderer(Model* model, float shadow)
:	super(model, shadow)
{
}

void SentryRenderer::scale(Mob* mob, float a) {
	// squash into a cube-ish guardian
	glScalef2(1.3f, 0.75f, 1.3f);
}

// ----------------------------------------------------------------------
MimicRenderer::MimicRenderer(HumanoidModel* model, float shadow)
:	super(model, shadow)
{
}

void MimicRenderer::scale(Mob* mob, float a) {
	glScalef2(1.1f, 0.6f, 1.1f);
}

// ----------------------------------------------------------------------
AetherBossRenderer::AetherBossRenderer(HumanoidModel* model, float shadow)
:	super(model, shadow)
{
}

void AetherBossRenderer::scale(Mob* mob, float a) {
	AetherBoss* boss = (AetherBoss*)mob;
	switch (boss->getTier()) {
	case 0: glScalef2(1.9f, 0.9f, 1.9f); break;   // Slider: a wide block
	case 1: glScalef2(1.0f, 1.15f, 1.0f); break;  // Valkyrie Queen: tall
	default: glScalef2(1.6f, 1.6f, 1.6f); break;  // Sun Spirit: big
	}
}

void AetherBossRenderer::renderName(Mob* mob, float x, float y, float z) {
	AetherBoss* boss = (AetherBoss*)mob;
	const char* name = boss->getTier() == 0 ? "Slider" : (boss->getTier() == 1 ? "Valkyrie Queen" : "Sun Spirit");
	renderNameTag(mob, name, x, y + mob->bbHeight + 0.3f, z, 32);
}
