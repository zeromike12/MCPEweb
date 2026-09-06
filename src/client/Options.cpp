#include "Options.h"
#include "OptionStrings.h"
#include "Minecraft.h"
#include "renderer/LevelRenderer.h"
#include "../platform/log.h"
#include "../world/Difficulty.h"
#include <cmath>
#include <sstream>
/*static*/
bool Options::debugGl = false;

void Options::initDefaultValues() {
	difficulty = Difficulty::NORMAL;
	hideGui = false;
	thirdPersonView = false;
	renderDebug = false;
	isFlying = false;
	smoothCamera = true;
	fixedCamera = false;
	flySpeed = 1;
	cameraSpeed = 1;
	guiScale = 0;

	useMouseForDigging = false;
	destroyVibration = true;
	isLeftHanded = false;

	isJoyTouchArea = false;

	music = 1;
	sound = 1;
	sensitivity = 0.5f;
	invertYMouse = false;
	viewDistance = 2;
	bobView = true;
	anaglyph3d = false;
	limitFramerate = false;
	fancyGraphics = true;//false;
	ambientOcclusion = false;
	fancyClouds = false;
	if(minecraft->supportNonTouchScreen())
		useTouchScreen = false;
	else
		useTouchScreen = true;
	pixelsPerMillimeter = minecraft->platform()->getPixelsPerMillimeter();
	//useMouseForDigging = true;

	//skin     = "Default";
	username = "Steve";
	serverVisible = true;
	sprintingEnabled = false;

	keyUp	 = KeyMapping("key.forward", Keyboard::KEY_W);
	keyLeft  = KeyMapping("key.left", Keyboard::KEY_A);
	keyDown  = KeyMapping("key.back", Keyboard::KEY_S);
	keyRight = KeyMapping("key.right", Keyboard::KEY_D);
	keyJump  = KeyMapping("key.jump", Keyboard::KEY_SPACE);
	keyBuild = KeyMapping("key.inventory", Keyboard::KEY_E);
	keySneak = KeyMapping("key.sneak", Keyboard::KEY_LSHIFT);
#ifndef RPI
	keyCraft = KeyMapping("key.crafting", Keyboard::KEY_Q);
	keyDrop  = KeyMapping("key.drop", Keyboard::KEY_Q);
	keyChat  = KeyMapping("key.chat", Keyboard::KEY_T);
	keyFog   = KeyMapping("key.fog", Keyboard::KEY_F);
	keyDestroy=KeyMapping("key.destroy", 88); // @todo @fix
	keyUse   = KeyMapping("key.use", Keyboard::KEY_U);
#endif

	//const int Unused = 99999;
	keyMenuNext     = KeyMapping("key.menu.next",     40);
	keyMenuPrevious = KeyMapping("key.menu.previous", 38);
	keyMenuOk       = KeyMapping("key.menu.ok",       13);
	keyMenuCancel   = KeyMapping("key.menu.cancel",   8);

	int k = 0;
	keyMappings[k++] = &keyUp;
	keyMappings[k++] = &keyLeft;
	keyMappings[k++] = &keyDown;
	keyMappings[k++] = &keyRight;
	keyMappings[k++] = &keyJump;
	keyMappings[k++] = &keySneak;
	keyMappings[k++] = &keyDrop;
	keyMappings[k++] = &keyBuild;
	keyMappings[k++] = &keyChat;
	keyMappings[k++] = &keyFog;
	keyMappings[k++] = &keyDestroy;
	keyMappings[k++] = &keyUse;

	keyMappings[k++] = &keyMenuNext;
	keyMappings[k++] = &keyMenuPrevious;
	keyMappings[k++] = &keyMenuOk;
	keyMappings[k++] = &keyMenuCancel;

//	"Polymorphism" at it's worst. At least it's better to have it here
//	for now, then to have it spread all around the game code (even if
//	it would be slightly better performance with it inlined. Should
//  probably create separate subclasses (or read from file). @fix @todo.
#if defined(ANDROID) || defined(__APPLE__) || defined(RPI)
    viewDistance = 2;
    thirdPersonView = false;
	useMouseForDigging = false;
	fancyGraphics = false;

	//renderDebug = true;
	#if !defined(RPI)
		keyUp.key		= 19;
		keyDown.key		= 20;
		keyLeft.key		= 21;
		keyRight.key	= 22;
		keyJump.key		= 23;
		keyUse.key		= 103;
		keyDestroy.key	= 102;
		keyCraft.key    = 109;

		keyMenuNext.key     = 20;
		keyMenuPrevious.key = 19;
		keyMenuOk.key       = 23;
		keyMenuCancel.key   = 4;
	#endif
#endif

#if defined(RPI)
	username = "StevePi";
	sensitivity *= 0.4f;
	useMouseForDigging = true;
#endif
}

