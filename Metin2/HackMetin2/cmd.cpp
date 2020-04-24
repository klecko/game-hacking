#include <iostream>
#include <string>
#include <vector>

#include "utils.h"
#include "hooks.h"
#include "packet.h"


using namespace std;

// Chosen target, changed when CG_TargetPacket is sent
uint ID_ATTACK;

void command(string s){
	s = s.substr(1, s.length()-1);
	vector<string> cmd = split(s, ' ');

	if (cmd[0] == "move") {
		// move type x y
		CG_MovePacket p(stoi(cmd[1]), 0, 0xc, stoi(cmd[2]), stoi(cmd[3]), 0);
		p.send();
	} else if (cmd[0] == "attack") {
		if (ID_ATTACK) {
			CG_AttackPacket p(0, ID_ATTACK, 0x0b, 0xcc);
			p.send();
		} else print_err("No hay objetivo seleccionado.");
	} else if (cmd[0] == "msg"){
		//string buf("\x03\x08\x00\x08\x71\x77\x65\x00\x00", 9);
		//CG_ChatPacket p(buf);
		CG_ChatPacket p(stoi(cmd[1]), cmd[2]);
		p.print();
		p.send();
	} else print_err("Unknown command");

}