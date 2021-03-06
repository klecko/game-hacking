#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "sigscan.h"
#include "hooks.h"
#include "packet.h"
#include "packet_struct.h"
#include "detours.h"
#include "utils.h"
#include "cmd.h"
#include "player.h"

#pragma comment(lib, "detours.lib")

using namespace std;

pMySend_t OriginalMySend;
pMyRecv_t OriginalMyRecv;
pChat_t OriginalChat;
pGetTime_t OriginalGetTime;
pAppendChat_t OriginalAppendChat;
pGetAttackByte_t OriginalGetAttackByte;
pPlayerRandomFunc_t OriginalPlayerRandomFunc;
pChatRandomFunc_t OriginalChatRandomFunc;
pParseRecvPacket_t OriginalParseRecvPacket;

const char process[] = "metin2client.exe";

// Function patterns for sigscanning
namespace pattern {
	const char MySend[]= "\x56\x57\x8B\xF1\xE8\x00\x00\x00\x00\x8B\xF8\x85\xFF\x7E\x4A\x8B\xCE\xE8\x00\x00\x00\x00\x84\xC0";
	const char MyRecv[] = "\x56\x8B\xF1\x57\x8B\x56\x28\x85\xD2\x7E\x27\x8B\x46\x24\x2B\xC2\x85\xC0\x7E\x11";
	const char Chat[] = "\x55\x8B\xEC\x83\xEC\x0C\x89\x4D\xFC\x8B\x45\x08\x50\xE8\x00\x00\x00\x00\x83\xC4\x04\x85\xC0";
	const char GetTime[] = "\x8B\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\x2B\x0D\x00\x00\x00\x00\x03\xC1\xC3";
	const char AppendChat[] = "\x55\x8B\xEC\x83\xEC\x20\x89\x4D\xFC\xE8\x00\x00\x00\x00\x89\x45\xEC\x83\x7D\xEC\x00\x75\x12\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00";
	const char GetAttackByte[] = "\x55\x8B\xEC\x51\x0F\xB6\x05\x74\x06\x91\x00\x0F\xB6\x88\x6C\x06\x91\x00\x0F\xB6\x15\x74\x06\x91\x00\x0F\xB6\x82\xB8\x17\x8E\x00\x33\xC8\x88\x4D\xFF";
	const char PlayerRandomFunc[] = "\x55\x8b\xec\x83\xec\x18\x56\x57\x8b\x7d\x08\x8b\xf1\x8b\xcf\xe8";
	const char ChatRandomFunc[] = "\x55\x8B\xEC\x83\xEC\x24\x89\x4D\xFC\x8B\x45\x08\x50\x8B\x4D\xFC";
	const char ParseRecvPacket[] = "\x55\x8B\xEC\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x83\xEC\x20\xA1\x00\x00\x00\x00\x33\xC5\x50\x8D\x45\xF4\x64\xA3\x00\x00\x00\x00\x89\x4D\xEC\x8B\x4D\xEC\x81\xC1\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x0F\xB6\xC0";
}

// Patterns masks for sigscanning
namespace mask {
	const char MySend[] = "xxxxx????xxxxxxxxx????xx";
	const char MyRecv[] = "xxxxxxxxxxxxxxxxxxxx";
	const char Chat[] = "xxxxxxxxxxxxxx????xxxxx";
	const char GetTime[] = "xx????x????xx????xx????xxx";
	const char AppendChat[] = "xxxxxxxxxx????xxxxxxxxxx????x????";
	const char GetAttackByte[] = "xxxxxxx????xxx????xxx????xxx????xxxxx";
	const char PlayerRandomFunc[] = "xxxxxxx????x?x?x";
	const char ChatRandomFunc[] = "xxxxxxxxxxxxxxxx";
	const char ParseRecvPacket[] = "xxxxxx????xx????xxxxx????xxxxxxxx????xxxxxxxx????x????xxx";
}

namespace objects {
	void* Chat = nullptr;
	player* Player = nullptr;
}

