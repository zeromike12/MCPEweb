#ifndef NET_MINECRAFT_WORLD_LEVEL__LevelSettings_H__
#define NET_MINECRAFT_WORLD_LEVEL__LevelSettings_H__

//package net.minecraft.world.level;

namespace GameType {
	const int Undefined = -1;
	const int Survival = 0;
	const int Creative = 1;
	/// Survival rules plus player/mob levels, item modifiers and loot chests.
	const int RPG = 2;

	const int Default = Creative;

	/// True for every mode that uses survival rules (health, hunger, no instabuild).
	inline bool isSurvivalLike(int gameType) {
		return gameType == Survival || gameType == RPG;
	}
}

class LevelSettings
{
public:
    LevelSettings(long seed, int gameType)
    :   seed(seed),
        gameType(gameType)
    {
    }
	static LevelSettings None() {
		return LevelSettings(-1,-1);
	}

    long getSeed() const {
        return seed;
    }

    int getGameType() const {
        return gameType;
    }

	//
	// Those two should actually not be here
	// @todo: Move out when we add LevelSettings.cpp :p
	//
	static int validateGameType(int gameType) {
        switch (gameType) {
		case GameType::Creative:
		case GameType::Survival:
		case GameType::RPG:
            return gameType;
        }
        return GameType::Default;
    }

	static std::string gameTypeToString(int gameType) {
		if (gameType == GameType::Survival) return "Survival";
		if (gameType == GameType::Creative) return "Creative";
		if (gameType == GameType::RPG) return "RPG";
		return "Undefined";
	}

private:
    long seed;
    int gameType;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL__LevelSettings_H__*/
