#pragma once
#include <stdint.h>

typedef struct TargetObject TargetObject;

struct GameData {
	short status;
	short state;
	short isConnected;
	int zone;
	uint64_t mouseOverGuid;
};

struct PlayerObject {
	char *name;
	int classId;
	int level;
	float x;
	float y;
	float z;
	float rotation;
	long health;
	long maxHealth;
	long mana;
	long maxMana;
	uint64_t targetGuid;
	TargetObject *target;
};

struct PlayerAddresses {

};

struct GameObject {
	uint64_t guid;
	unsigned int type;
	unsigned int displayId;
};

struct TargetObject {
	char *name;
	int classId;
	int level;
	float x;
	float y;
	float z;
	float rotation;
	long health;
	long maxHealth;
	long mana;
	long maxMana;
};

struct GatherObject {
	uint64_t guid;
	time_t lastSeen;
	unsigned int type;
	float x;
	float y;
	float z;
	GatherObject *nextObject;
};

struct AllGameData {
	PlayerObject *player;
	TargetObject *target;
	GatherObject *firstGatherObject;
};

#define WOW_GO_REMOVE_INACTIVE_SECONDS 5