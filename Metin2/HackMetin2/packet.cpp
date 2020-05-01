#include <iostream>
#include <string>
#include <assert.h>

#include "packet.h"
#include "utils.h"
#include "hooks.h"
#include "cmd.h"

using namespace std;

typedef unsigned char byte;

packet_struct std_pkt_struct;
bool ready_to_send = false;

Packet* parse_packet_send(const string& buf) {
	byte header = buf[0];
	switch (header){
		case HEADER_CG_MOVE:
			return new CG_Move(buf);
		case HEADER_CG_CHAT:
			return new CG_Chat(buf);
		case HEADER_CG_TARGET:
			return new CG_Target(buf);
		case HEADER_CG_ATTACK:
			return new CG_Attack(buf);
		case HEADER_CG_ITEM_USE:
			return new CG_ItemUse(buf);
		case HEADER_CG_ITEM_DROP2:
			return new CG_ItemDrop(buf);
		default:
			return new Packet(buf);
	}
}

Packet* parse_packet_recv(const string& buf) {
	byte header = buf[0];
	switch (header) {
		case HEADER_GC_CHARACTER_ADD:
			return new GC_CharacterAdd(buf);
			break;
		case HEADER_GC_CHARACTER_DEL:
			return new GC_CharacterDel(buf);
			break;
		case HEADER_GC_MOVE:
			return new GC_Move(buf);
			break;
		case HEADER_GC_CHAT:
			return new GC_Chat(buf);
			break;
		default:
			return new Packet(buf);
	}
}


// [ Packet ]
Packet::Packet(const string& buf){
	if (!buf.empty())
		this->header = buf[0];
	this->buf = buf;
}

void Packet::log(){
	// Print unknown packets
	string hex_buf = string_to_hex(this->get_buf());
	cout << "UKNOWN PACKET:" << hex_buf << endl;
}

string Packet::get_buf(){
	return this->buf;
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
	this->attach_to_pkt_struct(pkt_struct);
	cout << "Sent packet: " << string_to_hex(string(pkt_struct->buf_send, pkt_struct->buf_send_len)) << endl;
	this->log();
	return OriginalMySend(pkt_struct);
}

int Packet::send(){
	// Create a new pkt_struct from std_pkt_struct and a new send_buffer, and send.
	int ret = 0;
	if (ready_to_send) {
		packet_struct* new_pkt_struct = new packet_struct(std_pkt_struct); //hago una copia del pkt struct
		new_pkt_struct->buf_send = new char[this->get_buf().length() + 1]; //creo un nuevo buffer para que no apunte al que usa el cliente
		ret = this->send(new_pkt_struct);
		delete new_pkt_struct->buf_send;
		delete new_pkt_struct;
	}
	else cout << "Attempted to send a packet, but there is not packet struct yet!" << endl;
	return ret;
}


// [ CG_Move ]
CG_Move::CG_Move(byte type, byte subtype, byte direction, int x, int y, uint time){
	this->type = type;
	this->subtype = subtype;
	this->direction = direction;
	this->x = x;
	this->y = y;
	this->time = time;
	this->set_type_str();
}

CG_Move::CG_Move(const string& buf){
	this->type = buf[1];
	this->subtype = buf[2];
	this->direction = buf[3];
	this->x = u32(buf.substr(4, 4));
	this->y = u32(buf.substr(8, 4));
	this->time = u32(buf.substr(12, 4));
	this->set_type_str();
}

void CG_Move::set_type_str() {
	switch (this->type) {
	case 0:
		this->type_str = "stop_moving";
		break;
	case 1:
		this->type_str = "start_moving";
		break;
	case 3:
		this->type_str = "attack";
		break;
	default:
		this->type_str = "unknown(" + to_string((int)this->type) + ")";
	}
}

void CG_Move::log(){
	cout << "[SEND] Move packet of type " << this->type_str << " and subtype " << (int)this->subtype << ", to (" << this->x << ", " << this->y << ") and direction " << (unsigned int)this->direction << ". Time: " << this->time << endl;
}

string CG_Move::get_buf(){
	string buf;
	buf += this->header;
	buf += this->type;
	buf += this->subtype;
	buf += this->direction;
	buf += p32(this->x) + p32(this->y) + p32(OriginalGetTime());
	buf += '\x00';
	return buf;
}


// [ CG_Chat ]
/* [TYPES]
0: Normal
1:
2: TipMessage
3: Group?
4: Guild
5: Commands
6: Llamar
*/
CG_Chat::CG_Chat(byte type, const string& msg){
	this->type = type;
	this->msg = msg;
}

