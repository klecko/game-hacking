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
VER QUÉ PAQUETES ENVIA EL CLIENTE CUANDO LE MODIFICA LA SPEED Y ATACA
El cliente modifica | por || al enviar mensajes porque los | se usa para colores. Si escribes |cFFFFFF00|H|hhola sale en color.
Create getters and setters. Maybe do every attribute public?
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