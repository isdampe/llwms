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
	DWORD_PTR ProcGetBaseAddress(DWORD processId, HANDLE *processHandle);

	//Data
	HWND wHnd;
	HANDLE processHnd;
	DWORD processId;
	DWORD_PTR addressProcessBase, addressGameState, addressGameZone, addressPlayer,
			  addressTargetGuid, addressObjMgr;
	GameData game;
	PlayerObject player;
	TargetObject target;
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

	this->ComputeMemoryAddresses();

}

inline void WowMemory::Critical(char *message) {
	std::cout << message << "\n";
	throw message;
}

inline void WowMemory::ComputeMemoryAddresses() {

	//Get the process base address.
	this->addressProcessBase = this->ProcGetBaseAddress(this->processId, &this->processHnd);
	this->addressGameState = (this->addressProcessBase) + WOW_MEM_GAME_STATE;
	this->addressGameZone = (this->addressProcessBase) + WOW_MEM_ZONE;
	this->addressPlayer = (this->addressProcessBase) + WOW_MEM_PLAYER_BASE;
	this->addressTargetGuid = (this->addressProcessBase) + WOW_MEM_PLAYER_TARGET_GUID;

	std::cout << "\nList of calculated addresses:\n\n";
	std::cout << "Process base: " << std::hex << this->addressProcessBase << "\n";
	std::cout << "Game state: " << std::hex << this->addressGameState << "\n";
	std::cout << "Game zone: " << std::hex << this->addressGameZone << "\n";
	std::cout << "Player base: " << std::hex << this->addressPlayer << "\n";
	std::cout << "Target Guid: " << std::hex << this->addressTargetGuid << "\n\n";

}

inline void WowMemory::Update() {

	ReadProcessMemory(this->processHnd, (void *)(this->addressGameState), &this->game.state, sizeof(this->game.state), 0);

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

	//Do memory things...
	std::cout << "State: " << this->game.state << "\n";
	std::cout << "Read memory!\n";



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