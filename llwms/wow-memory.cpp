#include "stdafx.h"
#include "wow-memory.h"
#include "wow-offsets.h"
#include <iostream>
#include <windows.h>
#include <psapi.h>
#include <ctime>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

class WowMemory {
public:
	//Methods
	WowMemory();
	void Critical(char *message);
	void ComputeMemoryAddresses();
	void Update();
	void ReadEssentialData();
	void ReadGameData();
	void ReadPlayerData();
	void ReadObjectData();
	void ReadTargetData(DWORD_PTR objAddress);
	void PrependMinableObject(DWORD_PTR objAddress);
	void GarbageCollector();
	DWORD_PTR ProcGetBaseAddress(DWORD processId, HANDLE *processHandle);

	//Data
	HWND wHnd;
	HANDLE processHnd;
	DWORD processId;
	DWORD_PTR addressProcessBase, addressGameState, addressGameZone, addressPlayer,
			  addressTargetGuid, addressObjMgr, addressMouseOverGuid, addressIsConnected;
	GameData game;
	PlayerObject player;
	TargetObject target;
	GatherObject *firstGatherObject;
};

/*
 * Constructor function.
*/
inline WowMemory::WowMemory() {
	
	//Try to open game process.
	this->wHnd = FindWindow(nullptr, L"World of Warcraft");
	GetWindowThreadProcessId(this->wHnd, &this->processId);
	this->processHnd = OpenProcess(PROCESS_ALL_ACCESS, FALSE, this->processId);
	if (this->processHnd == 0) {
		this->Critical("Could not open process.");
		return;
	}

	//Reset the linked list.
	this->firstGatherObject = 0x0;
	this->ComputeMemoryAddresses();

}

inline void WowMemory::Critical(char *message) {
	std::cout << message << "\n";
	throw message;
}

inline void WowMemory::ComputeMemoryAddresses() {

	//Get the process base address.
	this->addressProcessBase = this->ProcGetBaseAddress(this->processId, &this->processHnd);

	DWORD_PTR player = (this->addressProcessBase) + WOW_MEM_PLAYER_BASE;
	DWORD_PTR objMgr = (this->addressProcessBase) + WOW_MEM_OBJMGR_BASE;

	//Player pointer.
	ReadProcessMemory(this->processHnd, (void *)(player), &this->addressPlayer, sizeof(this->addressPlayer), 0);
	ReadProcessMemory(this->processHnd, (void *)(objMgr), &this->addressObjMgr, sizeof(this->addressObjMgr), 0);

	//Rest are static.
	this->addressGameState = (this->addressProcessBase) + WOW_MEM_GAME_STATE;
	this->addressIsConnected = (this->addressProcessBase) + WOW_MEM_IS_CONNECTED;
	this->addressGameZone = (this->addressProcessBase) + WOW_MEM_ZONE;
	this->addressTargetGuid = (this->addressProcessBase) + WOW_MEM_PLAYER_TARGET_GUID;
	this->addressMouseOverGuid = (this->addressProcessBase) + WOW_MEM_MOUSEOVER_GUID;
	
}

inline void WowMemory::Update() {

	//Read game state, isConnected
	this->ReadEssentialData();
	
	//Check for dynamic recalculation requirement, i.e. user signed out and back in.
	if (this->game.status == 0) {
		if (this->game.state == 1) {
			std::cout << "Sign in detected\n";
			this->ComputeMemoryAddresses();
			this->game.status = 1;
			return;
		}
	} else {
		if (this->game.state != 1) {
			std::cout << "Sign out detected\n";
			this->game.status = 0;
			return;
		}
	}

	//Not in the game. No point trying to read addresses that aren't accessible.
	if (this->game.state == 0) {
		return;
	}

	//Core game data
	this->ReadGameData();

	//Player data
	this->ReadPlayerData();

	//Object manager (Target data)
	this->ReadObjectData();

	//Run the garbage collector
	this->GarbageCollector();

	//Do memory things...
	std::cout << "Zone: " << std::dec << this->game.zone << "\n";
	std::cout << "Mouseover GUID: " << std::hex << this->game.mouseOverGuid << "\n";
	std::cout << "XYZ: " << this->player.x << ", " << this->player.y << ", " << this->player.z << "\n";
	std::cout << "Target: " << this->player.targetGuid << "\n";

}

inline void WowMemory::ReadEssentialData() {

	short isConnected;
	ReadProcessMemory(this->processHnd, (void *)(this->addressGameState), &this->game.state, sizeof(this->game.state), 0);
	ReadProcessMemory(this->processHnd, (void *)(this->addressIsConnected), &isConnected, sizeof(isConnected), 0);

	this->game.isConnected = (isConnected == 0 ? 0 : 1);

}

