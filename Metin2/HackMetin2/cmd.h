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
#include "packet.h"

typedef unsigned char byte;
typedef unsigned int uint;


class Command {
private:
	static const std::map<std::string, std::string> help_msgs;

	// Disconnect
	static const int DISCONNECT_DEFAULT_PACKETS = 20;
	static const int DISCONNECT_PACKET_LEN = 500;
	static const byte DISCONNECT_BYTE = '\x18';

	// Packet injection. Set INJ_PADDING_BYTE to DISCONNECT_BYTE for
	// debugging. That way, if padding is not correct, client will crash. Otherwise,
	// the packet can pass unnoticed, as ' ' is a valid packet header.
	static const int INJ_PACKET_COUNT = 8;
	static const int INJ_PACKET_LEN = 503;
	static const int INJ_LAST_PACKET_PAD = 71;
	static const byte INJ_PADDING_BYTE = ' ';

	static const int TEST_CHAT_DEFAULT_PACKETS = 5;
	static const int TEST_CHAT_PACKET_LEN = 2000;

	// Forbidden methods
	Command(){};
	Command(const Command&){};
	Command& operator=(const Command&){};

	// Instance attributes
	// Chosen target, changed when CG_Target is sent
	uint target;
	std::string username_dc;
	std::map<uint, Enemy> enemies;
	bool autodmg_enabled;
	bool disconnecting;
	int disconnect_packets;

	bool test_chatting;
	int test_chat_packets;


	// Instance
	static Command* const instance;

	static bool check_n_args(uint n, const std::vector<std::string>& cmd);
	static void _autodmg();
	static void _packet_injection(const std::string& username, Packet* p);
	static void _disconnect();

	static void _test_chat();

public:
	// Class methods (interface)
	// If they need an instance attribute, they'll access it through instance

	// Getters and setters
	static void set_target(uint target);
	static uint get_target();
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
	static void porculing();
	static void run_GC_command(const std::string& cmd);
	static void hack_life(const std::string& username, const std::string& cmd);
	static void ghostmode();

	static void test_chat(int n_packets=TEST_CHAT_DEFAULT_PACKETS);

	static void run(const std::string& cmd);
};