const Options::Option
	Options::Option::MUSIC				 (0, "options.music",		true, false),
	Options::Option::SOUND				 (1, "options.sound",		true, false),
	Options::Option::INVERT_MOUSE		 (2, "options.invertMouse",	false, true),
	Options::Option::SENSITIVITY		 (3, "options.sensitivity",	true, false),
	Options::Option::RENDER_DISTANCE	 (4, "options.renderDistance",false, false),
	Options::Option::VIEW_BOBBING		 (5, "options.viewBobbing",	false, true),
	Options::Option::ANAGLYPH			 (6, "options.anaglyph",		false, true),
	Options::Option::LIMIT_FRAMERATE	 (7, "options.limitFramerate",false, true),
	Options::Option::DIFFICULTY			 (8, "options.difficulty",	false, false),
	Options::Option::GRAPHICS			 (9, "options.graphics",		false, true),
	Options::Option::AMBIENT_OCCLUSION	 (10, "options.ao",		false, true),
	Options::Option::GUI_SCALE			 (11, "options.guiScale",	false, false),
	Options::Option::THIRD_PERSON		 (12, "options.thirdperson",	false, true),
    Options::Option::HIDE_GUI			 (13, "options.hidegui",     false, true),
	Options::Option::SERVER_VISIBLE		 (14, "options.servervisible", false, true),
	Options::Option::LEFT_HANDED		 (15, "options.lefthanded", false, true),
	Options::Option::USE_TOUCHSCREEN	 (16, "options.usetouchscreen", false, true),
	Options::Option::USE_TOUCH_JOYPAD	 (17, "options.usetouchpad", false, true),
	Options::Option::DESTROY_VIBRATION   (18, "options.destroyvibration", false, true),
	Options::Option::FANCY_CLOUDS        (20, "options.fancyClouds", false, true),
	Options::Option::PIXELS_PER_MILLIMETER(19, "options.pixelspermilimeter", true, false),
	Options::Option::SPRINTING           (21, "options.sprinting", false, true);

/* private */
const float Options::SOUND_MIN_VALUE = 0.0f;
const float Options::SOUND_MAX_VALUE = 1.0f;
const float Options::MUSIC_MIN_VALUE = 0.0f;
const float Options::MUSIC_MAX_VALUE = 1.0f;
const float Options::SENSITIVITY_MIN_VALUE = 0.0f;
const float Options::SENSITIVITY_MAX_VALUE = 1.0f;
const float Options::PIXELS_PER_MILLIMETER_MIN_VALUE = 3.0f;
const float Options::PIXELS_PER_MILLIMETER_MAX_VALUE = 4.0f;
const int DIFFICULY_LEVELS[] = {
	Difficulty::PEACEFUL,
	Difficulty::NORMAL
};

/*private*/
const char* Options::RENDER_DISTANCE_NAMES[] = {
	"options.renderDistance.far",
	"options.renderDistance.normal",
	"options.renderDistance.short",
	"options.renderDistance.tiny"
};

/*private*/
const char* Options::DIFFICULTY_NAMES[] = {
	"options.difficulty.peaceful",
	"options.difficulty.easy",
	"options.difficulty.normal",
	"options.difficulty.hard"
};

/*private*/
const char* Options::GUI_SCALE[] = {
	"options.guiScale.auto",
	"options.guiScale.small",
	"options.guiScale.normal",
	"options.guiScale.large"
};

