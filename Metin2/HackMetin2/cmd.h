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
 * Current commands are documented in Command::help_msgs.
*/

#pragma once

#include <string>
#include <vector>
#include <map>

#include "enemy.h"
#include "player.h"
#include "packet.h"

typedef unsigned char byte;
typedef unsigned int uint;


class Command {
private:
	static const std::map<std::string, std::string> help_msgs;
	static const int DISCONNECT_DEFAULT_PACKETS = 20;
	static const int DISCONNECT_PACKET_LEN = 500;
	static const byte DISCONNECT_BYTE = '\x18';
	static const int INJECTION_PACKET_COUNT = 8;
	static const int INJECTION_PACKET_LEN = 500;

	// Forbidden methods
	Command(){};
	Command(const Command&){};
	Command& operator=(const Command&){};

	// Instance attributes
	// Chosen target, changed when CG_Target is sent
	uint id_attack;
	std::string username_dc;
	std::map<uint, Enemy> enemies;
	bool autodmg_enabled;
	bool disconnecting;
	int disconnect_packets;

	// Instance
	static Command* const instance;

	static bool check_n_args(uint n, const std::vector<std::string>& cmd);
	static void _autodmg();
	static void _packet_injection(const std::string& username, Packet* p);
	static void _disconnect();

public:
	// Class methods (interface)
	// If they need an instance attribute, they'll access it through instance

	// Getters and setters
	static void set_id_attack(uint id_attack);
	static uint get_id_attack();
	static void update_enemy(uint id, int x, int y);
	static void delete_enemy(uint id);

	// Hacks
	static void help();
	static void help(const std::string& what);
	static void send(const std::string& hexbuf);
	static void move(byte type, int x, int y);
	static void attack();
	static void autodmg(bool enabled);
	static std::string process_msg(std::string msg);
	static void msg(byte type, const std::string& msg);
	static void set_wallhack(bool enabled);
	static void disconnect(const std::string& username, int n_packets=DISCONNECT_DEFAULT_PACKETS);
	static void whisper(byte type, const std::string& to_user, std::string from_user, std::string msg);
	static void packet_injection(const std::string& username, const std::string& hexbuf);
	static void run(const std::string& cmd);
};
