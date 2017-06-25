#include "stdafx.h"
#include "wow-memory.cpp"

int main()
{
	WowMemory Client;
	while (1) {
		Client.Update();
		Sleep(1000);
	}
	system("PAUSE");
    return 0;
}