/*namespace pointer_lists {
	vector<DWORD> ChatObject = {0x2fe560, 4};
	vector<DWORD> PacketStruct = {0x2fe438};
	vector<DWORD> PlayerObject = {0x2fc158, 0xc, 0x1d4};
}

void* read_pointer_list(vector<DWORD> offsets){
	DWORD last_offset = offsets.back();
	offsets.pop_back(); // Don't access last offset

	DWORD result = (DWORD)GetModuleHandle(NULL);
	for (DWORD offset : offsets)
		result = *(DWORD*)(result + offset);

	return (void*)(result + last_offset);
}


void get_objects_addresses(){
	// Unfortunately we can't get the packet_struct here, because we need it
	// with its state when sending a packet
	while (!ingame())
		Sleep(1000);


	cout << "[OBJECTS] Starting..." << hex << endl;
	//objects::Chat = read_pointer_list(pointer_lists::ChatObject);
	//objects::Player = (player*)read_pointer_list(pointer_lists::PlayerObject);

	//cout << "[OBJECTS] Chat is at " << (DWORD)objects::Chat << endl;
	//cout << "[OBJECTS] Player is at " << (DWORD)objects::Player << endl;
	cout << "[OBJECTS] Done!" << endl;
	cout << endl;
}*/

// Calls scanner.FindPattern with error checking. Can return.
// I'm sorry, a project of mine *must* have a pair of ugly macros.
#define findfunc(func) \
	Original##func = (p##func##_t)scanner.FindPattern(process, pattern::func, mask::func);  \
	if (Original##func != nullptr)                                                          \
		cout << "[SIGSCAN] " #func " found at " << (DWORD)Original##func << endl;           \
	else {                                                                                  \
		cout << "[SIGSCAN] Not found " #func "!" << endl;                                   \
		return false;                                                                       \
	}

bool sigscan() {
	cout << "[SIGSCAN] Starting..." << endl;
	SigScan scanner;

	findfunc(MySend);
	findfunc(MyRecv);
	findfunc(Chat);
	findfunc(GetTime);
	findfunc(AppendChat);
	findfunc(GetAttackByte);
	findfunc(PlayerRandomFunc);
	findfunc(ChatRandomFunc);
	findfunc(ParseRecvPacket);

	cout << "[SIGSCAN] Done!" << endl << endl;
	return true;
}

// Calls DetourAttach with error checking. Can return.
#define detour(func)                                            \
	err = DetourAttach((void**)&Original##func, Hook##func);    \
	if (err != NO_ERROR){                                       \
		cout << "[DETOURS] Error detouring " #func "!" << endl; \
		return false;                                           \
	}

bool detours() {
	DWORD err;
	cout << "[DETOURS] Starting..." << endl;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	detour(MySend);
	detour(MyRecv);
	detour(Chat);
	detour(PlayerRandomFunc);
	detour(ChatRandomFunc);
	err = DetourTransactionCommit();
	if (err != NO_ERROR){
		cout << "[DETOURS] Error commiting detours!" << endl;
		return false;
	}

	cout << "[DETOURS] MySend is now at " << (DWORD)OriginalMySend << endl;
	cout << "[DETOURS] MyRecv is now at " << (DWORD)OriginalMyRecv << endl;
	cout << "[DETOURS] Chat is now at " << (DWORD)OriginalChat << endl;
	cout << "[DETOURS] PlayerRandomFunc is now at " << (DWORD)OriginalPlayerRandomFunc << endl;
	cout << "[DETOURS] ChatRandomFunc is now at " << (DWORD)OriginalChatRandomFunc << endl;
	cout << "[DETOURS] Done!" << endl << endl;
	return true;
}

void save_packet_struct(packet_struct* pkt_struct){
	// There seem to be more than one pkt_struct, maybe one for login?
	// The one we want has 0x20000 in buf_recv_len
	if (ingame() && !ready_to_send && pkt_struct->buf_recv_len == 0x20000) {
		std_pkt_struct = *pkt_struct;
		cout << "Packet copied! We can already send packets." << endl;
		ready_to_send = true;
	}
}

int __fastcall HookMySend(packet_struct *_this) {
	int ret = 1;
	string buf(_this->buf_send + _this->buf_send_offset, _this->buf_send_len - _this->buf_send_offset);
	string hex_buf = string_to_hex(buf);

	// Get that f***ing packet_struct
	save_packet_struct(_this);

	// Doing a process_send_packet and detecting when a packet has been
	// modified when there are several packets seems difficult. I'm not doing it atm.
	try {
		Packet* ppacket = parse_packet_send(buf);
		ppacket->log();

		// If on_hook, attach the packet to the packet_struct and send it.
		// Else, continue execution and send with no changes.
		ret = (ppacket->on_hook() ? ppacket->send(_this) : OriginalMySend(_this));
		delete ppacket;

	} catch (exception& e) {
		cout << "[ERROR] Send hook: " << e.what() << endl;
	}

	return ret;
}

uint process_recv_packet(const string& buf){
	uint size = buf.size();
	string hex_buf = string_to_hex(buf);
	try {
		Packet* ppacket = parse_packet_recv(buf);
		// For logging
		vector<int> allow = { HEADER_GC_WHISPER };
		vector<int> disallow = { HEADER_GC_CHAT, HEADER_GC_MOVE };
		//if (find(disallow.begin(), disallow.end(), buf[0]) == disallow.end())
		if (find(allow.begin(), allow.end(), ppacket->get_buf()[0]) != allow.end()){
			cout << "[RECV IMPORTANT] " << hex_buf << endl;
		}

		// Too much noise
		//ppacket->log();
		ppacket->on_hook();

		size = ppacket->bufsize();
		delete ppacket;

	} catch (exception& e) {
		cout << "[ERROR] Processing recv packet: " << e.what() << endl;
	}
	return size;
}

int __fastcall HookMyRecv(packet_struct *_this){
	// When client recvs, it cleans already_processed bytes from recv_buf and
	// sets already_processed to 0. After receiving, as there can be more than
	// one packet, each time it reads from recv_buf for parsing it increases
	// already_processed the number of bytes read.

	// buf_recv_offset - already_processed + len_received = new_buf_recv_offset
	// len_received = new_buf_recv_offset - buf_recv_offset + already_processed
	uint offset = _this->buf_recv_offset;
	uint already_processed = _this->already_processed;
	int ret = OriginalMyRecv(_this);
	uint len = _this->buf_recv_offset - offset + already_processed;

	// equals _this->buf_recv + offset - already_processed
	string buf = string(_this->buf_recv + _this->buf_recv_offset - len, len);
	string hex_buf = string_to_hex(buf);

	// Process every packet in the buf. Will stop when it reaches an unknown
	// packet because we won't know its size.
	int i = 0;
	already_processed = 0;
	while (already_processed < buf.size()){
		string buf_pkt = buf.substr(already_processed);
		already_processed += process_recv_packet(buf_pkt);
		i++;
	}
	//cout << "Processed " << i << " recv packets" << endl;
	return ret;
}


int __fastcall HookChat(packet_struct *_this, int edx, const char* input, char param2){
	// Client replaces | with || when sending msgs because | are used for
	// colors. Example: |cFFFFFF00|H|hhola. 
	cout << "CHAT: " << input << endl;

	if (input[0] == '$'){ // for testing and not sending the msg
		OriginalAppendChat(objects::Chat, 0, Command::process_msg(input).c_str());
		return 1;
	} else if (input[0] == '@'){ // commands
		try {
			Command::run(input+1);
		} catch (exception& e) {
			cout << "[ERROR] Running command " << input+1 << ": " << e.what() << endl;
			print_err("Error running command");
		}
		return 1;
	}
	return OriginalChat(_this, Command::process_msg(input).c_str(), param2);
}

// Simple hook for getting the player object.
bool __fastcall HookPlayerRandomFunc(player* _this, int edx, void* arg){
	if (objects::Player == nullptr){
		objects::Player = _this;
		cout << "[OBJECTS] Player is at " << hex << (DWORD)objects::Player << dec << endl;
	}
	return OriginalPlayerRandomFunc(_this, arg);
}

// Simple hook for getting the chat object.
void __fastcall HookChatRandomFunc(void *_this, int edx, void* arg){
	if (objects::Chat == nullptr){
		objects::Chat = (void*)((DWORD)_this+4);
		cout << "[OBJECTS] Chat is at " << hex << (DWORD)objects::Chat << dec << endl;
	}
	OriginalChatRandomFunc(_this, arg);
}