inline void WowMemory::ReadGameData() {

	ReadProcessMemory(this->processHnd, (void *)(this->addressGameZone), &this->game.zone, sizeof(this->game.zone), 0);
	ReadProcessMemory(this->processHnd, (void *)(this->addressMouseOverGuid), &this->game.mouseOverGuid, sizeof(this->game.mouseOverGuid), 0);
	ReadProcessMemory(this->processHnd, (void *)(this->addressTargetGuid), &this->player.targetGuid, sizeof(this->player.targetGuid), 0);

}

inline void WowMemory::ReadPlayerData() {

	DWORD_PTR addrPlayerX = this->addressPlayer + WOW_MEM_PLAYER_X;
	DWORD_PTR addrPlayerY = this->addressPlayer + WOW_MEM_PLAYER_Y;
	DWORD_PTR addrPlayerZ = this->addressPlayer + WOW_MEM_PLAYER_Z;
	DWORD_PTR addrPlayerRot = this->addressPlayer + WOW_MEM_PLAYER_ROTATION;
	DWORD_PTR addrPlayerHp = this->addressPlayer + WOW_MEM_PLAYER_HEALTH;
	DWORD_PTR addrPlayerMaxHp = this->addressPlayer + WOW_MEM_PLAYER_MAX_HEALTH;

	ReadProcessMemory(this->processHnd, (void *)(addrPlayerX), &this->player.x, sizeof(this->player.x), 0);
	ReadProcessMemory(this->processHnd, (void *)(addrPlayerY), &this->player.y, sizeof(this->player.y), 0);
	ReadProcessMemory(this->processHnd, (void *)(addrPlayerZ), &this->player.z, sizeof(this->player.z), 0);
	ReadProcessMemory(this->processHnd, (void *)(addrPlayerRot), &this->player.rotation, sizeof(this->player.rotation), 0);
	ReadProcessMemory(this->processHnd, (void *)(addrPlayerHp), &this->player.health, sizeof(this->player.health), 0);
	ReadProcessMemory(this->processHnd, (void *)(addrPlayerMaxHp), &this->player.maxHealth, sizeof(this->player.maxHealth), 0);

}

inline void WowMemory::ReadObjectData() {

	//Loops through the object manager.
	//Fishes out any wanted objects for reading, such as target or mouse-over information.
	//Also, mining nodes.
	DWORD_PTR currentObj, nextObj, firstObjAddress;
	GameObject currentGameObj;

	firstObjAddress = this->addressObjMgr + WOW_MEM_OBJMGR_FIRST;

	//Preload the first object.
	ReadProcessMemory(this->processHnd, (void *)firstObjAddress, &currentObj, sizeof(currentObj), 0);

	while (1) {

		//Read the GUID and object type.
		ReadProcessMemory(this->processHnd, (void *)(currentObj + WOW_MEM_OBJ_GUID), &currentGameObj.guid, sizeof(currentGameObj.guid), 0);
		ReadProcessMemory(this->processHnd, (void *)(currentObj + WOW_MEM_OBJ_TYPE), &currentGameObj.type, sizeof(currentGameObj.type), 0);

		//Read the next object.
		ReadProcessMemory(this->processHnd, (void *)(currentObj + WOW_MEM_OBJMGR_NEXT), &nextObj, sizeof(nextObj), 0);

		if (nextObj == currentObj || nextObj == 0x0) {
			std::cout << "End of object manager\n";
			break;
		}

		//Is the node a mining or herb type?
		if (currentGameObj.type == WOW_OBJECT_TYPE_MINING) {
			//std::cout << "Type: " << currentGameObj.type << " GUID: " << std::hex << currentGameObj.guid << "\n";
			this->PrependMinableObject(currentObj);
		}

		//Is the node the same as the player target guid?
		if (currentGameObj.guid == this->player.targetGuid) {
			std::cout << "Found player target object: " << std::hex << currentGameObj.guid << "\n";
		}

		//Is the node the same as the mouse over guid?
		if (currentGameObj.guid == this->game.mouseOverGuid) {
			std::cout << "Found mouse over object: " << std::hex << currentGameObj.guid << " (Address: " << currentObj << ")\n";
		}

		//Increment for the loop
		currentObj = nextObj;
		
	}

}

