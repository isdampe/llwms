#include "stdafx.h"
#include "wow-memory.h"
#include "wow-offsets.h"
#include <iostream>
#include <windows.h>
#include <psapi.h>

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

	//Do memory things...
	std::cout << "Zone: " << std::dec << this->game.zone << "\n";
	std::cout << "Mouseover GUID: " << std::dec << this->game.mouseOverGuid << "\n";
	std::cout << "X: " << this->player.x << "\n";
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
			std::cout << "Found mouse over object: " << std::hex << currentGameObj.guid << "\n";
		}

		//Increment for the loop
		currentObj = nextObj;
		
	}

}

inline void WowMemory::PrependMinableObject(DWORD_PTR objAddress) {
	
	bool objectExists = false;
	GatherObject *currentGatherObject = (GatherObject *)malloc(sizeof(GatherObject));

	//Read the GUID.
	ReadProcessMemory(this->processHnd, (void *)(objAddress + WOW_MEM_OBJ_GUID), &currentGatherObject->guid, sizeof(currentGatherObject->guid), 0);
	ReadProcessMemory(this->processHnd, (void *)(objAddress + WOW_MEM_OBJ_TYPE), &currentGatherObject->type, sizeof(currentGatherObject->type), 0);

	//Loop through existing mining objects.
	GatherObject *nextGatherObject = this->firstGatherObject;
	std::cout << std::hex << nextGatherObject << "\n";
	
	while (1) {
		if (nextGatherObject == 0x0) {
			std::cout << "Trigger empty\n";
			//End of list
			break;
		}

		std::cout << nextGatherObject->guid << ":" << currentGatherObject->guid << "\n";
		if (nextGatherObject->guid == currentGatherObject->guid) {
			//Already exists. Don't append, just update.
			std::cout << "Match\n";
			objectExists = true;
			break;
		}

		nextGatherObject = nextGatherObject->nextObject;

	}

	if (objectExists == true) {
		//Update existing object;
		currentGatherObject = nextGatherObject;
		std::cout << "Update " << std::hex << currentGatherObject->guid << "\n";
	}
	else {
		//Inject the object.
		currentGatherObject->nextObject = nextGatherObject;
		this->firstGatherObject = currentGatherObject;
		std::cout << "Create " << std::hex << currentGatherObject->guid << "\n";
	}

	system("PAUSE");

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