void Options::update()
{
	viewDistance = 2;
	StringVector optionLines = optionsFile.getOptionStrings();
	for (unsigned int i = 0; i < optionLines.size(); i++) {
		const std::string& line = optionLines[i];
		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos) continue;
		
		std::string key = Util::stringTrim(line.substr(0, colonPos));
		std::string value = Util::stringTrim(line.substr(colonPos + 1));

		// Multiplayer
		if (key == OptionStrings::Multiplayer_Username) username = value;
		if (key == OptionStrings::Multiplayer_ServerVisible) readBool(value, serverVisible);

		// Controls
		if (key == OptionStrings::Controls_Sensitivity) {
			float sens;
			if (readFloat(value, sens)) {
				sensitivity = 0.3f + std::pow(1.1f * sens, 1.3f) * 0.42f;
			}
		}
		if (key == OptionStrings::Controls_InvertMouse) {
			readBool(value, invertYMouse);
		}
		if (key == OptionStrings::Controls_IsLefthanded) {
			readBool(value, isLeftHanded);
		}
		if (key == OptionStrings::Controls_UseTouchJoypad) {
			readBool(value, isJoyTouchArea);
			if (!minecraft->useTouchscreen())
				isJoyTouchArea = false;
		}

		// Feedback
		if (key == OptionStrings::Controls_FeedbackVibration)
			readBool(value, destroyVibration);

		// Graphics
		if (key == OptionStrings::Graphics_Fancy) {
			readBool(value, fancyGraphics);
		}
		if (key == OptionStrings::Graphics_LowQuality) {
			bool isLow;
			readBool(value, isLow);
			if (isLow) {
				viewDistance = 3;
				fancyGraphics = false;
			}
		}
		if (key == OptionStrings::Graphics_AmbientOcclusion) {
			readBool(value, ambientOcclusion);
			Minecraft::useAmbientOcclusion = ambientOcclusion;
		}
		if (key == OptionStrings::Graphics_FancyClouds) {
			readBool(value, fancyClouds);
		}
		// Game
		if (key == OptionStrings::Game_DifficultyLevel) {
			readInt(value, difficulty);
			// Only support peaceful and normal right now
			if (difficulty != Difficulty::PEACEFUL && difficulty != Difficulty::NORMAL)
				difficulty = Difficulty::NORMAL;
		}
		if (key == OptionStrings::Game_SprintingEnabled) {
			readBool(value, sprintingEnabled);
		}
	}
    
#ifdef __APPLE__
//    if (minecraft->isSuperFast()) {
//        viewDistance = (viewDistance>0)? --viewDistance : 0;
//    }
//    LOGI("Is this card super fast?: %d\n", viewDistance);
#endif
    
    //LOGI("Lefty is: %d\n", isLeftHanded);
}

void Options::load()
{
	int a = 0;
	//try {
	//    if (!optionsFile.exists()) return;
	//    BufferedReader br = /*new*/ BufferedReader(/*new*/ FileReader(optionsFile));
	//    std::string line = "";
	//    while ((line = br.readLine()) != NULL) {
	//        std::string[] cmds = line.split(":");
	//        if (cmds[0].equals("music")) music = readFloat(cmds[1]);
	//        if (cmds[0].equals("sound")) sound = readFloat(cmds[1]);
	//        if (cmds[0].equals("mouseSensitivity")) sensitivity = readFloat(cmds[1]);
	//        if (cmds[0].equals("invertYMouse")) invertYMouse = cmds[1].equals("true");
	//        if (cmds[0].equals("viewDistance")) viewDistance = Integer.parseInt(cmds[1]);
	//        if (cmds[0].equals("guiScale")) guiScale = Integer.parseInt(cmds[1]);
	//        if (cmds[0].equals("bobView")) bobView = cmds[1].equals("true");
	//        if (cmds[0].equals("anaglyph3d")) anaglyph3d = cmds[1].equals("true");
	//        if (cmds[0].equals("limitFramerate")) limitFramerate = cmds[1].equals("true");
	//        if (cmds[0].equals("difficulty")) difficulty = Integer.parseInt(cmds[1]);
	//        if (cmds[0].equals("fancyGraphics")) fancyGraphics = cmds[1].equals("true");
	//        if (cmds[0].equals("ao")) ambientOcclusion = cmds[1].equals("true");
	//        if (cmds[0].equals("skin")) skin = cmds[1];
	//        if (cmds[0].equals("lastServer") && cmds.length >= 2) lastMpIp = cmds[1];

	//        for (int i = 0; i < keyMappings.length; i++) {
	//            if (cmds[0].equals("key_" + keyMappings[i].name)) {
	//                keyMappings[i].key = Integer.parseInt(cmds[1]);
	//            }
	//        }
	//    }
	//    br.close();
	//} catch (Exception e) {
	//    System.out.println("Failed to load options");
	//    e.printStackTrace();
	//}
}