CG_Chat::CG_Chat(const string& buf){
	uint length = u16(buf.substr(1,2)) - 5;
	this->type = buf[3];
	this->msg = string(buf, 4, length);
}

void CG_Chat::log(){
	cout << "[SEND] CHAT PACKET OF TYPE " << (int)this->type << ": " << this->msg << endl;
}

string CG_Chat::get_buf() {
	string buf;
	buf += this->header;
	buf += p16(this->msg.length()+5);
	buf += this->type;
	buf += this->msg + '\x00'; // null cstring
	buf += '\x00';
	return buf;
}



// [ CG_Target ]
CG_Target::CG_Target(const string& buf){
	this->id = u32(buf.substr(1,4));
}

void CG_Target::log(){
	cout << "[SEND] TARGET PACKET, ID: " << this->id << endl;
}

bool CG_Target::on_hook(){
	Command::set_id_attack(this->id);
	return false;
}

string CG_Target::get_buf(){
	string buf;
	buf += this->header;
	buf += p32(this->id);
	buf += '\x00';
	return buf;
}


// [ CG_ATTACKPACKET ]
CG_Attack::CG_Attack(byte type, uint id){
	this->type = type;
	this->id = id;
	this->b1 = OriginalGetAttackByte();
	this->b2 = OriginalGetAttackByte();
}

CG_Attack::CG_Attack(byte type, uint id, byte b1, byte b2) {
	this->type = type;
	this->id = id;
	this->b1 = b1;
	this->b2 = b2;
}

CG_Attack::CG_Attack(const string& buf){
	this->type = buf[1];
	this->id = u32(buf.substr(2,4));
	this->b1 = buf[6];
	this->b2 = buf[7];
}

void CG_Attack::log(){
	cout << "[SEND] Attack packet of type " << (int)this->type << " to id " << this->id << ". Bytes: " << int(this->b1) << ", " << int(this->b2) << endl;
}

string CG_Attack::get_buf(){
	string buf;
	buf += this->header;
	buf += this->type;
	buf += p32(this->id);
	buf += this->b1;
	buf += this->b2;
	buf += '\x00';
	return buf;
}

// [ CG_ItemUse ]
CG_ItemUse::CG_ItemUse(byte item_pos){
	this->item_pos = item_pos;
}

CG_ItemUse::CG_ItemUse(const std::string& buf){
	this->item_pos = buf[2];
}

string CG_ItemUse::get_buf(){
	string buf;
	buf += this->header;
	buf += '\x01'; // seems to be always 1?
	buf += this->item_pos;
	buf += '\x00'; // padding?
	buf += '\x00';
	return buf;
}

void CG_ItemUse::log(){
	cout << "[SEND] Using item " << (int)this->item_pos << endl;
}

// [ CG_ItemDrop ]
CG_ItemDrop::CG_ItemDrop(byte item_pos, byte item_amount){
	this->is_dropping_item = true;
	this->item_pos = item_pos;
	this->item_amount = item_amount;
	this->yang_amount = 0;
}

CG_ItemDrop::CG_ItemDrop(uint yang_amount){
	this->is_dropping_item = false;
	this->item_pos = 0;
	this->item_amount = 0;
	this->yang_amount = yang_amount;
}

CG_ItemDrop::CG_ItemDrop(const string& buf){
	this->is_dropping_item = (bool)buf[1];
	this->item_pos = buf[2];
	this->yang_amount = u32(buf.substr(4, 4));
	this->item_amount = buf[8];
	if (buf[3] != 0)
		cout << "CG_ItemDrop strange packet: " << string_to_hex(buf) << endl;
	if (!this->is_dropping_item && (this->item_pos != 0 || this->item_pos != 0))
		cout << "CG_ItemDrop strange packet2: " << string_to_hex(buf) << endl;
}

string CG_ItemDrop::get_buf(){
	string buf;
	buf += this->header;
	buf += (byte)this->is_dropping_item;
	buf += this->item_pos;
	buf += '\x00'; // ?
	buf += p32(this->yang_amount);
	buf += this->item_amount;
	buf += '\x00';
	return buf;
}

void CG_ItemDrop::log(){
	if (this->is_dropping_item)
		cout << "[SEND] Dropping item " << (int)this->item_pos << ", amount " << (int)this->item_amount << endl;
	else
		cout << "[SEND] Dropping yang, amount " << this->yang_amount << endl;
}



