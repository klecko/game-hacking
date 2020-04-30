#pragma once

#include <string>
#include <vector>

//extern uint ID_ATTACK;

/*
 * Handles user input from chat. The command is s. User must type @s, and s
 * gets here from the hook of Chat.
 *
 * Current commands:
 * - move type x y: moves to coords (x, y)
 * - attack: attacks the chosen target
 * - msg type message: sends the message as selected type
 * - send hexbuf: sends the packet built with the hextbuf
 *
 * TODO: handle errors PL0X
*/

typedef unsigned char byte;
typedef unsigned int uint;

class Command {
private:
	// Forbidden methods
	Command(){};
	Command(const Command&){};
	Command& operator=(const Command&){};

	// Instance attributes
	// Chosen target, changed when CG_TargetPacket is sent
	uint id_attack;

	// Instance
	static Command* instance;

	static bool check_n_args(int n, std::vector<std::string> cmd);

public:
	// Class methods (interface)
	// If they need an instance attribute, they'll get it through instance
	static void send(std::string hexbuf);
	static void move(byte type, int x, int y);
	static void set_id_attack(int id_attack);
	static uint get_id_attack();
	static void attack();
	static void msg(byte type, std::string msg);
	static void set_wallhack(bool activated);
	static void run(std::string cmd);
};
