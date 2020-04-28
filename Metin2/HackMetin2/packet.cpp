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

Packet* parse_packet_send(const string& buf) {
	byte header = buf[0];
	switch (header){
		case HEADER_CG_MOVE:
			return new CG_MovePacket(buf);
		case HEADER_CG_CHAT:
			return new CG_ChatPacket(buf);
		case HEADER_CG_TARGET:
			return new CG_TargetPacket(buf);
		case HEADER_CG_ATTACK:
			return new CG_AttackPacket(buf);
		case HEADER_CG_ITEM_USE:
			return new CG_ItemUse(buf);
		case HEADER_CG_ITEM_DROP2:
			return new CG_ItemDrop(buf);
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
Packet::Packet(const string& buf){
	if (!buf.empty())
		header = buf[0];
	this->buf = buf;
}
void Packet::print(){
	//cout << "UNKNOWN PACKET" << endl;
}

string Packet::get_buf(){
	return buf;
}



void Packet::attach_to_pkt_struct(packet_struct* pkt_struct){
	string s = this->get_buf();
	pkt_struct->buf_send_len = s.length();
	memcpy(pkt_struct->buf_send, s.c_str(), s.length());
	pkt_struct->buf_send[s.length()] = '\x00';
}

// Anteriormente los dos métodos send creaban un nuevo pkt_struct (uno a partir del std,
// otro a partir del argumento). Eso hacía que no se pudiera interceptar un paquete, ya que
// no modificabamos el pkt_struct original, se interpretaba como que el send habia fallado,
// y se volvía a intentar todo el rato.
int Packet::send(packet_struct* pkt_struct){
	// Use the given pkt_struct. Just attach to it and call OriginalMySend
	attach_to_pkt_struct(pkt_struct);
	cout << "Sent packet: " << string_to_hex(string(pkt_struct->buf_send, pkt_struct->buf_send_len)) << endl;
	this->print();
	return OriginalMySend(pkt_struct);
}

int Packet::send(){
	// Create a new pkt_struct from std_pkt_struct and a new send_buffer and send.
	int ret = 0;
	if (ready_to_send) {
		packet_struct* new_pkt_struct = new packet_struct(std_pkt_struct); //hago una copia del pkt struct
		new_pkt_struct->buf_send = new char[this->get_buf().length() + 1]; //creo un nuevo buffer para que no apunte al que usa el cliente
		send(new_pkt_struct);
		delete new_pkt_struct->buf_send;
		delete new_pkt_struct;
	}
	else cout << "Attempted to send a packet, but there is not packet struct yet!" << endl;
	return ret;
}


//CG_MOVEPACKET
CG_MovePacket::CG_MovePacket(byte type, byte subtype, byte direction, int x, int y, int time){
	this->type = type;
	this->subtype = subtype;
	this->direction = direction;
	this->x = x;
	this->y = y;
	this->time = time;
	set_type_str();
}

CG_MovePacket::CG_MovePacket(const string& buf){
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
	cout << "[SEND] Move packet of type " << type_str << " and subtype " << (int)subtype << ", to (" << x << ", " << y << ") and direction " << (unsigned int)direction << ". Time: " << time << endl;
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
	//cout << "[RECV] Move packet of id " << id << ", type " << type_str << " and subtype " << (int)subtype << ", to " << x << ", " << y << " and direction " << (unsigned int)direction << ". Time: " << time << ". Duration: " << duration << endlx;
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
/* [TYPES]
0: Normal
1:
2: TipMessage
3: Group?
4: Guild
5: Commands
6: Llamar
*/
CG_ChatPacket::CG_ChatPacket(byte type, const string& msg){
	this->type = type;
	this->msg = msg;
}

CG_ChatPacket::CG_ChatPacket(const string& buf){
	int length = u16(buf.substr(1,2)) - 5;
	type = buf[3];
	msg = string(buf, 4, length);
}

void CG_ChatPacket::print(){
	cout << "[SEND] CHAT PACKET OF TYPE " << (int)type << ": " << msg << endl;
}

string CG_ChatPacket::get_buf() {
	string buf;
	buf += header;
	buf += p16(msg.length()+5);
	buf += type;
	buf += msg + '\x00'; // null cstring
	buf += '\x00';
	return buf;
}



//CG_TARGETPACKET
CG_TargetPacket::CG_TargetPacket(const string& buf){
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
	this->type = type;
	this->id = id;
	this->unk1 = unk1;
	this->unk2 = unk2;
}

CG_AttackPacket::CG_AttackPacket(const string& buf){
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

// CG_ItemUse
CG_ItemUse::CG_ItemUse(byte item_pos){
	this->item_pos = item_pos;
}

CG_ItemUse::CG_ItemUse(const std::string& buf){
	item_pos = buf[2];
}

string CG_ItemUse::get_buf(){
	string buf;
	buf += header;
	buf += '\x01'; // seems to be always 1?
	buf += item_pos;
	buf += '\x00'; // padding?
	buf += '\x00';
	return buf;
}

void CG_ItemUse::print(){
	cout << "[SEND] Using item " << (int)item_pos << endl;
}

// CG_ItemDrop
CG_ItemDrop::CG_ItemDrop(byte item_pos, byte item_amount){
	is_dropping_item = true;
	this->item_pos = item_pos;
	this->item_amount = item_amount;
	yang_amount = 0;
}

CG_ItemDrop::CG_ItemDrop(int yang_amount){
	is_dropping_item = false;
	item_pos = 0;
	item_amount = 0;
	this->yang_amount = yang_amount;
}

CG_ItemDrop::CG_ItemDrop(const string& buf){
	is_dropping_item = (bool)buf[1];
	item_pos = buf[2];
	yang_amount = u32(buf.substr(4, 4));
	item_amount = buf[8];
	if (buf[3] != 0)
		cout << "CG_ItemDrop strange packet: " << string_to_hex(buf) << endl;
	if (!is_dropping_item && (item_pos != 0 || item_pos != 0))
		cout << "CG_ItemDrop strange packet2: " << string_to_hex(buf) << endl;
}

string CG_ItemDrop::get_buf(){
	string buf;
	buf += header;
	buf += (byte)is_dropping_item;
	buf += item_pos;
	buf += '\x00'; // ?
	buf += p32(yang_amount);
	buf += item_amount;
	buf += '\x00';
	return buf;
}

void CG_ItemDrop::print(){
	if (is_dropping_item)
		cout << "[SEND] Dropping item " << (int)item_pos << ", amount " << (int)item_amount << endl;
	else
		cout << "[SEND] Dropping yang, amount " << yang_amount << endl;
}