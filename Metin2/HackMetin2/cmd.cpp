#include <iostream>
#include <string>
#include <vector>

#include "cmd.h"
#include "utils.h"
#include "hooks.h"
#include "packet.h"


using namespace std;

Command* Command::instance = new Command();

bool Command::check_n_args(int n, vector<string> cmd){
	if (cmd.size() == n+1)
		return true;
	print_err("Wrong arguments!");
	return false;
}

void Command::send(std::string hexbuf){
	Packet p(hex_to_string(hexbuf));
	p.send();
}

void Command::move(byte type, int x, int y){
	CG_MovePacket p(type, 0, 0xc, x, y, 0);
	p.send();
}

void Command::set_id_attack(int id_attack){
	instance->id_attack = id_attack;
	print("New ID selected: " + to_string(id_attack));
}

uint Command::get_id_attack(){
	return instance->id_attack;
}

void Command::attack(){
	if (instance->id_attack){
		CG_AttackPacket p(0, instance->id_attack, 0x0b, 0xcc);
		p.send();
	} else print_err("There's no target.");
}

void Command::msg(byte type, std::string msg){
	CG_ChatPacket p(type, msg);
	p.send();
}

void Command::set_wallhack(bool activated){
	*(byte*)((DWORD)addr::PlayerObject + 0x490) = (byte)activated;
	string msg = "Set wallhack ";
	msg += (activated ? "on." : "off.");
	print(msg);
}

void Command::run(string _cmd){
	vector<string> cmd = split(_cmd, ' ');

	if (cmd[0] == "move") {
		if (check_n_args(3, cmd))
			move(stoi(cmd[1]), stoi(cmd[2]), stoi(cmd[3]));

	} else if (cmd[0] == "attack") {
		if (check_n_args(0, cmd))
			attack();

	} else if (cmd[0] == "msg"){
		if (check_n_args(2, cmd))
			msg(stoi(cmd[1]), cmd[2]);

	} else if (cmd[0] == "send"){
		if (check_n_args(1, cmd))
			send(cmd[1]);

	} else if (cmd[0] == "set_wallhack"){
		if (check_n_args(1, cmd))
			set_wallhack((bool)cmd[1][0]);

	} else if (cmd[0] == "shoot"){
		// TESTING
		string buf("\x33");
		buf += p32(instance->id_attack);
		buf += string("\x70\xa3\x0e\x00\x71\x81\x02\x00\x00", 9);
		string buf2("\x36\x00\x00", 3);
		Packet p1(buf);
		Packet p2(buf2);
		p1.send();
		p2.send();

	} else print_err("Unknown command");

}