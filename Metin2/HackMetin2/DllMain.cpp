#include <iostream>
#include <Windows.h>
#include <string>
#include <conio.h>

#include "hooks.h"
#include "utils.h"

/*
[TODO]
1. Teleport
2. Investigar relogging
5. Investigar la func a la que se llama en parse_recv_chat, parece que itera los ids
4. Hacer posible lo de ejecutar server commands en local
7. Add a function to check if we're okay: send a packet and expect answer
9. Revisar estructura cmd
10. Maybe I could avoid all those packs and unpacks just copying memory
11. Check why sometimes my client segfaults with dc hack in guabina server

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


[ INVESTIGACION PORCULING ]
Parece que aplica retroceso cuando se sincroniza el packet attack y el packet move con el subtype de retroceso.
Que le golpee en mi cliente no sirve de nada, debe ser en el suyo. Es cada cliente el que detecta la colisi�n.
Enviar un packet attack solo: solo hace da�o
Enviar un packet move attack solo: solo hace la animacion
Enviar ambos: retroceso

IDEA: hack que envie packet attack al target (ya lo tengo), pero tambien que envie packet_move_attack. 
Para ello debemos pillar las coords del target.

Puede que no sea posible. Si cambio el packet CG_Move para que siempre haga retroceso me echa el server


[ INVESTIGACION GHOSTMODE ]
Funcion dead en archivo char_battle.cpp
Funcion EVENTFUNC(recovery_event) en archivo char.cpp


[ INVESTIGACION HACK LIFE ]
con UNC path \\192.168.1.47\test_klecko\payload.exe:
https://gyazo.com/c19489f7a05467e9d05ddab08a19b79f?token=5e1fa641fc05fe1cbb6a0dba6e4ca9dd
https://gyazo.com/3c3ba70c1215e34fd274c507590b4e07
https://gyazo.com/0a87d7acbc3847fb43b9e7a1b6164f8f
parece IE no lo abre si no esta en trusted sites

con UNC path \\192.168.1.47\test_klecko
https://gyazo.com/19a9682d56b4ad75b846663c59e6691d
https://gyazo.com/608f9915c5064d74e013e4bfde4c72a9
works

con http 192.168.1.47/payload.exe
https://gyazo.com/9beb5325b5edf8c35e2ea4842fab5c70
https://gyazo.com/bc2390bb37903c26e03d44cbfddf87a8
works. parece que salta el antivirus, pero probablemente se pueda evitar


con http 192.168.1.47 y un link a UNC \\192.168.1.47\test_klecko\payload.exe:
no abre links UNC



DOS OPCIONES:
1. UNC path a la carpeta, poner un README que convenza a ejecutar. el antivirus suda.
2. HTTP path a sitio metin2lion phishing que convenza a descargar y ejecutar. hacer que el binario no sea detectado por el antivirus.

El packet injection va como el culo, no funciona en ch1
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
		
		//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)get_objects_addresses, 0, 0, 0);
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)print_inicio, 0, 0, 0);

	} else if (dwReason == DLL_PROCESS_DETACH) {
		cout << "see you.." << endl;
		fclose(pCout);
		fclose(pCin);
		FreeConsole();
	}
	return true;
}
