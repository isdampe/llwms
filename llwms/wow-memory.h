#pragma once
#include <stdint.h>

typedef struct TargetObject TargetObject;

struct GameData {
	short status;
	short state;
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
	uint64_t *playerTargetGuid;
	TargetObject *playerTarget;
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
	int type;
	float x;
	float y;
	float z;
};

struct AllGameData {
	PlayerObject *player;
	TargetObject *target;
};
