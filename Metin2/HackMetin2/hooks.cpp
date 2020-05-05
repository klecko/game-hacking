#include <iostream>
#include <string>
#include <vector>

#include "sigscan.h"
#include "hooks.h"
#include "packet.h"
#include "packet_struct.h"
#include "detours.h"
#include "utils.h"
#include "cmd.h"

#pragma comment(lib, "detours.lib")

using namespace std;

pMySend_t OriginalMySend;
pMyRecv_t OriginalMyRecv;
pChat_t OriginalChat;
pGetTime_t OriginalGetTime;
pAppendChat_t OriginalAppendChat;
pGetAttackByte_t OriginalGetAttackByte;
pEncryptPacket_t OriginalEncryptPacket;

packet_struct* pkt_struct;

const char process[] = "metin2client.exe";

// Function patterns for sigscanning
namespace pattern {
	const char MySend[]= "\x56\x57\x8B\xF1\xE8\x00\x00\x00\x00\x8B\xF8\x85\xFF\x7E\x4A\x8B\xCE\xE8\x00\x00\x00\x00\x84\xC0";
	const char MyRecv[] = "\x56\x8B\xF1\x57\x8B\x56\x28\x85\xD2\x7E\x27\x8B\x46\x24\x2B\xC2\x85\xC0\x7E\x11";
	const char Chat[] = "\x55\x8B\xEC\x83\xEC\x0C\x89\x4D\xFC\x8B\x45\x08\x50\xE8\x00\x00\x00\x00\x83\xC4\x04\x85\xC0";
	const char GetTime[] = "\x8B\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\x2B\x0D\x00\x00\x00\x00\x03\xC1\xC3";
	const char AppendChat[] = "\x55\x8B\xEC\x83\xEC\x20\x89\x4D\xFC\xE8\x00\x00\x00\x00\x89\x45\xEC\x83\x7D\xEC\x00\x75\x12\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00";
	const char GetAttackByte[] = "\x55\x8B\xEC\x51\x0F\xB6\x05\x74\x06\x91\x00\x0F\xB6\x88\x6C\x06\x91\x00\x0F\xB6\x15\x74\x06\x91\x00\x0F\xB6\x82\xB8\x17\x8E\x00\x33\xC8\x88\x4D\xFF";
}

// Patterns masks for sigscanning
namespace mask {
	const char MySend[] = "xxxxx????xxxxxxxxx????xx";
	const char MyRecv[] = "xxxxxxxxxxxxxxxxxxxx";
	const char Chat[] = "xxxxxxxxxxxxxx????xxxxx";
	const char GetTime[] = "xx????x????xx????xx????xxx";
	const char AppendChat[] = "xxxxxxxxxx????xxxxxxxxxx????x????";
	const char GetAttackByte[] = "xxxxxxx????xxx????xxx????xxx????xxxxx";
}

namespace addr {
	DWORD Base;
	void* MySend;
	void* MyRecv;
	void* Chat;
	void* GetTime;
	void* AppendChat;
	void* GetAttackByte;
	void* ChatObject;
	void* PlayerObject;
	void* PacketStruct;
	void* CryptObject;
}

namespace pointer_lists {
	vector<DWORD> ChatObject = {0x2fe560, 4};
	vector<DWORD> PacketStruct = {0x2fe438, 0};
	vector<DWORD> PlayerObject = {0x2fc158, 0xc, 0x1d4};
}

void* read_pointer_list(vector<DWORD> offsets){
	DWORD last_offset = offsets.back();
	offsets.pop_back(); // Don't access last offset

	DWORD result = addr::Base;
	for (DWORD offset : offsets)
		result = *(DWORD*)(result + offset);

	return (void*)(result + last_offset);
}

void get_objects_addresses(){
	// Unfortunately we can't get the packet_struct here, because we need it
	// with its state when sending a packet
	while (!ingame())
		Sleep(1000);

	cout << "Getting objects addresses..." << hex << endl;
	addr::PacketStruct = read_pointer_list(pointer_lists::PacketStruct);
	addr::PlayerObject = read_pointer_list(pointer_lists::PlayerObject);
	while (addr::PlayerObject == (void*)0x1d4){
		cout << "[ERROR] Getting Player Object pointer. Trying again.." << endl;
		Sleep(2000);
		addr::PlayerObject = read_pointer_list(pointer_lists::PlayerObject);
	}

	pkt_struct = (packet_struct*)(addr::PacketStruct);
	addr::CryptObject = (char*)pkt_struct->pointer_crypto + 4;
	OriginalEncryptPacket = *(pEncryptPacket_t*)( (*(DWORD*)(addr::CryptObject))+0x1C );

	cout << "Packet struct found at " << (DWORD)addr::PacketStruct << endl;
	cout << "PlayerObject found at " << (DWORD)addr::PlayerObject << endl;
	cout << "OriginalEncryptPacket found at " << (DWORD)OriginalEncryptPacket << endl;
	cout << endl;
}

