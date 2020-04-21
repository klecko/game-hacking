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


namespace addr {
	DWORD Base;
	void* MySend;
	void* MyRecv;
	void* Chat;
	void* GetTime;
	void* AppendChat;
	void* ChatObject;
}

void sigscan() {
	cout << "Sigscanning..." << endl;
	SigScan scanner;
	addr::Base = (DWORD)GetModuleHandle(NULL);
	addr::MySend = (void*)scanner.FindPattern("metin2client.exe", "\x56\x57\x8B\xF1\xE8\x00\x00\x00\x00\x8B\xF8\x85\xFF\x7E\x4A\x8B\xCE\xE8\x00\x00\x00\x00\x84\xC0", "xxxxx????xxxxxxxxx????xx");
	addr::MyRecv = (void*)scanner.FindPattern("metin2client.exe", "\x56\x8B\xF1\x57\x8B\x56\x28\x85\xD2\x7E\x27\x8B\x46\x24\x2B\xC2\x85\xC0\x7E\x11", "xxxxxxxxxxxxxxxxxxxx");
	addr::Chat = (void*)scanner.FindPattern("metin2client.exe", "\x55\x8B\xEC\x83\xEC\x0C\x89\x4D\xFC\x8B\x45\x08\x50\xE8\x00\x00\x00\x00\x83\xC4\x04\x85\xC0", "xxxxxxxxxxxxxx????xxxxx");
	addr::GetTime = (void*)scanner.FindPattern("metin2client.exe", "\x8B\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\x2B\x0D\x00\x00\x00\x00\x03\xC1\xC3", "xx????x????xx????xx????xxx");
	addr::AppendChat = (void*)scanner.FindPattern("metin2client.exe", "\x55\x8B\xEC\x83\xEC\x20\x89\x4D\xFC\xE8\x00\x00\x00\x00\x89\x45\xEC\x83\x7D\xEC\x00\x75\x12\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00", "xxxxxxxxxx????xxxxxxxxxx????x????");
	addr::ChatObject = (void*)(*(DWORD*)(addr::Base + 0x2fe560)+4); //[base+0x2fe560]+4

	cout << "MySend found at " << (DWORD)addr::MySend << endl;
	cout << "MyRecv found at " << (DWORD)addr::MyRecv << endl;
	cout << "Chat found at " << (DWORD)addr::Chat << endl;
	cout << "GetTime found at " << (DWORD)addr::GetTime << endl;
	cout << "RecvChat found at " << (DWORD)addr::AppendChat << endl;
	cout << "ChatObject found at " << (DWORD)addr::ChatObject << endl;
}

void detours() {
	cout << "Doing detours..." << endl;
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&addr::MySend, HookMySend);
	DetourAttach(&addr::MyRecv, HookMyRecv);
	DetourAttach(&addr::Chat, HookChat);
	DetourTransactionCommit();

	OriginalMySend = (pMySend_t)addr::MySend;
	OriginalMyRecv = (pMyRecv_t)addr::MyRecv;
	OriginalChat = (pChat_t)addr::Chat;
	OriginalGetTime = (pGetTime_t)addr::GetTime; //not hooking
	OriginalAppendChat = (pAppendChat_t)addr::AppendChat;

	cout << "DONE!" << endl;
}


int __fastcall HookMySend(packet_struct *_this) {
	//Parece que el ultimo byte del final sigue una secuencia.
	//en los primeros bytes nos encontramos un 01 seguido de un 2e
	//Hacer un metodo de packet que te devuelva un packet_struct.
	//hay direcciones que cambian con cada logeo, como 0x50 (pointer_crypto?).
	//pillarlas en un paquete y guardarlas
	string buf(_this->buf_send, _this->buf_send_len);
	string hex_buf = string_to_hex(buf);

	//string msg = color("ffff00") + "hola mundo" + color("ff0000") + "adiosjeje";
	//OriginalAppendChat(addr::ChatObject, 0, msg.c_str());

	Packet* ppacket = parse_packet_send(buf);
	ppacket->print();

	if (ppacket->get_header() == HEADER_CG_MOVE){
		if (!ready_to_send){
			std_pkt_struct = *_this;
			print("Packet copied! You can already send packets.");
			ready_to_send = true;
		}
	} else if (ppacket->get_header() == HEADER_CG_TARGET){
		ID_ATTACK = ((CG_TargetPacket*)ppacket)->get_id();
		print("New ID selected: " + to_string(ID_ATTACK));
	}

	if (!ppacket->get_header()) cout << hex_buf << endl;
	delete ppacket;
	
	//cout << "Last byte: " << hex_buf[_this->buf_send_len * 2 - 2] << hex_buf[_this->buf_send_len * 2 - 1] << endl;
	return OriginalMySend(_this);
}

int __fastcall HookMyRecv(packet_struct *_this){
	uint offset = _this->buf_recv_offset;
	uint field28 = _this->field_0x28;
	int ret = OriginalMyRecv(_this);
	uint len = _this->buf_recv_offset + offset - field28;

	string buf = string(_this->buf_recv, len);

	Packet* ppacket = parse_packet_recv(buf);
	ppacket->print();
	delete ppacket;

	// cout << "recv bytes: " << len << endl;
	// cout << string_to_hex(buf) << endl;

	// recv_offset - 0x28 + len_recv = recv_offset2 --> len_recv = recv_offset2 + 0x28 - recv_offset
	return ret;
}


int __fastcall HookChat(packet_struct *_this, int edx, const char* input, char param2){
	cout << "CHAT: " << input << endl;

	if (input[0] == '$'){
		OriginalAppendChat(addr::ChatObject, 0, replace_all(input, "||", "|").c_str());
		return 1;
	}
	if (!(input[0] == '@'))
		return OriginalChat(_this, replace_all(input, "||", "|").c_str(), param2);

	command(input);

	return 1;
}