void Options::save()
{
	StringVector stringVec;
	// Game
	addOptionToSaveOutput(stringVec, OptionStrings::Multiplayer_ServerVisible, serverVisible);
	addOptionToSaveOutput(stringVec, OptionStrings::Game_DifficultyLevel, difficulty);
	addOptionToSaveOutput(stringVec, OptionStrings::Game_SprintingEnabled, sprintingEnabled);

	// Input
	addOptionToSaveOutput(stringVec, OptionStrings::Controls_InvertMouse, invertYMouse);
	addOptionToSaveOutput(stringVec, OptionStrings::Controls_Sensitivity, sensitivity);
	addOptionToSaveOutput(stringVec, OptionStrings::Controls_IsLefthanded, isLeftHanded);
	addOptionToSaveOutput(stringVec, OptionStrings::Controls_UseTouchScreen, useTouchScreen);
	addOptionToSaveOutput(stringVec, OptionStrings::Controls_UseTouchJoypad, isJoyTouchArea);
	addOptionToSaveOutput(stringVec, OptionStrings::Controls_FeedbackVibration, destroyVibration);

	// Graphics
	addOptionToSaveOutput(stringVec, OptionStrings::Graphics_Fancy, fancyGraphics);
	addOptionToSaveOutput(stringVec, OptionStrings::Graphics_AmbientOcclusion, ambientOcclusion);
	addOptionToSaveOutput(stringVec, OptionStrings::Graphics_FancyClouds, fancyClouds);

	optionsFile.save(stringVec);
}
void Options::addOptionToSaveOutput(StringVector& stringVector, std::string name, bool boolValue) {
	std::stringstream ss;
	ss << name << ":" << boolValue;
	stringVector.push_back(ss.str());
}
void Options::addOptionToSaveOutput(StringVector& stringVector, std::string name, float floatValue) {
	std::stringstream ss;
	ss << name << ":" << floatValue;
	stringVector.push_back(ss.str());
}
void Options::addOptionToSaveOutput(StringVector& stringVector, std::string name, int intValue) {
	std::stringstream ss;
	ss << name << ":" << intValue;
	stringVector.push_back(ss.str());
}

std::string Options::getMessage( const Option* item )
{
	return "Options::getMessage - Not implemented";

	//Language language = Language.getInstance();
	//std::string caption = language.getElement(item.getCaptionId()) + ": ";

	//if (item.isProgress()) {
	//    float progressValue = getProgressValue(item);

	//    if (item == Option.SENSITIVITY) {
	//        if (progressValue == 0) {
	//            return caption + language.getElement("options.sensitivity.min");
	//        }
	//        if (progressValue == 1) {
	//            return caption + language.getElement("options.sensitivity.max");
	//        }
	//        return caption + (int) (progressValue * 200) + "%";
	//    } else {
	//        if (progressValue == 0) {
	//            return caption + language.getElement("options.off");
	//        }
	//        return caption + (int) (progressValue * 100) + "%";
	//    }
	//} else if (item.isBoolean()) {

	//    bool booleanValue = getBooleanValue(item);
	//    if (booleanValue) {
	//        return caption + language.getElement("options.on");
	//    }
	//    return caption + language.getElement("options.off");
	//} else if (item == Option.RENDER_DISTANCE) {
	//    return caption + language.getElement(RENDER_DISTANCE_NAMES[viewDistance]);
	//} else if (item == Option.DIFFICULTY) {
	//    return caption + language.getElement(DIFFICULTY_NAMES[difficulty]);
	//} else if (item == Option.GUI_SCALE) {
	//    return caption + language.getElement(GUI_SCALE[guiScale]);
	//} else if (item == Option.GRAPHICS) {
	//    if (fancyGraphics) {
	//        return caption + language.getElement("options.graphics.fancy");
	//    }
	//    return caption + language.getElement("options.graphics.fast");
	//}

	//return caption;
}