void sigscan() {
	cout << "Sigscanning..." << endl;
	SigScan scanner;
	addr::Base = (DWORD)GetModuleHandle(NULL);
	addr::MySend = scanner.FindPattern(process, pattern::MySend, mask::MySend);
	addr::MyRecv = scanner.FindPattern(process, pattern::MyRecv, mask::MyRecv);
	addr::Chat = scanner.FindPattern(process, pattern::Chat, mask::Chat);
	addr::GetTime = scanner.FindPattern(process, pattern::GetTime, mask::GetTime);
	addr::AppendChat = scanner.FindPattern(process, pattern::AppendChat, mask::AppendChat);
	addr::GetAttackByte = scanner.FindPattern(process, pattern::GetAttackByte, mask::GetAttackByte);
	addr::ChatObject = read_pointer_list(pointer_lists::ChatObject);

	// Function assignation
	OriginalMySend = (pMySend_t)addr::MySend;
	OriginalMyRecv = (pMyRecv_t)addr::MyRecv;
	OriginalChat = (pChat_t)addr::Chat;
	OriginalGetTime = (pGetTime_t)addr::GetTime;
	OriginalAppendChat = (pAppendChat_t)addr::AppendChat;
	OriginalGetAttackByte = (pGetAttackByte_t)addr::GetAttackByte;

	cout << "MySend found at " << (DWORD)addr::MySend << endl;
	cout << "MyRecv found at " << (DWORD)addr::MyRecv << endl;
	cout << "Chat found at " << (DWORD)addr::Chat << endl;
	cout << "GetTime found at " << (DWORD)addr::GetTime << endl;
	cout << "AppendChat found at " << (DWORD)addr::AppendChat << endl;
	cout << "GetAttackByte found at " << (DWORD)addr::GetAttackByte << endl;
	cout << "ChatObject found at " << (DWORD)addr::ChatObject << endl;
	cout << endl;
}

void detours() {
	cout << "Doing detours..." << endl;
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&addr::MySend, HookMySend);
	DetourAttach(&addr::MyRecv, HookMyRecv);
	DetourAttach(&addr::Chat, HookChat);
	DetourTransactionCommit();

	// Function update, as original func addresses change after detour
	OriginalMySend = (pMySend_t)addr::MySend;
	OriginalMyRecv = (pMyRecv_t)addr::MyRecv;
	OriginalChat = (pChat_t)addr::Chat;

	cout << "MySend is now at " << (DWORD)addr::MySend << endl;
	cout << "MyRecv is now at " << (DWORD)addr::MyRecv << endl;
	cout << "Chat is now at " << (DWORD)addr::Chat << endl;
	cout << endl;
}


int __fastcall HookMySend(packet_struct *_this) {
	int ret;
	string buf(_this->buf_send, _this->buf_send_len);
	string hex_buf = string_to_hex(buf);

	// Get that f***ing packet_struct
	if (ingame() && !ready_to_send) {
		std_pkt_struct = *_this;
		cout << "Packet copied! We can already send packets." << endl;
		ready_to_send = true;
	}

	Packet* ppacket = parse_packet_send(buf);
	ppacket->log();

	// If on_hook, attach the packet to the packet_struct and send it.
	// Else, continue execution and send with no changes.
	ret = (ppacket->on_hook() ? ppacket->send(_this) : OriginalMySend(_this));

	delete ppacket;
	return ret;
}

int __fastcall HookMyRecv(packet_struct *_this){
	uint offset = _this->buf_recv_offset;
	uint field28 = _this->field_0x28;
	int ret = OriginalMyRecv(_this);
	// recv_offset - 0x28 + len_recv = recv_offset2 --> len_recv = recv_offset2 + 0x28 - recv_offset
	uint len = _this->buf_recv_offset - offset + field28;

	string buf = string(_this->buf_recv, len);

	Packet* ppacket = parse_packet_recv(buf);
	//ppacket->log();
	ppacket->on_hook();

	delete ppacket;
	return ret;
}


int __fastcall HookChat(packet_struct *_this, int edx, const char* input, char param2){
	// Client replaces | with || when sending msgs because | are used for
	// colors. Example: |cFFFFFF00|H|hhola. 
	cout << "CHAT: " << input << endl;

	if (input[0] == '$'){ // for testing and not sending the msg
		OriginalAppendChat(addr::ChatObject, 0, Command::process_msg(input).c_str());
		return 1;
	} else if (input[0] == '@'){ // commands
		Command::run(input+1);
		return 1;
	}
	return OriginalChat(_this, Command::process_msg(input).c_str(), param2);
}
