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
	} else print_err("Unknown command");

}