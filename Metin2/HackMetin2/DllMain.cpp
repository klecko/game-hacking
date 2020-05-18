#include <iostream>
#include <Windows.h>
#include <string>
#include <conio.h>

#include "hooks.h"
#include "sigscan.h"
#include "packet_struct.h"
#include "packet.h"
#include "utils.h"

/*
[TODO]
1. Teleport
2. Investigar relogging
3. Try to reduce dependencies?
4. Think about race condition in two threads created in dllmain
5. Investigar la func a la que se llama en parse_recv_chat, parece que itera los ids
6. Maybe in attack hack we could have only ids and not coordinates
7. Add a function to check if we're okay: send a packet and expect answer
8. MORE PACKETS
9. Revisar estructura cmd
10. Maybe I could avoid all those packs and unpacks just copying memory
11. Avoid pointer lists: find another way

Packets attacking with a bow a metin whose id was: 191193 (0x02ead9)
seems like there are two packets?

header target_id(int) ?? another_packet
parece que ?? va cambiando :(

33 D9EA0200 70A30E00718102006F 07030E0468A10E0066B90300A8BF1F020C
360015
33D9EA020070A30E0071810200A407030E0468A10E0066B9030002C31F02DC
36008D
33D9EA020070A30E00718102004F07030E0468A10E0066B903005CC61F020B
3600A6
33D9EA020070A30E00718102004707030E0468A10E0066B90300B6C91F02A5
3600B7
33D9EA020070A30E0071810200A507030E0468A10E0066B9030010CD1F02F0
360041
33D9EA020070A30E00718102001707030E0468A10E0066B903006AD01F026C
3600FE
33D9EA020070A30E00718102009C07030E0468A10E0066B90300C4D31F020D
360063


[PACKET_STRUCT2]+c parece puntero a algo que parece el player, tiene el nombre.
y [[PACKET_STRUCT2]+c]+1d4] parece puntero a algo como player_stats, que es lo que yo ahora mismo llamo player

aunque [PACKET_STRUCT2]+c tmb tiene cosas de stats como posicion o direccion

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

void press_key(){
	cout << endl << "Press key to exit..." << endl;
	_getch();
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		AllocConsole();
		freopen_s(&pCout, "CONOUT$", "w", stdout);
		freopen_s(&pCin, "CONIN$", "r", stdin);
		cout << "[WELCOME] Metin2 hack by Klecko" << endl << endl;

		cout << hex;
		if (!sigscan()){
			cout << "[ERROR] Sigscan failed" << endl;
			press_key();
			return false;
		}
		if (!detours()){
			cout << "[ERROR] Detours failed" << endl;
			press_key();
			return false;
		}
		cout << dec;
		
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)get_objects_addresses, 0, 0, 0);
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)print_inicio, 0, 0, 0);

	} else if (dwReason == DLL_PROCESS_DETACH) {
		cout << "see you.." << endl;
		fclose(pCout);
		fclose(pCin);
		FreeConsole();
	}
	return true;
}