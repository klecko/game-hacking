/*
TO DO:
Igual puedo hookear el constructor de un objeto y pasarle como un argumento un puntero this que privamente
reservo con el tamaño del objeto (o sin reservar), de manera que tenga la direccion del objeto en ese puntero, y podria
acceder a sus datos mediante esa direccion. Probar a hacerlo con un vector3?

asegurar que el tamaño del name en el log no supere 20 IMPORTANT

*******IMPORTANT*********
PENSAR HACER LA VARIABLE PLAYER GLOBAL, DE MANERA QUE SEA CONST. HAY ALGUNA MANERA?
HACER QUE PLAYER TICK COMPRUEBA SI LA GLOBAL ES NULLPTR, EN ESE CASO PONER pPlayer Y realpPplayer
Y ASI QUITAR TODOS LOS ARGS

QUE TE DIGA QUE NO HA ENCONTRADO FUNCIONES EN ESE CASO.

QUE EL BRUTEFORCE PARE CUANDO ENCUENTRE ALGO

THREADS NO VAN BIEN POR LA DIVISION ENTERA

*/
#include <iostream>
#include <Windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <map>
#include "detours.h"
#include "sigscan.h"

#pragma comment(lib, "detours.lib")

using namespace std;

FILE* pCout;
FILE* pCin;

const int addrsToNope = 2;
DWORD addrToNopeFastTravels[addrsToNope];
const BYTE codeToNopeFastTravels[addrsToNope][2] = { { '\x75', '\x75' }, {'\x74', '\x66'} };

float playerSpeed = 200.0;
float playerJump = 420.0;
bool playerCanJumpActivated = false;
bool fastTravelsActivated = false;

map<string, string> help = {
	{"Help", "Available commands: set, tp, tpr, input, output, bruteforce. Write help <command> to know more."},
	{"set", "Sets a value to a variable. Syntax: set <variable> <value>. Available variables and values are listed below.\nSpeed: player walking speed. Double.\nJump: player jump height. Double.\nCanjump: enables or disables flying hack. Bool.\nFasttravels: enables or disables having all fast travels discovered. Bool."},
	{"tp", "Teleports you to given coords. Syntax: tp <x> <y> <z>"},
	{"tpr", "Teleports you to given coords relatively to your current position. Syntax: tpr <x> <y> <z>"},
	{"input", "Sets the given value as input of the given stage of Blocky Challenge. The stage must be given as a number from 1 to 5. Syntax: input <stage> <value>"},
	{"output", "Gets the final output of the given stage of Blocky Challenge. The stage must be given as a number from 1 to 5. Syntax: output <stage>"},
	{"bruteforce", "Performs a bruteforce attack against a stage of the Blocky Challenge. Syntax: bruteforce <stage> <number_of_inputs> <number_of_threads>"}
};

//Definition of pointer to function types
typedef void (__thiscall *pPlayerTick_t)(void *real_pPlayer, float delta_time);
typedef void (__thiscall *pPlayerChat_t)(void *_pPlayer, const char *text);
typedef void (__thiscall *pActorSetPosition_t)(void *_pActor, DWORD pVector3);
typedef void (__thiscall *pPlayerReceiveChat_t)(void *real_pPlayer, DWORD pSource, const char *text);
typedef void (__thiscall *pActorGetPosition_t)(void *_pActor, float *result);
typedef void (__thiscall *pPlayerSetCircuitInputs_t)(void *_pPlayer, const char *name, unsigned int state);
typedef void (__thiscall *pPlayerGetCircuitOutputs_t)(void *_pPlayer, const char *name, bool states[], unsigned int len);
typedef bool (__thiscall *pPlayerCanJump_t)(void *_pPlayer);


//Player.Tick seems to be called every frame. I hook it to change constantly speed and jump height.
pPlayerTick_t OriginalPlayerTick;

//Player.Chat is called when the player sends a message. This function is hooked so I can
//read input from the player and perform actions according to that.
pPlayerChat_t OriginalPlayerChat;

//Actor.SetPosition is called when the player performs a teleport with the Fast Travel System.
//(maybe it's used somewhere else). This function was hooked as a proof of concept of hooking
//and modifying arguments, nothing useful here.
pActorSetPosition_t OriginalActorSetPosition;

//Player.ReceiveChat makes the player to receive a chat message. I use this function to log my
//own messages in the chat (results of command execution, help, etc)
pPlayerReceiveChat_t PlayerReceiveChat;

