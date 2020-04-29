#include <iostream>
#include <string>
#include <vector>

#include "utils.h"
#include "hooks.h"
#include "packet.h"


using namespace std;

uint ID_ATTACK;

// Sending attack packets?
bool attacking = false;

// Testing
void attack(){
	while (attacking){
		if (ID_ATTACK){
			CG_AttackPacket p(0, ID_ATTACK, 92, 247);
			p.send();
			Sleep(100);
		}
	}
}

void command(string s){
	s = s.substr(1, s.length()-1);
	vector<string> cmd = split(s, ' ');

	// Switch case?
	if (cmd[0] == "move") {
		// move type x y
		if (cmd.size() == 4){
			CG_MovePacket p(stoi(cmd[1]), 0, 0xc, stoi(cmd[2]), stoi(cmd[3]), 0);
			p.send();
		}

	} else if (cmd[0] == "attack") {
		// attack
		if (ID_ATTACK) {
			CG_AttackPacket p(0, ID_ATTACK, 0x0b, 0xcc);
			p.send();
		} else print_err("No hay objetivo seleccionado.");

	} else if (cmd[0] == "msg"){
		// msg type message
		CG_ChatPacket p(stoi(cmd[1]), cmd[2]);
		p.print();
		p.send();

	} else if (cmd[0] == "send"){
		string hexbuf = cmd[1];
		Packet p(hex_to_string(hexbuf));
		p.send();

	} else if (cmd[0] == "start_attack") {
		attacking = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)attack, 0, 0, 0);

	} else if (cmd[0] == "stop_attacking") {
		attacking = false;

	} else if (cmd[0] == "shoot"){
		// TESTING
		string buf("\x33");
		buf += p32(ID_ATTACK);
		buf += string("\x70\xa3\x0e\x00\x71\x81\x02\x00\x00", 9);
		string buf2("\x36\x00\x00", 3);
		Packet p1(buf);
		Packet p2(buf2);
		p1.send();
		p2.send();

	} else if (cmd[0] == "enable_wallhack") {
		print("Enabled wallhack!");
		*(byte*)((DWORD)addr::PlayerObject + 0x490) = 1;
	} else if (cmd[0] == "disable_wallhack") {
		print("Disabled wallhack!");
		*(byte*)((DWORD)addr::PlayerObject + 0x490) = 0;
	} else print_err("Unknown command");

}