// [ GC_Move ]
GC_Move::GC_Move(const string& buf) {
	this->type = buf[1];
	this->subtype = buf[2];
	this->direction = buf[3];
	this->id = u32(buf.substr(4, 4));
	this->x = u32(buf.substr(8, 4));
	this->y = u32(buf.substr(12, 4));
	this->time = u32(buf.substr(16, 4));
	this->duration = u32(buf.substr(20, 4));
	this->set_type_str();
}

void GC_Move::set_type_str() {
	switch (this->type) {
	case 0:
		this->type_str = "stop_moving";
		break;
	case 1:
		this->type_str = "start_moving";
		break;
	case 3:
		this->type_str = "attack";
		break;
	default:
		this->type_str = "unknown(" + to_string((int)this->type) + ")";
	}
}

bool GC_Move::on_hook() {
	Command::update_enemy(this->id, this->x, this->y);
	return false;
}

void GC_Move::log() {
	cout << "[RECV] Move packet of id " << id << ", type " << type_str << " and subtype " << (int)subtype << ", to " << x << ", " << y << " and direction " << (unsigned int)direction << ". Time: " << time << ". Duration: " << duration << endl;
}

string GC_Move::get_buf() {
	string buf;
	buf += this->header;
	buf += this->type;
	buf += this->subtype;
	buf += this->direction;
	buf += p32(this->id) + p32(this->x) + p32(this->y) + p32(OriginalGetTime());
	buf += p32(this->duration);
	buf += '\x00';
	return buf;
}

// [ GC_CharacterAdd ]
/*
01F0B50500B1DED842A3810E0074C7030000000000006500966400000000000000000001F1B50500ABCC1443E2810E00F5C50300000000000065009664000000000000000000015DFE0500F3E4A443FE840E0091BE0300000000000065009664000000000000000000029AB605000280FC050002C8B60500029BB605000278FC050002A1B60500027FFC050002A5B605000288FC05000234D605000290340000029EB6050002A0B6050002A4B6050002A2B605000281FC0500028F34000002ECD20500
01C8B60500027EC642387F0E0044CB03000000000000AC00646400000000000000000001EEB50500A7CCBA429D800E0058C6030000000000006500966400000000000000000001EAD20500214D88431E860E0072BA03000000000000650096640000000000000000000160FE0500C2961540DE820E002EC003000000000000650096640000000000000000000161FE050008239C435F840E0093BD030000000000006500966400000000000000000001ECB50500FAFEAE43F07F0E0015C8030000000000006500966400000000000000000001C8AF0500E545084377820E0044D603000000000000AB00646400000000000000000001CAAF050001006143AC810E00C7D403000000000000AB00646400000000000000000002A3B605000232D6050002A7B605000235D605000233D60500
*/
GC_CharacterAdd::GC_CharacterAdd(const string& buf)
: Packet(buf){
	
}

void GC_CharacterAdd::log(){
	string hex_buf = string_to_hex(this->get_buf());
	cout << "[RECV] Character add packet: " << hex_buf << endl;
}

// [ GC_CharacterDel ]
GC_CharacterDel::GC_CharacterDel(const string& buf)
: Packet(buf) {

}

void GC_CharacterDel::log() {
	string hex_buf = string_to_hex(this->get_buf());
	cout << "[RECV] Character del packet: " << hex_buf << endl;
}

// [ GC_Chat ]
/*
04 3700060000000003 7C6346466666303030307C487C685B4C696F6E5D7C6346464137464644347C487C682045626F6F6B73203A203332
04 5900060000000003 7C6346463030383046467C487C685B4C696F6E5D7C6346464137464644347C487C6820536F756C74616B6572203A2056454E444F20454D504520475545525245524F2059204E494E4A41204D504D5050
04 2300050000000003 5365745465616D4F66666C696E65205B474D5D706572693230300421000500000000035365745465616D4F66666C696E65205B544D5D766163696F0421000500000000035365745465616D4F66666C696E65205B474D5D766163696F0421000500000000035365745465616D4F66666C696E65205B474D5D766163696F0421000500000000035365745465616D4F66666C696E65205B544D5D766163696F
*/
GC_Chat::GC_Chat(const string& buf)
	: Packet(buf) {

}

void GC_Chat::log() {
	string hex_buf = string_to_hex(this->get_buf());
	cout << "[RECV] Chat packet: " << hex_buf << endl;
}