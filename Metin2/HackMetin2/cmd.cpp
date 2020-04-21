#include <iostream>
#include <string>
#include <vector>

#include "utils.h"
#include "hooks.h"
#include "packet.h"


using namespace std;

uint ID_ATTACK;

void command(string s){
	s = s.substr(1, s.length()-1);
	vector<string> cmd = split(s, ' ');

	if (cmd[0] == "move") {
		CG_MovePacket p(stoi(cmd[1]), 0, 0xc, stoi(cmd[2]), stoi(cmd[3]), 0);
		p.send();
	} else if (cmd[0] == "attack") {
		if (ID_ATTACK) {
			CG_AttackPacket p(0, ID_ATTACK, 0x0b, 0xcc);
			p.send();
		}else print_err("No hay objetivo seleccionado.");
	} else print_err("Unknown command");

}