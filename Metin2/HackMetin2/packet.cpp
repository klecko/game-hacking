#include <iostream>
#include <string>
#include <assert.h>

#include "packet.h"
#include "utils.h"
#include "hooks.h"

using namespace std;

typedef unsigned char byte;

packet_struct std_pkt_struct;
bool ready_to_send = false;

//PENSAR CAMBIAR POR QUE RECORRA UN VECTOR?
Packet* parse_packet_send(const string& buf) {
	byte header = buf[0];
	switch (header){
		case HEADER_CG_MOVE:
			return new CG_MovePacket(buf);
			break;
		case HEADER_CG_CHAT:
			return new CG_ChatPacket(buf);
			break;
		case HEADER_CG_TARGET:
			return new CG_TargetPacket(buf);
			break;
		case HEADER_CG_ATTACK:
			return new CG_AttackPacket(buf);
			break;
		default:
			return new Packet();
	}
}

Packet* parse_packet_recv(const string& buf) {
	byte header = buf[0];
	switch (header) {
		case HEADER_GC_MOVE:
			return new GC_MovePacket(buf);
			break;
		default:
			return new Packet();
	}
}


//PACKET
void Packet::print(){
	//cout << "UNKNOWN PACKET" << endl;
}

string Packet::get_buf(){
	return "";
}

packet_struct* Packet::get_packet_struct(packet_struct std){
	string s = this->get_buf();
	packet_struct* new_pkt_struct = new packet_struct(std); //hago una copia del pkt struct
	new_pkt_struct->buf_send_len = s.length();
	new_pkt_struct->buf_send = new char[s.length() + 1]; //creo un nuevo buffer para que no apunte al que usa el cliente
	memcpy(new_pkt_struct->buf_send, s.c_str(), s.length());
	//strcpy_s(new_pkt_struct->buf_send, s.length() + 1, s.c_str());
	return new_pkt_struct;
}

void Packet::send(){
	if (ready_to_send){
		packet_struct* pkt_struct = this->get_packet_struct(std_pkt_struct);
		cout << "Sent packet: " << string_to_hex(string(pkt_struct->buf_send, pkt_struct->buf_send_len)) << endl;
		OriginalMySend(pkt_struct);
		if (pkt_struct->buf_send) delete pkt_struct->buf_send;
		delete pkt_struct;
	} else cout << "Attempted to send a packet, but there is not packet struct yet!" << endl;
}


//CG_MOVEPACKET
CG_MovePacket::CG_MovePacket(byte type, byte subtype, byte direction, int x, int y, int time){
	header = HEADER_CG_MOVE;
	this->type = type;
	this->subtype = subtype;
	this->direction = direction;
	this->x = x;
	this->y = y;
	this->time = time;
	set_type_str();
}

CG_MovePacket::CG_MovePacket(const string& buf){
	header = HEADER_CG_MOVE;
	type = buf[1];
	subtype = buf[2];
	direction = buf[3];
	x = u32(buf.substr(4, 4));
	y = u32(buf.substr(8, 4));
	time = u32(buf.substr(12, 4));
	set_type_str();
}

void CG_MovePacket::set_type_str() {
	switch (type) {
	case 0:
		type_str = "stop_moving";
		break;
	case 1:
		type_str = "start_moving";
		break;
	case 3:
		type_str = "attack";
		break;
	default:
		type_str = "unknown(" + to_string((int)type) + ")";
	}
}

void CG_MovePacket::print(){
	cout << dec << "[SEND] Move packet of type " << type_str << " and subtype " << (int)subtype << ", to " << x << ", " << y << " and direction " << (unsigned int)direction << ". Time: " << time << endl << hex;
}

string CG_MovePacket::get_buf(){
	string buf;
	buf += header;
	buf += type;
	buf += subtype;
	buf += direction;
	buf += p32(x) + p32(y) + p32(OriginalGetTime());
	buf += '\x00';
	return buf;
}

//GC_MOVEPACKET
GC_MovePacket::GC_MovePacket(const string& buf) {
	header = HEADER_GC_MOVE;
	type = buf[1];
	subtype = buf[2];
	direction = buf[3];
	id = u32(buf.substr(4, 4));
	x = u32(buf.substr(8, 4));
	y = u32(buf.substr(12, 4));
	time = u32(buf.substr(16, 4));
	duration = u32(buf.substr(20,4));
	set_type_str();
}

void GC_MovePacket::set_type_str() {
	switch (type) {
	case 0:
		type_str = "stop_moving";
		break;
	case 1:
		type_str = "start_moving";
		break;
	case 3:
		type_str = "attack";
		break;
	default:
		type_str = "unknown(" + to_string((int)type) + ")";
	}
}

void GC_MovePacket::print() {
	//cout << dec << "[RECV] Move packet of id " << id << ", type " << type_str << " and subtype " << (int)subtype << ", to " << x << ", " << y << " and direction " << (unsigned int)direction << ". Time: " << time << ". Duration: " << duration << endl << hex;
}

string GC_MovePacket::get_buf() {
	string buf;
	buf += header;
	buf += type;
	buf += subtype;
	buf += direction;
	buf += p32(id) + p32(x) + p32(y) + p32(OriginalGetTime());
	buf += p32(duration);
	buf += '\x00';
	return buf;
}


//CG_CHATPACKET
CG_ChatPacket::CG_ChatPacket(const string& buf){
	header = HEADER_CG_CHAT;
	length = buf[1]-5;
	msg = string(buf, 4, length);
}

void CG_ChatPacket::print(){
	cout << "[SEND] CHAT PACKET: " << msg << endl;
}

string CG_ChatPacket::get_buf() {
	return "";
}



//CG_TARGETPACKET
CG_TargetPacket::CG_TargetPacket(const string& buf){
	header = HEADER_CG_TARGET;
	id = u32(buf.substr(1,4));
}

void CG_TargetPacket::print(){
	cout << "[SEND] TARGET PACKET, ID: " << id << endl;
}

string CG_TargetPacket::get_buf(){
	string buf;
	buf += header;
	buf += p32(id);
	buf += '\x00';
	return buf;
}


//CG_ATTACKPACKET
CG_AttackPacket::CG_AttackPacket(byte type, int id, byte unk1, byte unk2) {
	header = HEADER_CG_ATTACK;
	this->type = type;
	this->id = id;
	this->unk1 = unk1;
	this->unk2 = unk2;
}

CG_AttackPacket::CG_AttackPacket(const string& buf){
	header = HEADER_CG_ATTACK;
	type = buf[1];
	id = u32(buf.substr(2,4));
	unk1 = buf[6];
	unk2 = buf[7];
}

void CG_AttackPacket::print(){
	cout << "[SEND] Attack packet of type " << (int)type << " to id " << id << ". Unks: " << int(unk1) << ", " << int(unk2) << endl;
}

string CG_AttackPacket::get_buf(){
	string buf;
	buf += header;
	buf += type;
	buf += p32(id);
	buf += unk1;
	buf += unk2;
	buf += '\x00';
	return buf;
}
