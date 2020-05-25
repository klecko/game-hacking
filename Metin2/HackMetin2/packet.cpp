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
		case HEADER_CG_WHISPER:
			return new CG_Whisper(buf);
		default:
			return new Packet(buf);
	}
}

Packet* parse_packet_recv(const string& buf) {
	byte header = buf[0];
	switch (header) {
		case HEADER_GC_CHARACTER_ADD:
			return new GC_CharacterAdd(buf);
		case HEADER_GC_CHAR_ADDITIONAL_INFO:
			return new GC_CharacterAdditionalInfo(buf);
		case HEADER_GC_CHARACTER_DEL:
			return new GC_CharacterDel(buf);
		case HEADER_GC_MOVE:
			return new GC_Move(buf);
		case HEADER_GC_CHAT:
			return new GC_Chat(buf);
		case HEADER_GC_WHISPER:
			return new GC_Whisper(buf);
		case HEADER_GC_ITEM_UPDATE:
			return new GC_ItemUpdate(buf);
		case HEADER_GC_ITEM_DEL:
			return new GC_ItemDel(buf);
		case HEADER_GC_ITEM_SET:
			return new GC_ItemSet(buf);
		case HEADER_GC_ITEM_USE:
			return new GC_ItemUse(buf);
		case HEADER_GC_ITEM_DROP:
			return new GC_ItemDrop(buf);
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
	// length is offset where we are copying to plus length of what we are copying
	pkt_struct->buf_send_len = pkt_struct->buf_send_offset + s.length();
	memcpy(pkt_struct->buf_send + pkt_struct->buf_send_offset, s.c_str(), s.length());
	pkt_struct->buf_send[s.length()] = '\x00';
}

void Packet::check_size(const string& buf, uint size){
	if (buf.size() < size){
		string msg = "Bad packet size (should be " + to_string(size) + ", but it's " + to_string(buf.size()) + "). Buf: " + string_to_hex(buf);
		throw exception(msg.c_str());
	}
}

// Anteriormente los dos métodos send creaban un nuevo pkt_struct (uno a partir del std,
// otro a partir del argumento). Eso hacía que no se pudiera interceptar un paquete, ya que
// no modificabamos el pkt_struct original, se interpretaba como que el send habia fallado,
// y se volvía a intentar todo el rato.
int Packet::send(packet_struct* pkt_struct){
	// Use the given pkt_struct. Just attach to it and call OriginalMySend
	this->attach_to_pkt_struct(pkt_struct);
	cout << "Sent packet: " << endl;// << string_to_hex(string(pkt_struct->buf_send, pkt_struct->buf_send_len)) << endl;
	//this->log();
	return OriginalMySend(pkt_struct);
}

int Packet::send(){
	// Create a new pkt_struct from std_pkt_struct and a new send_buffer, and send.
	int ret = 0;
	if (ready_to_send) {
		packet_struct* new_pkt_struct = new packet_struct(std_pkt_struct); //hago una copia del pkt struct
		new_pkt_struct->buf_send = new char[this->get_buf().length() + 1]; //creo un nuevo buffer para que no apunte al que usa el cliente
		new_pkt_struct->buf_send_offset = 0;
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
	check_size(buf, this->size);
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

bool CG_Move::on_hook(){
	/*if (this->type == 3){
		this->subtype = 0x11;
		return true;
	}*/
	return false;
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
8: Mensaje grande de anuncio
*/
CG_Chat::CG_Chat(byte type, const string& msg){
	this->type = type;
	this->msg = msg;
}

CG_Chat::CG_Chat(const string& buf){
	check_size(buf, this->size);
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
	check_size(buf, this->size);
	this->id = u32(buf.substr(1,4));
}

void CG_Target::log(){
	cout << "[SEND] TARGET PACKET, ID: " << this->id << endl;
}

bool CG_Target::on_hook(){
	Command::set_target(this->id);
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
	check_size(buf, this->size);
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
	check_size(buf, this->size);
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
	check_size(buf, this->size);
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

// [ CG_Whisper ]
CG_Whisper::CG_Whisper(const string& username, const string& msg){
	this->username = username;
	this->msg = msg;
}

CG_Whisper::CG_Whisper(const string& buf){
	// 13 2B00 4461726B536173696E00000000000000000000000000000005 414142424343444445454646474700 A7
	check_size(buf, this->size);
	ushort packet_len = u16(buf.substr(1, 2));
	this->username = buf.substr(3, this->username_len);
	this->username = this->username.substr(0, this->username.find('\x00')); // remove nullbytes
	this->msg = buf.substr(3+this->username_len, buf.size()-(3+this->username_len)-2);
	// -2 bc of last byte and msg nullbyte
}

string CG_Whisper::get_buf(){
	// packet_len does not include last byte
	ushort packet_len = 3 + this->username_len + 1 + this->msg.size() + 1;
	//packet_len++;
	string buf;
	buf += this->header;
	buf += p16(packet_len);
	buf += this->username + string(this->username_len - this->username.size(), '\x00');
	buf += '\x05'; // ??
	buf += this->msg + '\x00';
	buf += '\x00';
	return buf;
}

void CG_Whisper::log(){
	cout << "[SEND] Whispering " << this->username << ": " << this->msg << endl;
}

// [ GC_Move ]
GC_Move::GC_Move(const string& buf) {
	check_size(buf, this->size);
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

GC_Move::GC_Move(byte type, byte subtype, byte direction, uint id, int x, int y, int duration){
	this->type = type;
	this->subtype = subtype;
	this->direction = direction;
	this->id = id;
	this->x = x;
	this->y = y;
	this->time = OriginalGetTime();
	this->duration = duration;
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
	return buf;
}

// [ GC_CharacterAdd ]
/*
01 FFD20800 8832AF43 12910E00 0FC10300 00000000 00 6500 96 64 00 00000000 00000000
01 FCD20800 E4A1A343 F9900E00 61BE0300 00000000 00 6500 96 64 00 00000000 00000000
01 7C790300 ED031142 B9940E00 A9DA0300 00000000 06 0200 64 64 00 00000000 00000000
0  1        5        9        13       17       21 22   24 25
   id      direction x        y        z      type mob
*/

GC_CharacterAdd::GC_CharacterAdd(const string& buf){
	check_size(buf, this->size);
	this->id = u32(buf.substr(1, 4));
	this->direction = 0; // buf.substr(5,4) as float
	this->x = u32(buf.substr(9, 4));
	this->y = u32(buf.substr(13, 4));
	this->z = u32(buf.substr(17, 4));
	this->type = buf[21];
	this->mob_id = u16(buf.substr(22, 2));
	this->moving_speed = buf[24];
	this->attack_speed = buf[25];
}

string GC_CharacterAdd::get_buf(){
	string buf;
	buf += this->header;
	buf += p32(this->id);
	buf += p32(0); // direction as float
	buf += p32(this->x) + p32(this->y) + p32(this->z);
	buf += this->type;
	buf += p16(this->mob_id);
	buf += this->moving_speed;
	buf += this->attack_speed;
	buf += '\x00'; // ?
	buf += p32(0); // ?
	buf += p32(0); // ?
	return buf;
}

void GC_CharacterAdd::log(){
	cout << "[RECV] Character add, id " << this->id << " at (" << this->x << ", " << this->y << ", " << this->z << ") of type "
		<< (int)this->type << " and mob id " << this->mob_id << endl;
}

bool GC_CharacterAdd::on_hook(){
	Command::update_enemy(this->id, this->x, this->y);
	return false;
}

// [ GC_CharacterAdditionalInfo ]
// TODO
std::string GC_CharacterAdditionalInfo::get_buf() { return string(this->size, 'A'); }

// [ GC_CharacterDel ]
GC_CharacterDel::GC_CharacterDel(uint id){
	this->id = id;
}

GC_CharacterDel::GC_CharacterDel(const string& buf){
	check_size(buf, this->size);
	this->id = u32(buf.substr(1, 4));
}

string GC_CharacterDel::get_buf(){
	string buf;
	buf += this->header;
	buf += p32(this->id);
	return buf;
}

void GC_CharacterDel::log(){
	cout << "[RECV] Character del: " << this->id << endl;
}

bool GC_CharacterDel::on_hook(){
	Command::delete_enemy(this->id);
	return false;
}

// [ GC_Chat ]
/*
04 3700 060000000003 7C6346466666303030307C487C685B4C696F6E5D7C6346464137464644347C487C682045626F6F6B73203A203332
04 5900 060000000003 7C6346463030383046467C487C685B4C696F6E5D7C6346464137464644347C487C6820536F756C74616B6572203A2056454E444F20454D504520475545525245524F2059204E494E4A41204D504D5050
04 2300 050000000003 5365745465616D4F66666C696E65205B474D5D706572693230300421000500000000035365745465616D4F66666C696E65205B544D5D766163696F0421000500000000035365745465616D4F66666C696E65205B474D5D766163696F0421000500000000035365745465616D4F66666C696E65205B474D5D766163696F0421000500000000035365745465616D4F66666C696E65205B544D5D766163696F
04 0101 060101010103 6161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161
*/
GC_Chat::GC_Chat(const string& buf) {
	check_size(buf, this->size);
	ushort packet_len = u16(buf.substr(1, 2));
	this->type = buf[3];
	this->id = u32(buf.substr(4, 4));
	this->msg = buf.substr(9, packet_len-1-2-6);
}

GC_Chat::GC_Chat(byte type, uint id, const string& msg){
	this->type = type;
	this->id = id;
	this->msg = msg;
}

string GC_Chat::get_buf(){
	ushort packet_len = this->msg.length() + 9;
	string buf;
	buf += this->header;
	buf += p16(packet_len);
	buf += this->type;
	buf += p32(this->id);
	buf += '\x03'; // KINGDOM?
	buf += this->msg;
	return buf;
}

void GC_Chat::log() {
	cout << "[RECV] Chat packet of type " << (int)this->type << ": " << this->msg << endl;
}

// [ GC_Whisper ]
/*
22 2100 00 4B6C65636B6100000000000000000000000000000000000000 41414141
22 3000 00 4B6C65636B6100000000000000000000000000000000000000 746F6E746F20656C20717565206C6F206C6561
22 0101 05 61616161616161616161616161616161616161616161616161 61616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161
*/
GC_Whisper::GC_Whisper(const string& buf){
	check_size(buf, this->size);
	ushort packet_len = u16(buf.substr(1, 2));
	this->type = buf[3];
	this->username = buf.substr(4, this->username_len);
	this->username = this->username.substr(0, this->username.find('\x00')); // remove nullbytes
	this->msg = buf.substr(3 + 1 + this->username_len, buf.size() - (3 + 1 + this->username_len));
	cout << "[RECV] WHISPER: " << this->username << ": " << this->msg << endl;
}

GC_Whisper::GC_Whisper(byte type, const std::string& username, const std::string& msg){
	this->type = type;
	this->username = username;
	this->msg = msg;
}

string GC_Whisper::get_buf(){
	ushort packet_len = 4 + this->username_len  + this->msg.length();
	string buf;
	buf += this->header;
	buf += p16(packet_len);
	buf += this->type;
	buf += this->username + string(this->username_len - this->username.size(), '\x00');
	buf += this->msg;
	return buf;
}

void GC_Whisper::log(){

}

// [ GC_ItemUpdate ]
GC_ItemUpdate::GC_ItemUpdate(const string& buf)
	: Packet(buf) {

}

void GC_ItemUpdate::log() {
	string hex_buf = string_to_hex(this->get_buf());
	cout << "[RECV] GC_ItemUpdate: " << hex_buf << endl;
}

// [ GC_ItemDel ]
// 14 01 1400 0000000000000000000000000000000000000000000000000000000000000000000000000000
GC_ItemDel::GC_ItemDel(const string& buf)
	: Packet(buf) {
	check_size(buf, this->size);
}

void GC_ItemDel::log() {
	string hex_buf = string_to_hex(this->get_buf());
	cout << "[RECV] GC_ItemDel: " << hex_buf << endl;
}

// [ GC_ItemSet ]
GC_ItemSet::GC_ItemSet(const string& buf)
	: Packet(buf) {
	check_size(buf, this->size);
}

void GC_ItemSet::log() {
	string hex_buf = string_to_hex(this->get_buf());
	cout << "[RECV] GC_ItemSet: " << hex_buf << endl;
}

// [ GC_ItemUse ]
GC_ItemUse::GC_ItemUse(const string& buf)
	: Packet(buf) {
	check_size(buf, this->size);
}

void GC_ItemUse::log() {
	string hex_buf = string_to_hex(this->get_buf());
	cout << "[RECV] GC_ItemUse: " << hex_buf << endl;
}

// [ GC_ItemDrop ]
GC_ItemDrop::GC_ItemDrop(const string& buf)
	: Packet(buf) {
	check_size(buf, this->size);
}

void GC_ItemDrop::log() {
	string hex_buf = string_to_hex(this->get_buf());
	cout << "[RECV] GC_ItemDrop: " << hex_buf << endl;
}
