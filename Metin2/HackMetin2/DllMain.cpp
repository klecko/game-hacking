#include <iostream>
#include <Windows.h>
#include <string>
#include <conio.h>

#include "hooks.h"
#include "utils.h"

/*
[TODO]
1. Teleport
2. Avoid hack crashing when relogging
6. Investigate why sometimes in whisp command the name and msg join together.
	If clients is receiving everything as user, maybe we can overflow
7. [DOIT] think about fuzzing client calling the recv packet functions with packets that
	can be injected
10. Maybe I could avoid all those packs and unpacks just copying memory
11. Check why sometimes my client segfaults with dc hack in guabina server



[NOTAS]
[PACKET_STRUCT2]+c parece puntero a algo que parece el player, tiene el nombre.
y [[PACKET_STRUCT2]+c]+1d4] parece puntero a algo como player_stats, que es lo que yo ahora mismo llamo player

aunque [PACKET_STRUCT2]+c tmb tiene cosas de stats como posicion o direccion


[ INVESTIGACION PORCULING ]
Parece que aplica retroceso cuando se sincroniza el packet attack y el packet move con el subtype de retroceso.
Que le golpee en mi cliente no sirve de nada, debe ser en el suyo. Es cada cliente el que detecta la colisión.
Enviar un packet attack solo: solo hace daño
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
https://gyazo.com/c19489f7a05467e9d05ddab08a19b79f
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


DOS CASOS HIPOTÉTICOS:
1. UNC path a la carpeta, poner un README que convenza a ejecutar. el antivirus suda.
2. HTTP path a sitio metin2lion phishing que convenza a descargar y ejecutar. hacer que el binario no sea detectado por el antivirus.


[URI SCHEMES]
Otra idea: Uri schemes. Hace un par de años había CVEs para conseguir RCE pero ya no
parece posible por el URL encoding. Lo bueno: se ejecutan sin pedir confirmación. Lo que hace
un par de años en un navegador normal era 1-click-RCE aquí hubiera sido 0-click-RCE.
Uri schemes posibles:
	- PowerPoint. Parece que pregunta si se quiere abrir el archivo y avisa que puede tener virus.
		Hay otros parecidos: ms-* <--- INVESTIGAR
	- Acrobat. No dice nada de virus.
	- jnlp. algo de java, parece que puedo conseguir ejecutar un .jar
	- steam://run/<id>//<args>/ Runs an application. It will be installed if necessary.
		Encontrar aplicación vulnerable con bad parsing de argumentos o algo?
		Habia muchas opciones pero ahora steam avisa que se esta ejecutando con argumentos y te hace
			querer darle a NO
	- Abrir vscode, libreoffice. abrir archivo malicioso? hace falta que diga que sí a ejecutar macros.

[INVESTIGAR NTLM HASH]
Es posible conseguir el NetNTLM hash del usuario haciendo que abra mi servidor samba.


El packet injection va como el culo, no funciona en ch1


[ INVESTIGACION ROOT CAUSE PACKET INJECTION ]
El cliente realiza un primer recv en el que recibe todo el padding. El último paquete que recibe
está cortado. En parse_recv_whisper intenta recibir todo el paquete, pero como no ha llegado devuelve
false, y al devolver false se imprime el siguiente error en syserr.txt:
	0603 00:06:41401 :: Phase Game does not handle this header (header: 34, last: 34, 34)

El segundo recv es el que recibe el paquete inyectado.

- Investigar cómo los procesa el server y por qué pasa eso.

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
