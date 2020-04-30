/**
 * Packet module
 *
 * Metin2 packets are encrypted. In order to avoid encryption, instead of
 * hooking send and recv APIs, we hook other high level functions where packets are
 * not encrypted yet. I called those functions MySend and MyRecv. Their single
 * parameter is a pointer to a struct I called packet_struct. It has a lot of
 * fields, most important are buf_send and buf_recv, which contains the
 * data that is being sended or received. 
 *
 * The important thing is that, instead
 * of creating this packet_struct from scratch correctly each time we need to send a fake
 * packet, we copy it the first time an ingame packet is sent, and then we
 * just have to duplicate it and set its send buffer accordingly. This means
 * we can not send packets until we have this packet_struct, called 
 * std_pkt_struct, with the positive side of not having to figure out what is
 * every field of the structure.
 *
 * For the purpose of giving some abstraction, we have a class Packet and a
 * subclass for each different type of packet. 
 * Each subclass must define:
 *   - Its proper attributes appart from the byte header.
 *   - A constructor given the buffer (buffer-to-packet).
 *	 - The `get_buffer` method that builds the buffer which will be sent to the
 *	   server (packet-to-buffer).
 *	 - The `print` method.
 * Also, those packets we want to build and send from scratch should implement an appropiate
 * constructor.
 * The Packet class implements the `send` method, which is generic to every
 * packet and sends the packet to the server. If you don't provide a packet_struct, it will create one as explained before.
 *
 * Example of creating and sending an attack packet:
 *		CG_AttackPacket p(type, target_id, 0x0b, 0xcc);
 *		p.send();
 *
 * Example of modifying every chat packet and sending it inside HookMySend:
 *		int __fastcall HookMySend(packet_struct *_this) {
 *			int ret;
 *			string buf(_this->buf_send, _this->buf_send_len);
 *			Packet* ppacket = parse_packet_send(buf);
 *			if (ppacket->get_header() == HEADER_CG_CHAT){
 *				((CG_ChatPacket*)ppacket)->set_msg("CENSORED");
 *				ret = ppacket->send(_this);
 *			} else {
 *				ret = OriginalMySend(_this);
 *			}
 *			delete ppacket;
 *			return ret;
 *		}
 *
 */

/**
 * [NOTES]
 *  - Last byte of each packet send buf seems to change with every packet,
 *    always in the same order. However, just setting a null byte seems to work
 */

#pragma once

#include <string>
#include "packet_headers.h"
#include "packet_struct.h"

typedef unsigned char byte;

extern packet_struct std_pkt_struct;
extern bool ready_to_send;

class Packet;

// Creates packet from buffer. Returned pointer can be a Packet if the
// type of packet isn't known, or any of its subclasses otherwise.
Packet* parse_packet_send(const std::string& buf);
Packet* parse_packet_recv(const std::string& buf);

class Packet{
private:
	// Generic packet can have any header
	byte header = 0;
	std::string buf;

protected:
	// Builds the buffer that will be sent to the server
	virtual std::string get_buf();

	// Copies the content of the packet into the send_buf of a pkt_struct
	void attach_to_pkt_struct(packet_struct* pkt_struct);

public:
	Packet() {};

	// Builds the packet given the buffer
	Packet(const std::string& buf);

	// Prints a description of the packet
	virtual void log();
	
	// Called when the packet is sent or received in the hook function
	virtual void on_hook() {};

	// Sends the packet to the server
	int send();
	int send(packet_struct* pkt_struct);
};

class CG_MovePacket : public Packet{
private:
	static const byte header = HEADER_CG_MOVE;
	byte type;
	byte subtype;
	byte direction;
	int x;
	int y;
	int time;
	std::string type_str;

	std::string get_buf();
	void set_type_str();
	
public:
	CG_MovePacket() {};
	CG_MovePacket(byte type, byte subtype, byte direction, int x, int y, int time);
	CG_MovePacket(const std::string& buf);
	void log();
};



class GC_MovePacket : public Packet {
private:
	static const byte header = HEADER_GC_MOVE;
	byte type;
	byte subtype;
	byte direction;
	int id;
	int x;
	int y;
	int time;
	int duration;
	std::string type_str;

	std::string get_buf();
	void set_type_str();

public:
	GC_MovePacket() {};
	GC_MovePacket(const std::string& buf);
	void log();
};



class CG_ChatPacket : public Packet {
private:
	static const byte header = HEADER_CG_CHAT;
	std::string msg;
	byte type;

	std::string get_buf();

public:
	CG_ChatPacket() {};
	CG_ChatPacket(const std::string& buf);
	CG_ChatPacket(byte type, const std::string& msg);
	
	const std::string& get_msg() { return msg; }
	void set_msg(const std::string& msg) { this->msg = msg; }

	void log();
};



class CG_TargetPacket : public Packet {
private:
	static const byte header = HEADER_CG_TARGET;
	int id;

	std::string get_buf();

public:
	CG_TargetPacket() {};
	CG_TargetPacket(const std::string& buf);
	int get_id() { return id; }
	void on_hook();
	void log();
};


class CG_AttackPacket : public Packet {
private:
	static const byte header = HEADER_CG_ATTACK;
	byte type;
	int id;
	byte unk1;
	byte unk2;

	std::string get_buf();

public:
	CG_AttackPacket() {};
	CG_AttackPacket(byte type, int id, byte unk1, byte unk2);
	CG_AttackPacket(const std::string& buf);
	void log();
};

class CG_ItemUse : public Packet {
private:
	static const byte header = HEADER_CG_ITEM_USE;
	byte item_pos;

	std::string get_buf();

public:
	CG_ItemUse() {};
	CG_ItemUse(byte item_pos);
	CG_ItemUse(const std::string& buf);
	void print();
};

class CG_ItemDrop : public Packet {
private:
	static const byte header = HEADER_CG_ITEM_DROP2;
	bool is_dropping_item;
	byte item_pos;
	byte item_amount;
	int yang_amount;

	std::string get_buf();

public:
	CG_ItemDrop() {};
	CG_ItemDrop(byte item_pos, byte item_amount);
	CG_ItemDrop(int yang_amount);
	CG_ItemDrop(const std::string& buf);
	void print();
};