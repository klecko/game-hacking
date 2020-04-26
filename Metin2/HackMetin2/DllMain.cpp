#include <iostream>
#include <Windows.h>
#include <string>

#include "hooks.h"
#include "sigscan.h"
#include "packet_struct.h"
#include "packet.h"
#include "utils.h"

/*
[TODO]
Teleport
Autodmg for near targets
Description and error handling in cmd
Create getters and setters. Maybe do every attribute public?
DEFAULT CONSTRUCTORS ARE NOT SETTING HEADER!!

Throwing and picking up 1 yang:
14000000010000000036
0F575E14007C
14000000010000000033
0F275F14002F
14000000010000000098
0FC85F140076

Throwing other items:
1401010000000000014E  
14010500000000000116
140105000000000033F1

*/



using namespace std;

FILE* pCout;
FILE* pCin;

void print_inicio(){
	while (!ingame())
		Sleep(1000);

	Sleep(2000);
	print("Welcome to heaven, boy.");
}




BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH || dwReason == DLL_PROCESS_DETACH) {
		if (dwReason == DLL_PROCESS_ATTACH) {
			AllocConsole();
			freopen_s(&pCout, "CONOUT$", "w", stdout);
			freopen_s(&pCin, "CONIN$", "r", stdin);
			cout << "HI BABE" << hex << endl;
			sigscan();
			detours();

			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)print_inicio, 0, 0, 0);
		}
		else if (dwReason == DLL_PROCESS_DETACH) {
			cout << "see you.." << endl;
			fclose(pCout);
			fclose(pCin);
		}
	}
	return true;
}