//Actor.GetPosition is used to the get the position of the actor. I use this to perform a
//teleport which is relative to the position of the player.
pActorGetPosition_t ActorGetPosition;

//Player.SetCircuitInputs sets the input of the circuit of a stage. I use this to automate that
//process and also for a *failed* attempt of bruteforce (useless for 2^32 combinations, too slow)
pPlayerSetCircuitInputs_t PlayerSetCircuitInputs;

//Player.GetCircuitInputs returns the state of the circuit of a stage. I use this for the
//bruteforce, so I can check if a combination is valid or not.
pPlayerGetCircuitOutputs_t PlayerGetCircuitOutputs;

//Player.CanJump returns true or false depending on whether the player should be allowed to jump
//or not. I detour it so I can return always true or just call the original function.
pPlayerCanJump_t OriginalPlayerCanJump;

//Returns the given string splitted according to the delimiter #copied
vector<string> split(const string& s, char delimiter)
{
	//no idea what this functions does, just copied it
	//not gonna waste time in this xdd although it seems quite easy
	vector<std::string> tokens;
	string token;
	istringstream tokenStream(s);
	while (getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

//Returns the a string with the float with the desired precision #copied
string to_string_precision(const float a_value, const int n)
{
	//copied function
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << a_value;
	return out.str();
}

//Writes the name of the stage into stage according to the given numbr
void get_stage(string n, char *stage){
	if (n == "1") {
		strcpy_s(stage, 20, "Stage1");
	}
	else if (n == "2") {
		strcpy_s(stage, 20, "Stage2");
	}
	else if (n == "3") {
		strcpy_s(stage, 20, "Stage3");
	}
	else if (n == "4") {
		strcpy_s(stage, 20, "Stage4");
	} else {
		strcpy_s(stage, 20, "FinalStage");
	}
}

//Logs text_str in the chat game as if a player with name_str would send it, using Player.ReceiveChat
void log_ingame(void* real_pPlayer, string name_str, string text_str){
	const char *name = name_str.c_str();
	const char *text = text_str.c_str();

	//lo que hace el cliente:
	//si la longitud del texto es menor o igual a 20, en text+20 pone una F (menor que 0x10)
	//si es mayor de 20, en text pone un puntero, y pone text+20 a 1F, de manera que luego
	//comprueba text+20 y deferencia el puntero para obtener el texto largo.
	//despues de unas horas para averiguar eso, y otro par de ellas rayandome la cabeza decidi
	//cambiar la comprobacion que hace que se deferencie el puntero cuando la longitud del texto
	//es mayor que 20 de manera que nunca se haga y a tomar por saco
	//WARNING: CHAPUZA A LA VISTA
	BYTE *addr_a_cambiar = (BYTE*)((DWORD)PlayerReceiveChat + 0xED);
	DWORD oldprot, useless;
	VirtualProtect(addr_a_cambiar, 1, PAGE_EXECUTE_READWRITE, &oldprot);
	*addr_a_cambiar = 0xEB; //cambio JB SHORT por JMP SHORT
	VirtualProtect(addr_a_cambiar, 1, oldprot, &useless);

	char antiguo_nombre[20]; //it seems that the max space for the name is 20 bytes
	char *player_name = (char*)((DWORD)real_pPlayer + 0x78);

	strcpy_s(antiguo_nombre, 20, player_name);
	strcpy_s(player_name, 20, name);
	PlayerReceiveChat(real_pPlayer, (DWORD)real_pPlayer, text);
	strcpy_s(player_name, 20, antiguo_nombre);

	VirtualProtect(addr_a_cambiar, 1, PAGE_EXECUTE_READWRITE, &oldprot);
	*addr_a_cambiar = 0x72; //cambio JMP SHORT por JB SHORT
	VirtualProtect(addr_a_cambiar, 1, oldprot, &useless);
}

//Logs ingame the help of a given command
void display_help(void *real_pPlayer, string command_name){
	log_ingame(real_pPlayer, "Help [" + command_name + "]", help[command_name]);
}

//Teleports the player to the pos
void tp(float pos[3], void *real_pPlayer) {
	//seems like pos is something like a array of 6 floats: {x, y, z, diffx, diffy, diffz},
	//but those last three seem to do nothing
	float pos_final[3] = { pos[0], pos[1], pos[2] }; // , 100, 100, 100 };
	OriginalActorSetPosition(real_pPlayer, (DWORD)pos_final);
}

//Performs a bruteforce attack in the stage trying out numbers from first to last
void bruteforce(UINT64 first, UINT64 last, const char *stage, void *_pPlayer) {
	void *real_pPlayer = (void*)((DWORD)_pPlayer - 0x70);

	bool output[] = { false };
	UINT64 i;
	for (i = first; i < last && !output[0]; i++) {
		//PlayerSetCircuitInputs(_pPlayer, stage, stoi(splitted_text[2]));
		PlayerSetCircuitInputs(_pPlayer, stage, i);
		PlayerGetCircuitOutputs(_pPlayer, stage, output, 1);
		string log = "Input set to " + to_string(i) + ". Output: " + to_string(output[0]) + ". Done: " + to_string((i - first)*100.0 / (last-first)) + "%\n";
		cout << log;
	}

	if (output[0]) {
		cout << "Fucking found!!" << endl;
		log_ingame(real_pPlayer, "Command", "Bruteforce succeeded. Answer is " + to_string(i-1));
	}
}

void disenable_fast_travels(bool enable){
	DWORD oldprot, useless;
	for (int i = 0; i < addrsToNope; i++){
		VirtualProtect((void*)addrToNopeFastTravels[i], 2, PAGE_EXECUTE_READWRITE, &oldprot);
		if (enable){
			*(BYTE*)(addrToNopeFastTravels[i]) = '\x90';
			*(BYTE*)(addrToNopeFastTravels[i] + 1) = '\x90';
		} else {
			*(BYTE*)(addrToNopeFastTravels[i]) = codeToNopeFastTravels[i][0];
			*(BYTE*)(addrToNopeFastTravels[i] + 1) = codeToNopeFastTravels[i][1];
		}
		VirtualProtect((void*)addrToNopeFastTravels[i], 2, oldprot, &useless);
	}
}

//Handles the tp command, performing relative or absolute teleport and logging
void tp_command(vector<string> splitted_text, bool relative, void *real_pPlayer){
	if (splitted_text.size() == 4) {
		float pos[3] = { stof(splitted_text[1]), stof(splitted_text[2]), stof(splitted_text[3]) };
		if (relative){
			float current_pos[3];
			ActorGetPosition(real_pPlayer, current_pos);
			pos[0] += current_pos[0];
			pos[1] += current_pos[1];
			pos[2] += current_pos[2];
		}
		log_ingame(real_pPlayer, "Command", "Teleporting to position " + to_string_precision(pos[0], 0) + ", " + to_string_precision(pos[1], 0) + ", " + to_string_precision(pos[2], 0) + ".");
		tp(pos, real_pPlayer);
	} else {
		//log_ingame(real_pPlayer, "Command", "Wrong tp command??");
		display_help(real_pPlayer, splitted_text[0]);
	}
}

//Handles the input command, setting the circuit inputs and logging
void input_command(vector<string> splitted_text, void *_pPlayer){
	void *real_pPlayer = (void*)((DWORD)_pPlayer - 0x70);
	if (splitted_text.size() == 3) {
		char stage[20];
	
		get_stage(splitted_text[1], stage);
	
		PlayerSetCircuitInputs(_pPlayer, stage, stoi(splitted_text[2]));

		cout << "input " << splitted_text[2] << " set to " << stage << endl;
		log_ingame(real_pPlayer, "Command", "Input " + splitted_text[2] + " set to " + stage);
	} else {
		display_help(real_pPlayer, splitted_text[0]);
	}
}

//Handles the output command, getting the circuit output and logging
void output_command(vector<string> splitted_text, void *_pPlayer){
	void *real_pPlayer = (void*)((DWORD)_pPlayer - 0x70);
	if (splitted_text.size() == 2) {
		char stage[20];
	
		get_stage(splitted_text[1], stage);
		bool states[1];
		PlayerGetCircuitOutputs(_pPlayer, stage, states, 1);

		cout << "state of " << stage << ": " << states[0] << endl;
		log_ingame(real_pPlayer, "Command", "Output of " + string(stage) + " is " + (states[0] ? "1" : "0"));
	} else {
		display_help(real_pPlayer, splitted_text[0]);
	}
}

//Handles the bruteforce command, creating threads, calling bruteforce and logging
void bruteforce_command(vector<string> splitted_text, void *_pPlayer){
	void *real_pPlayer = (void*)((DWORD)_pPlayer - 0x70);
	if (splitted_text.size() == 4){
		char stage[20];
		get_stage(splitted_text[1], stage);
		
		SigScan scanner;
		DWORD addr = scanner.FindPattern("GameLogic.dll", "\x77\x6A\x80\x3E\x00", "xxxxx");
		DWORD oldprot, useless;
		VirtualProtect((void*)addr, 2, PAGE_EXECUTE_READWRITE, &oldprot);
		BYTE original = *(BYTE*)(addr+1);
		*(BYTE*)(addr) = 0x90;
		*(BYTE*)(addr+1) = 0x90;
		VirtualProtect((void*)addr, 2, oldprot, &useless);

		int inputs = stoi(splitted_text[2]);
		int threads = stoi(splitted_text[3]);
		UINT64 combinations = pow(2, inputs);
		cout << "Starting bruteforce of " << stage << " which has " << inputs << " inputs (" << combinations << " combinations)." << endl;

		vector<thread> v;
		UINT64 init, end;
		UINT64 step = combinations / threads;
		//thread t(bruteforce, init, end);
		for (int i = 0; i < threads; i++){
			init = i * step;
			end = (i+1)*step;
			cout << "Creating thread number " << i << " from " << init << " to " << end << endl;
			thread t(bruteforce, init, end, stage, _pPlayer);
			v.push_back(move(t));
		}

		for (auto& t : v){
			if (t.joinable())
				t.join();
		}

		log_ingame(real_pPlayer, "Command", "Bruteforce finished.");

		VirtualProtect((void*)addr, 2, PAGE_EXECUTE_READWRITE, &oldprot);
		*(BYTE*)(addr) = 0x77;
		*(BYTE*)(addr + 1) = original;
		VirtualProtect((void*)addr, 2, oldprot, &useless);
	} else {
		display_help(real_pPlayer, splitted_text[0]);
	}
}

//Handles the set command, modifying player speed, jump height, ..., and logging
void set_command(vector<string> splitted_text, void *real_pPlayer){
	if (splitted_text.size() == 3){
		if (splitted_text[1] == "speed"){
			playerSpeed = stof(splitted_text[2]);
			log_ingame(real_pPlayer, "Command", "Set player speed to " + splitted_text[2]);
		} else if (splitted_text[1] == "jump"){
			playerJump = stof(splitted_text[2]);
			log_ingame(real_pPlayer, "Command", "Set player jump height to " + splitted_text[2]);
		} else if (splitted_text[1] == "canjump"){
			if (splitted_text[2] == "1" || splitted_text[2] == "true"){
				playerCanJumpActivated = true;
				log_ingame(real_pPlayer, "Command", "Enabled jump hack.");
			} else if (splitted_text[2] == "0" || splitted_text[2] == "false"){
				playerCanJumpActivated = false;
				log_ingame(real_pPlayer, "Command", "Disabled jump hack.");
			}
		} else if (splitted_text[1] == "fasttravels"){
			if (splitted_text[2] == "1" || splitted_text[2] == "true") {
				disenable_fast_travels(true);
				log_ingame(real_pPlayer, "Command", "Enabled all fast travels.");
			}
			else if (splitted_text[2] == "0" || splitted_text[2] == "false") {
				disenable_fast_travels(false);
				log_ingame(real_pPlayer, "Command", "Disabled all fast travels.");
			}
		}
	} else {
		display_help(real_pPlayer, splitted_text[0]);
	}
}

//Handles the help command, logging the help
void help_command(vector <string> splitted_text, void *real_pPlayer){
	if (splitted_text.size() == 2){
		const char * command = splitted_text[1].c_str();
		if (help.count(command) != 0) {
			display_help(real_pPlayer, splitted_text[1]);
		} else {
			log_ingame(real_pPlayer, "Help", "Help for command " + splitted_text[1] + " not found!");
		}
	} else {
		display_help(real_pPlayer, "Help");
	}
}

//For each command, calls its corresponding handler function
void command(string text, void* real_pPlayer){
	vector<string> splitted_text = split(text, ' ');
	void* _pPlayer = (void*)((DWORD)real_pPlayer + 0x70);
	if (splitted_text.size() > 0){
		if (splitted_text[0] == "tp") {
			tp_command(splitted_text, false, real_pPlayer);
		} else if (splitted_text[0] == "tpr") {
			tp_command(splitted_text, true, real_pPlayer);
		} else if (splitted_text[0] == "input") {
			input_command(splitted_text, _pPlayer);
		} else if (splitted_text[0] == "output") {
			output_command(splitted_text, _pPlayer);
		} else if (splitted_text[0] == "bruteforce") {
			bruteforce_command(splitted_text, _pPlayer);
		} else if (splitted_text[0] == "set") {
			set_command(splitted_text, real_pPlayer);
		} else if (splitted_text[0] == "help") {
			help_command(splitted_text, real_pPlayer);
		} else {
			log_ingame(real_pPlayer, "Command", "Unknown command.");
		}
	}
}

//Hook of Player.Tick. Changes player speed and jump height
void __fastcall HookPlayerTick(void *real_pPlayer, int edx, float delta_time){
	float *pPlayerSpeed = (float*)((DWORD)real_pPlayer + 0x190);
	float *pPlayerJump = (float*)((DWORD)real_pPlayer + 0x194);
	*pPlayerSpeed = playerSpeed;
	*pPlayerJump = playerJump;
	//parece que funciona pero la vez anterior no
	OriginalPlayerTick(real_pPlayer, delta_time);
}

//Hook of Player.Chat. Calls the function command with the received text
void __fastcall HookPlayerChat(void *_pPlayer, int edx, const char *text){
	//example chat _pPlayer: 0x2ede98d0
	//example tick or setposition _pPlayer (or _pActor): 0x2ede9860
	//because of this difference, it seems the real pointer is 0x70 bytes before the one
	//that this function receives.
	string s_text(text);
	void *real_pPlayer = (void*)((DWORD)_pPlayer-0x70);
	command(text, real_pPlayer);
	//OriginalPlayerChat(_pPlayer, text);
}

//Hook of Actor.SetPosition. Just a proof of concept which sets the final position 3000 higher
void __fastcall HookActorSetPosition(void *_pActor, int edx, DWORD pVector3){
	//NOTA: PARECE que pVector3 es algo asi como [x, y, z, diffx, diffy, diffz], aunque estas 3 ultimas
	//parece que no afectan
	float x = *(float*)pVector3;
	float y = *(float*)(pVector3 + 4);
	float z = *(float*)(pVector3 + 8);
	log_ingame(_pActor, "SetPositionHook", "Changing tp to a bit higher.");
	*(float*)(pVector3 + 8) = z + 3000;
	OriginalActorSetPosition(_pActor, pVector3);
}

//Hook of Player.CanJump. Returns true or calls the original function
bool __fastcall HookPlayerCanJump(void *_pPlayer){
	bool ret;
	if (playerCanJumpActivated){
		ret = true;
	} else {
		ret = OriginalPlayerCanJump(_pPlayer);
	}
	return ret;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved){
	if (dwReason == DLL_PROCESS_ATTACH || dwReason == DLL_PROCESS_DETACH){
		if (dwReason == DLL_PROCESS_ATTACH){
			AllocConsole();
			freopen_s(&pCout, "CONOUT$", "w", stdout);
			freopen_s(&pCin, "CONIN$", "r", stdin);
			cout << "HI BABE" << hex << endl;

			//Looks for every function
			SigScan scanner;
			DWORD addrPlayerTick = scanner.FindPattern("GameLogic.dll", "\x55\x8B\xEC\x83\xE4\xC0", "xxxxxx");
			DWORD addrPlayerChat = scanner.FindPattern("GameLogic.dll", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x83\xEC\x24\xA1\x00\x00\x00\x00\x33\xC4\x89\x44\x24\x1C\x56\x57\xA1\x00\x00\x00\x00\x33\xC4\x50\x8D\x44\x24\x30\x64\xA3\x00\x00\x00\x00\x8B\xF9\x8B\x55\x08\xC7\x44\x24\x00\x00\x00\x00\x00\xC7\x44\x24\x00\x00\x00\x00\x00\xC6\x44\x24\x10\x00\x80\x3A\x00\x75\x04\x33\xC9", "xxxxxxxxx????xx????xxxxx????xxxxxxxxx????xxxxxxxxx????xxxxxxxx?????xxx?????xxxxxxxxxxxx", 3);
			DWORD addrActorSetPosition = scanner.FindPattern("GameLogic.dll", "\x55\x8B\xEC\x8B\x55\x08\xF3\x0F\x7E\x02\x66\x0F\xD6\x41", "xxxxxxxxxxxxxx");
			DWORD addrPlayerReceiveChat = scanner.FindPattern("GameLogic.dll", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x83\xEC\x40\xA1\x00\x00\x00\x00\x33\xC4\x89\x44\x24\x38\x53\x56\x57\xA1\x00\x00\x00\x00\x33\xC4\x50\x8D\x44\x24\x50\x64\xA3\x00\x00\x00\x00\x8B\xC1", "xxxxxxxxx????xx????xxxxx????xxxxxxxxxx????xxxxxxxxx????xx");
			DWORD addrActorGetPosition = scanner.FindPattern("GameLogic.dll", "\x55\x8B\xEC\x8B\x49\x0C\x85\xC9\x75\x0F\x8B\x45\x08\x89\x08\x89\x48\x04\x89\x48\x08\x5D\xC2\x04\x00\x8B\x11\xFF\x75\x08\xFF\x52\x08", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
			DWORD addrPlayerSetCircuitInputs = scanner.FindPattern("GameLogic.dll", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x83\xEC\x24\xA1\x00\x00\x00\x00\x33\xC4\x89\x44\x24\x1C\x56\x57\xA1\x00\x00\x00\x00\x33\xC4\x50\x8D\x44\x24\x30\x64\xA3\x00\x00\x00\x00\x8B\xF9\x8B\x0D", "xxxxxxxxx????xx????xxxxx????xxxxxxxxx????xxxxxxxxx????xxxx");
			DWORD addrPlayerGetCircuitOutputs = scanner.FindPattern("GameLogic.dll", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x24\xA1\x00\x00\x00\x00\x33\xC4\x89\x44\x24\x20\x8B\x45\x0C\x53\x8B\x5D\x10", "xxxxxxxxxx????xxxxxxxxxxxxx");
			DWORD addrPlayerCanJump = scanner.FindPattern("GameLogic.dll", "\x8B\x49\x9C\x85\xC9", "xxxxx");
			DWORD addrToNopeFastTravelsFirst = scanner.FindPattern("GameLogic.dll", "\x75\x75\x8B\x03\x8B\xCB", "xxxxxx");
			cout << "PlayerTick address: " << addrPlayerTick << endl;
			cout << "PlayerChat address: " << addrPlayerChat << endl;
			cout << "ActorSetPosition address: " << addrActorSetPosition << endl;
			cout << "PlayerReceive chat address: " << addrPlayerReceiveChat << endl;
			cout << "ActorGetPosition address: " << addrActorGetPosition << endl;
			cout << "PlayerSetCircuitInputs address: " << addrPlayerSetCircuitInputs << endl;
			cout << "PlayerGetCircuitOutputs address: " << addrPlayerGetCircuitOutputs << endl;
			cout << "PlayerCanJump address: " << addrPlayerCanJump << endl;
			cout << "Nope fast travels found at: " << addrToNopeFastTravelsFirst << endl;
			
			//Performs detours
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourAttach(&(PVOID&)addrPlayerTick, HookPlayerTick);
			DetourAttach(&(PVOID&)addrPlayerChat, HookPlayerChat);
			DetourAttach(&(PVOID&)addrActorSetPosition, HookActorSetPosition);
			DetourAttach(&(PVOID&)addrPlayerCanJump, HookPlayerCanJump);
			DetourTransactionCommit();

			cout << "\nDetours commited. New original addressed:" << endl;
			cout << "PlayerTick: " << addrPlayerTick << endl;
			cout << "PlayerChat: " << addrPlayerChat << endl;
			cout << "ActorSetPosition: " << addrActorSetPosition << endl;
			cout << "PlayerCanJump: " << addrPlayerCanJump << endl;
			cout << endl;

			//Save the functions
			OriginalPlayerTick = (pPlayerTick_t)addrPlayerTick;
			OriginalPlayerChat = (pPlayerChat_t)addrPlayerChat;
			OriginalActorSetPosition = (pActorSetPosition_t)addrActorSetPosition;
			PlayerReceiveChat = (pPlayerReceiveChat_t)addrPlayerReceiveChat;
			ActorGetPosition = (pActorGetPosition_t)addrActorGetPosition;
			PlayerSetCircuitInputs = (pPlayerSetCircuitInputs_t)addrPlayerSetCircuitInputs;
			PlayerGetCircuitOutputs = (pPlayerGetCircuitOutputs_t)addrPlayerGetCircuitOutputs;
			OriginalPlayerCanJump = (pPlayerCanJump_t)addrPlayerCanJump;
			
			addrToNopeFastTravels[0] = addrToNopeFastTravelsFirst;
			addrToNopeFastTravels[1] = addrToNopeFastTravelsFirst + 0x0F;


			cout << dec;

		} else if (dwReason == DLL_PROCESS_DETACH){
			cout << "see you.." << endl;
			fclose(pCout);
			fclose(pCin);
		}
	}
	return true;
}
