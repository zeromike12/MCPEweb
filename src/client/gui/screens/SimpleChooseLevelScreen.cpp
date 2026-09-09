#include "SimpleChooseLevelScreen.h"
#include "ProgressScreen.h"
#include "ScreenChooser.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include "../../../world/level/LevelSettings.h"
#include "../../../platform/time.h"

SimpleChooseLevelScreen::SimpleChooseLevelScreen(const std::string& levelName)
:	bCreative(0),
	bSurvival(0),
	bRpg(0),
	bBack(0),
	levelName(levelName),
	hasChosen(false)
{
}

SimpleChooseLevelScreen::~SimpleChooseLevelScreen()
{
	delete bCreative;
	delete bSurvival;
	delete bRpg;
	delete bBack;
}

void SimpleChooseLevelScreen::init()
{
	if (minecraft->useTouchscreen()) {
		bCreative = new Touch::TButton(1, "Creative mode");
		bSurvival = new Touch::TButton(2, "Survival mode");
		bRpg      = new Touch::TButton(4, "RPG mode");
		bBack	  = new Touch::TButton(3, "Back");
	} else {
		bCreative = new Button(1, "Creative mode");
		bSurvival = new Button(2, "Survival mode");
		bRpg      = new Button(4, "RPG mode");
		bBack	  = new Button(3, "Back");
	}
	buttons.push_back(bCreative);
	buttons.push_back(bSurvival);
	buttons.push_back(bRpg);
	buttons.push_back(bBack);

	tabButtons.push_back(bCreative);
	tabButtons.push_back(bSurvival);
	tabButtons.push_back(bRpg);
	tabButtons.push_back(bBack);
}

void SimpleChooseLevelScreen::setupPositions()
{
	bCreative->width = bSurvival->width = bRpg->width = bBack->width = 120;
	// Three evenly spaced mode buttons above the Back button
	int spacing = (height - 40) / 4;
	bCreative->x = (width - bCreative->width) / 2;
	bCreative->y = spacing - 30;
	bSurvival->x = (width - bSurvival->width) / 2;
	bSurvival->y = 2*spacing - 30;
	bRpg->x = (width - bRpg->width) / 2;
	bRpg->y = 3*spacing - 30;
	bBack->x = bSurvival->x + bSurvival->width - bBack->width;
	bBack->y = height - 40;
}

void SimpleChooseLevelScreen::render( int xm, int ym, float a )
{
	renderDirtBackground(0);
    glEnable2(GL_BLEND);

	drawCenteredString(minecraft->font, "Mobs, health and gather resources", width/2, bSurvival->y + bSurvival->height + 4, 0xffcccccc);
	drawCenteredString(minecraft->font, "Unlimited resources and flying", width/2, bCreative->y + bCreative->height + 4, 0xffcccccc);
	drawCenteredString(minecraft->font, "Survival with levels, loot and modifiers", width/2, bRpg->y + bRpg->height + 4, 0xffcccccc);

	Screen::render(xm, ym, a);
    glDisable2(GL_BLEND);
}

void SimpleChooseLevelScreen::buttonClicked( Button* button )
{
	if (button == bBack) {
		minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
		return;
	}
	if (hasChosen)
		return;

	int gameType = GameType::Survival;

	if (button == bCreative)
		gameType = GameType::Creative;

	if (button == bSurvival)
		gameType = GameType::Survival;

	if (button == bRpg)
		gameType = GameType::RPG;

	std::string levelId = getUniqueLevelName(levelName);
	LevelSettings settings(getEpochTimeS(), gameType);
#ifdef __EMSCRIPTEN__
	// Show ProgressScreen first, then defer level creation to next frame so it can render
	// before we block on generation. prepareLevel will yield to allow progress updates.
	minecraft->setScreen(new ProgressScreen());
	minecraft->scheduleLevelCreation(levelId, levelId, settings.getSeed(), gameType);
#else
	minecraft->selectLevel(levelId, levelId, settings);
	minecraft->hostMultiplayer();
	minecraft->setScreen(new ProgressScreen());
#endif
	hasChosen = true;
}

bool SimpleChooseLevelScreen::handleBackEvent(bool isDown) {
	if (!isDown)
		minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
	return true;
}