inline void WowMemory::GarbageCollector() {

	time_t diff = 0;
	time_t currentTime = std::time(nullptr);
	GatherObject *currentObj = this->firstGatherObject;
	GatherObject *prevObj = 0x0;
	GatherObject *nextObj = 0x0;

	while (1) {

		//No more in loop.
		if (currentObj == 0x0) {
			break;
		}

		nextObj = currentObj->nextObject;

		//Check to see if it's stale.
		diff = (currentTime - currentObj->lastSeen);
		if ( diff >= WOW_GO_REMOVE_INACTIVE_SECONDS) {
			std::cout << "Remove stale object entry " << std::hex << (void *)currentObj << ", inactive for: " << std::dec << diff << " seconds\n";
			if (prevObj != 0x0) {
				prevObj->nextObject = nextObj;
			}
			if (this->firstGatherObject == currentObj) {
				this->firstGatherObject = nextObj;
			}
			free(currentObj);
			currentObj = nextObj;
			continue;
		}

		//Step to next child in list.
		prevObj = currentObj;
		currentObj = nextObj;

	}

}

inline void WowMemory::PrependMinableObject(DWORD_PTR objAddress) {
	
	bool objectExists = false;
	time_t currentTime = std::time(nullptr);
	GatherObject gatherObject;
	
	//Read the GUID.
	ReadProcessMemory(this->processHnd, (void *)(objAddress + WOW_MEM_OBJ_GUID), &gatherObject.guid, sizeof(gatherObject.guid), 0);
	ReadProcessMemory(this->processHnd, (void *)(objAddress + WOW_MEM_OBJ_TYPE), &gatherObject.type, sizeof(gatherObject.type), 0);
	ReadProcessMemory(this->processHnd, (void *)(objAddress + WOW_MEM_OBJ_X), &gatherObject.x, sizeof(gatherObject.x), 0);
	ReadProcessMemory(this->processHnd, (void *)(objAddress + WOW_MEM_OBJ_Y), &gatherObject.y, sizeof(gatherObject.y), 0);
	ReadProcessMemory(this->processHnd, (void *)(objAddress + WOW_MEM_OBJ_Z), &gatherObject.z, sizeof(gatherObject.z), 0);
	gatherObject.lastSeen = currentTime;
	
	GatherObject *currentObj = this->firstGatherObject;
	bool matchFound = false;

	while (1) {

		//No more in loop.
		if (currentObj == 0x0 ) {
			break;
		}

		//Already exists in list.
		if (currentObj->guid == gatherObject.guid) {
			matchFound = true;
			break;
		}

		//Step to next child in list.
		currentObj = currentObj->nextObject;

	}

	if (matchFound) {
		//Update.
		//std::cout << "Updated: " << std::hex << (void *)currentObj << "\n";
		currentObj->guid = gatherObject.guid;
		currentObj->type = gatherObject.type;
		currentObj->lastSeen = gatherObject.lastSeen;
	}
	else {
		//Create
		GatherObject *futureObject = (GatherObject *)malloc(sizeof(GatherObject));
		futureObject->guid = gatherObject.guid;
		futureObject->type = gatherObject.type;
		futureObject->lastSeen = gatherObject.lastSeen;
		futureObject->nextObject = this->firstGatherObject;
		this->firstGatherObject = futureObject;
		//std::cout << "Created: " << std::hex << (void *)futureObject << "\n";
	}

	std::cout << "Current object type: " << std::dec << gatherObject.type << ", GUID: " << std::hex << gatherObject.guid << ", XYZ: " <<
		std::dec << gatherObject.x << ", " << gatherObject.y << ", " << gatherObject.z << " last seen: " << std::dec << gatherObject.lastSeen << "\n";

}


inline void WowMemory::ReadTargetData(DWORD_PTR objAddress) {

	//Read target data and point PlayerObject to the object.

}

/*
 * Gets the base memory address location for a given process ID
 */
inline DWORD_PTR WowMemory::ProcGetBaseAddress(DWORD processId, HANDLE *processHandle) {

	DWORD_PTR baseAddress = 0;
	HMODULE *moduleArray;
	LPBYTE moduleArrayBytes;
	DWORD bytesRequired;

	if (*processHandle) {
		if (EnumProcessModulesEx(*processHandle, NULL, 0, &bytesRequired, 0x02)) {
			if (bytesRequired) {
				moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);
				if (moduleArrayBytes) {

					unsigned int moduleCount;
					moduleCount = bytesRequired / sizeof(HMODULE);
					moduleArray = (HMODULE *)moduleArrayBytes;

					if (EnumProcessModulesEx(*processHandle, moduleArray, bytesRequired, &bytesRequired, 0x02)) {
						baseAddress = (DWORD_PTR)moduleArray[0];
					}

					LocalFree(moduleArrayBytes);
				}
			}
		}

	}

	return baseAddress;

}