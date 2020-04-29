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

const char process[] = "metin2client.exe";

// Patterns of functions for sigscanning
namespace pattern {
	const char MySend[]= "\x56\x57\x8B\xF1\xE8\x00\x00\x00\x00\x8B\xF8\x85\xFF\x7E\x4A\x8B\xCE\xE8\x00\x00\x00\x00\x84\xC0";
	const char MyRecv[] = "\x56\x8B\xF1\x57\x8B\x56\x28\x85\xD2\x7E\x27\x8B\x46\x24\x2B\xC2\x85\xC0\x7E\x11";
	const char Chat[] = "\x55\x8B\xEC\x83\xEC\x0C\x89\x4D\xFC\x8B\x45\x08\x50\xE8\x00\x00\x00\x00\x83\xC4\x04\x85\xC0";
	const char GetTime[] = "\x8B\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\x2B\x0D\x00\x00\x00\x00\x03\xC1\xC3";
	const char AppendChat[] = "\x55\x8B\xEC\x83\xEC\x20\x89\x4D\xFC\xE8\x00\x00\x00\x00\x89\x45\xEC\x83\x7D\xEC\x00\x75\x12\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00";
}

// Mask of patterns for sigscanning
namespace mask {
	const char MySend[] = "xxxxx????xxxxxxxxx????xx";
	const char MyRecv[] = "xxxxxxxxxxxxxxxxxxxx";
	const char Chat[] = "xxxxxxxxxxxxxx????xxxxx";
	const char GetTime[] = "xx????x????xx????xx????xxx";
	const char AppendChat[] = "xxxxxxxxxx????xxxxxxxxxx????x????";
}

namespace addr {
	DWORD Base;
	void* MySend;
	void* MyRecv;
	void* Chat;
	void* GetTime;
	void* AppendChat;
	void* ChatObject;
	void* PlayerObject;
}

namespace pointer_lists {
	vector<DWORD> ChatObject = {0x2fe560, 4};
	vector<DWORD> PacketStruct = {0x2fe438};
	vector<DWORD> PlayerObject = {0x2fc158, 0xc, 0x1d4};
}

void* read_pointer_list(vector<DWORD> offsets){
	DWORD last_offset = offsets.back();
	offsets.pop_back(); // Don't access last offset

	DWORD result = addr::Base;
	for (DWORD offset : offsets){
		result = *(DWORD*)(result + offset);
	}
	return (void*)(result + last_offset);
}

void get_objects_addresses(){
	// Unfortunately we can't get the packet_struct here, because we need id
	// with its state when sending a packet
	while (!ingame())
		Sleep(1000);

	cout << "Getting objects addresses..." << hex << endl;
	addr::PlayerObject = read_pointer_list(pointer_lists::PlayerObject);
	while (addr::PlayerObject == (void*)0x1d4){
		cout << "[ERROR] Getting Player Object pointer. Trying again.." << endl;
		Sleep(2000);
		addr::PlayerObject = read_pointer_list(pointer_lists::PlayerObject);
	}

	cout << "PlayerObject found at " << (DWORD)addr::PlayerObject << endl;

}

void sigscan() {
	cout << "Sigscanning..." << endl;
	SigScan scanner;
	addr::Base = (DWORD)GetModuleHandle(NULL);
	addr::MySend = (void*)scanner.FindPattern(process, pattern::MySend, mask::MySend);
	addr::MyRecv = (void*)scanner.FindPattern(process, pattern::MyRecv, mask::MyRecv);
	addr::Chat = (void*)scanner.FindPattern(process, pattern::Chat, mask::Chat);
	addr::GetTime = (void*)scanner.FindPattern(process, pattern::GetTime, mask::GetTime);
	addr::AppendChat = (void*)scanner.FindPattern(process, pattern::AppendChat, mask::AppendChat);
	addr::ChatObject = read_pointer_list(pointer_lists::ChatObject);

	cout << "MySend found at " << (DWORD)addr::MySend << endl;
	cout << "MyRecv found at " << (DWORD)addr::MyRecv << endl;
	cout << "Chat found at " << (DWORD)addr::Chat << endl;
	cout << "GetTime found at " << (DWORD)addr::GetTime << endl;
	cout << "RecvChat found at " << (DWORD)addr::AppendChat << endl;
	cout << "ChatObject found at " << (DWORD)addr::ChatObject << endl;

	// Function assignation
	OriginalMySend = (pMySend_t)addr::MySend;
	OriginalMyRecv = (pMyRecv_t)addr::MyRecv;
	OriginalChat = (pChat_t)addr::Chat;
	OriginalGetTime = (pGetTime_t)addr::GetTime;
	OriginalAppendChat = (pAppendChat_t)addr::AppendChat;
}

void detours() {
	cout << endl << "Doing detours..." << endl;
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
	cout << "MySend found at " << (DWORD)addr::MySend << endl;
	cout << "MyRecv found at " << (DWORD)addr::MyRecv << endl;
	cout << "Chat found at " << (DWORD)addr::Chat << endl;
	cout << "GetTime found at " << (DWORD)addr::GetTime << endl;
	cout << "RecvChat found at " << (DWORD)addr::AppendChat << endl;
	cout << "ChatObject found at " << (DWORD)addr::ChatObject << endl;

	cout << "DONE!" << endl;
}


int __fastcall HookMySend(packet_struct *_this) {
	string buf(_this->buf_send, _this->buf_send_len);
	string hex_buf = string_to_hex(buf);

	Packet* ppacket = parse_packet_send(buf);
	ppacket->print();

	if (ingame() && !ready_to_send) {
		std_pkt_struct = *_this;
		cout << "Packet copied! We can already send packets." << endl;
		ready_to_send = true;
	}

	if (ppacket->get_header() == HEADER_CG_TARGET){
		ID_ATTACK = ((CG_TargetPacket*)ppacket)->get_id();
		print("New ID selected: " + to_string(ID_ATTACK));
	}

	delete ppacket;
	
	return OriginalMySend(_this);
}

int __fastcall HookMyRecv(packet_struct *_this){
	uint offset = _this->buf_recv_offset;
	uint field28 = _this->field_0x28;
	int ret = OriginalMyRecv(_this);
	uint len = _this->buf_recv_offset + offset - field28;

	string buf = string(_this->buf_recv, len);

	Packet* ppacket = parse_packet_recv(buf);
	//ppacket->print();
	delete ppacket;

	// cout << "recv bytes: " << len << endl;
	// cout << string_to_hex(buf) << endl;

	// recv_offset - 0x28 + len_recv = recv_offset2 --> len_recv = recv_offset2 + 0x28 - recv_offset
	return ret;
}


int __fastcall HookChat(packet_struct *_this, int edx, const char* input, char param2){
	// Client replaces | with || when sending msgs because | are used for
	// colors. Example: |cFFFFFF00|H|hhola. 
	cout << "CHAT: " << input << endl;

	if (input[0] == '$'){
		OriginalAppendChat(addr::ChatObject, 0, replace_all(input, "||", "|").c_str());
		return 1;
	} else if (input[0] == '@'){
		command(input);
		return 1;
	}
	return OriginalChat(_this, replace_all(input, "||", "|").c_str(), param2);
}
