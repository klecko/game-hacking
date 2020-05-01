/**
 * Command - provides hack functionality
 *
 * This class implements the singleton pattern design. Its interface is made of
 * class methods exclusively. They are the main interface with the user, and
 * provide most of the hack functionality using the abstraction given by the
 * packet module.
 *
 * The method `run` is the intermediary between the user input in the chat and
 * and the rest of the interface. It handles the typed command and calls the
 * corresponding function.
 *
 * Current commands:
 *     - send hexbuf: sends the packet built with the hexbuf
 *     - move type x y: moves to coords (x, y)
 *     - attack: attacks the chosen target
 *     - msg type message: sends the message as selected type
 *     - wallhack 0/1: enables or disables the wallhack
*/

#pragma once

#include <string>
#include <vector>
#include <map>

typedef unsigned char byte;
typedef unsigned int uint;

class Command {
private:
	static const std::map<std::string, std::string> help_msgs;

	// Forbidden methods
	Command(){};
	Command(const Command&){};
	Command& operator=(const Command&){};

	// Instance attributes
	// Chosen target, changed when CG_TargetPacket is sent
	uint id_attack;

	// Instance
	static Command* const instance;

	static bool check_n_args(uint n, std::vector<std::string> cmd);

public:
	// Class methods (interface)
	// If they need an instance attribute, they'll access it through instance

	// Getters and setters
	static void set_id_attack(int id_attack);
	static uint get_id_attack();

	// Hacks
	static void help();
	static void help(std::string what);
	static void send(std::string hexbuf);
	static void move(byte type, int x, int y);
	static void attack();
	static void msg(byte type, std::string msg);
	static void set_wallhack(bool activated);
	static void run(std::string cmd);
};