/*static*/
bool Options::readFloat(const std::string& string, float& value) {
	if (string == "true" || string == "YES")  { value = 1; return true; }
	if (string == "false" || string == "NO") { value = 0; return true; }
#ifdef _WIN32
	if (sscanf_s(string.c_str(), "%f", &value))
		return true;
#else
	if (sscanf(string.c_str(), "%f", &value))
		return true;
#endif
	return false;
}

/*static*/
bool Options::readInt(const std::string& string, int& value) {
	if (string == "true" || string == "YES")  { value = 1; return true; }
	if (string == "false" || string == "NO") { value = 0; return true; }
#ifdef _WIN32
	if (sscanf_s(string.c_str(), "%d", &value))
		return true;
#else
	if (sscanf(string.c_str(), "%d", &value))
		return true;
#endif
	return false;
}

/*static*/
bool Options::readBool(const std::string& string, bool& value) {
	std::string s = Util::stringTrim(string);
	if (string == "true" || string == "1" || string == "YES")  { value = true;  return true; }
	if (string == "false" || string == "0" || string == "NO") { value = false; return true; }
	return false;
}

void Options::toggle(const Option* option, int dir) {
	if (option == &Option::INVERT_MOUSE)	invertYMouse = !invertYMouse;
	if (option == &Option::RENDER_DISTANCE) viewDistance = (viewDistance + dir) & 3;
	if (option == &Option::GUI_SCALE)		guiScale = (guiScale + dir) & 3;
	if (option == &Option::VIEW_BOBBING)	bobView = !bobView;
	if (option == &Option::THIRD_PERSON)	thirdPersonView = !thirdPersonView;
	if (option == &Option::HIDE_GUI)		hideGui = !hideGui;
	if (option == &Option::SERVER_VISIBLE)		serverVisible = !serverVisible;
	if (option == &Option::LEFT_HANDED) isLeftHanded = !isLeftHanded;
	if (option == &Option::USE_TOUCHSCREEN) useTouchScreen = !useTouchScreen;
	if (option == &Option::USE_TOUCH_JOYPAD) isJoyTouchArea = !isJoyTouchArea;
	if (option == &Option::DESTROY_VIBRATION) destroyVibration = !destroyVibration;
	if (option == &Option::ANAGLYPH) {
		anaglyph3d = !anaglyph3d;
		//minecraft->textures.reloadAll();
	}
	if (option == &Option::LIMIT_FRAMERATE) limitFramerate = !limitFramerate;
	if (option == &Option::DIFFICULTY) difficulty = (difficulty + dir) & 3;
	if (option == &Option::GRAPHICS) {
		fancyGraphics = !fancyGraphics;
		if (minecraft->levelRenderer) minecraft->levelRenderer->allChanged();
	}
	if (option == &Option::AMBIENT_OCCLUSION) {
		ambientOcclusion = !ambientOcclusion;
		Minecraft::useAmbientOcclusion = ambientOcclusion;
		if (minecraft->levelRenderer) minecraft->levelRenderer->allChanged();
	}
	if (option == &Option::FANCY_CLOUDS) {
		fancyClouds = !fancyClouds;
	}
	if (option == &Option::SPRINTING) {
		sprintingEnabled = !sprintingEnabled;
	}
	notifyOptionUpdate(option, getBooleanValue(option));
	save();
}

void Options::notifyOptionUpdate( const Option* option, bool value ) {
	minecraft->optionUpdated(option, value);
}

void Options::notifyOptionUpdate( const Option* option, float value ) {
	minecraft->optionUpdated(option, value);
}

void Options::notifyOptionUpdate( const Option* option, int value ) {
	minecraft->optionUpdated(option, value);
}
