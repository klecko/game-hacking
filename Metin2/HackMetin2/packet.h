/**
 * Packet module
 *
 * Metin2 packets are encrypted. In order to avoid encryption, instead of
 * hooking send and recv APIs, we hook other high level functions where packets
 * are not encrypted yet. I called those functions MySend and MyRecv. Their 
 * single parameter is a pointer to a struct I called packet_struct. It has a
 * lot of fields, most important are buf_send and buf_recv, which contains the
 * data that is being sent or received. 
 *
 * The important thing is that, instead of creating this packet_struct from 
 * scratch correctly each time we need to send a fake packet, we copy it the
 * first time an ingame packet is sent, and then we just have to duplicate it
 * and set its send or recv buffer accordingly. This means we can not send 
 * packets until we have this copyable packet_struct, called std_pkt_struct,
 * with the positive side of not having to figure out what is every field of
 * the structure.
 *
 * For the purpose of giving some abstraction, we have a class Packet and a
 * subclass for each different type of packet. 
 * Each subclass must define:
 *   - Its proper attributes, including the byte header as class attribute.
 *   - A constructor given the buffer (buffer-to-packet).
 *	 - The `get_buffer` method that builds the buffer which will be sent to the
 *	   server (packet-to-buffer).
 *	 - The `log` method.
 *
 * Also, those packets we want to build and send from scratch should implement
 * an appropiate constructor. Finally, the `on_hook` method can be also
 * implemented, which acts as an event handlers for when a packet is received
 * or sent.
 *
 * The Packet class implements the `send` and `recv` methods, which are generic
 * to every packet and send the packet to the server or simulates having
 * received it. If you don't provide a packet_struct, it will create one as 
 * explained before.
 *
 * Example of creating and sending an ItemUse packet:
 *		CG_ItemUse p(inventory_pos);
 *		p.send();
 *
 * Example of modifying every sent chat packet:
 *		bool CG_Chat::on_hook(){
 *			this->msg = "CENSORED";
 *			return true;
 *		}
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
	// Min size of the packet. A buff must be at least this long for being this
	// type of packet. Used only for check_size.
	static const uint size = 1;

	// Generic packet can have any header
	byte header = 0;

	// Just save the buf
	std::string buf;

	// Copies the content of the packet into the send_buf of a pkt_struct
	void attach_to_pkt_struct(packet_struct* pkt_struct);

protected:
	// Throws an exception if buf is not at least size bytes long
	static void check_size(const std::string& buf, uint size);

public:
	Packet() {};

	// Builds the packet given the buffer
	Packet(const std::string& buf);

	// Builds the buffer that will be sent to the server
	virtual std::string get_buf();

	// Prints a description of the packet
	virtual void log(std::ostream& out=std::cout);
	
	// Called when the packet is sent or received in the hook function
	// Should return true if the packet has been modified. It will then be sent
	// instead of the original packet
	virtual bool on_hook() { return false; };

	// Real size of the packet. Should be the same as `size` for static packets.
	uint bufsize() { return this->get_buf().size(); };

	// Sends the packet to the server
	int send();
	int send(packet_struct* pkt_struct);

	// Simulates receiving the packet from the server
	int recv();
};


class CG_Move : public Packet{
private:
	static const byte header = HEADER_CG_MOVE;
	static const uint size = 17;
	byte type;
	byte subtype;
	byte direction;
	int x;
	int y;
	uint time;
	std::string type_str;

	void set_type_str();
	
public:
	CG_Move() {};
	CG_Move(byte type, byte subtype, byte direction, int x, int y, uint time);
	CG_Move(const std::string& buf);
	std::string get_buf();
	void log(std::ostream& out);
	bool on_hook();
};


class CG_Chat : public Packet {
private:
	static const byte header = HEADER_CG_CHAT;
	static const uint size = 5; // + msg
	std::string msg;
	byte type;

public:
	CG_Chat() {};
	CG_Chat(const std::string& buf);
	CG_Chat(byte type, const std::string& msg);
	std::string get_buf();
	void log(std::ostream& out);

	const std::string& get_msg() { return msg; }
	void set_msg(const std::string& msg) { this->msg = msg; }
};


class CG_Target : public Packet {
private:
	static const byte header = HEADER_CG_TARGET;
	static const uint size = 6;
	uint id;

public:
	CG_Target() {};
	CG_Target(const std::string& buf);
	std::string get_buf();
	void log(std::ostream& out);
	bool on_hook();

	uint get_id() { return id; }
};


class CG_Attack : public Packet {
private:
	static const byte header = HEADER_CG_ATTACK;
	static const uint size = 9;
	byte type;
	uint id;
	byte b1;
	byte b2;

public:
	CG_Attack() {};
	CG_Attack(byte type, uint id);
	CG_Attack(byte type, uint id, byte b1, byte b2);
	CG_Attack(const std::string& buf);
	std::string get_buf();
	void log(std::ostream& out);
};


class CG_ItemUse : public Packet {
private:
	static const byte header = HEADER_CG_ITEM_USE;
	static const uint size = 5;
	byte item_pos;

public:
	CG_ItemUse() {};
	CG_ItemUse(byte item_pos);
	CG_ItemUse(const std::string& buf);
	std::string get_buf();
	void log(std::ostream& out);
};


class CG_ItemDrop : public Packet {
private:
	static const byte header = HEADER_CG_ITEM_DROP2;
	static const uint size = 10;
	bool is_dropping_item;
	byte item_pos;
	byte item_amount;
	uint yang_amount;

public:
	CG_ItemDrop() {};
	CG_ItemDrop(byte item_pos, byte item_amount);
	CG_ItemDrop(uint yang_amount);
	CG_ItemDrop(const std::string& buf);
	std::string get_buf();
	void log(std::ostream& out);
};


class CG_Whisper : public Packet {
private:
	static const byte header = HEADER_CG_WHISPER;
	static const int username_len = 25;
	static const uint size = 4 + username_len; // + msg
	std::string username;
	std::string msg;

public:
	CG_Whisper() {};
	CG_Whisper(const std::string& username, const std::string& msg);
	CG_Whisper(const std::string& buf);
	std::string get_buf();
	void log(std::ostream& out);
};


class GC_Move : public Packet {
private:
	static const byte header = HEADER_GC_MOVE;
	static const uint size = 24;
	byte type;
	byte subtype;
	byte direction;
	uint id;
	int x;
	int y;
	uint time;
	int duration;
	std::string type_str;

	void set_type_str();

public:
	GC_Move() {};
	GC_Move(const std::string& buf);
	GC_Move(byte type, byte subtype, byte direction, uint id, int x, int y, int duration);
	std::string get_buf();
	void log(std::ostream& out);
	bool on_hook();
};


class GC_CharacterAdd : public Packet {
private:
	static const byte header = HEADER_GC_CHARACTER_ADD;
	static const uint size = 35;
	uint id;
	float direction;
	int x, y, z;
	byte type;
	ushort mob_id;
	byte moving_speed;
	byte attack_speed;

public:
	GC_CharacterAdd() {};
	GC_CharacterAdd(uint id, float direction, int x, int y, int z, byte type,
		ushort mob_id, byte moving_speed, byte attack_speed);
	GC_CharacterAdd(const std::string& buf);
	std::string get_buf();
	void log(std::ostream& out);
	bool on_hook();
};


class GC_CharacterAdditionalInfo : public Packet {
private:
	static const byte header = HEADER_GC_CHAR_ADDITIONAL_INFO;
	static const uint size = 54;

public:
	GC_CharacterAdditionalInfo() {};
	GC_CharacterAdditionalInfo(const std::string& buf) {}; // TODO
	std::string get_buf(); // TODO
};


class GC_CharacterDel : public Packet {
private:
	static const byte header = HEADER_GC_CHARACTER_DEL;
	static const uint size = 5;
	uint id;

public:
	GC_CharacterDel() {};
	GC_CharacterDel(uint id);
	GC_CharacterDel(const std::string& buf);
	std::string get_buf();
	void log(std::ostream& out);
	bool on_hook();
};


class GC_Chat : public Packet {
private:
	static const byte header = HEADER_GC_CHAT;
	static const uint size = 9; // + msg
	std::string msg;
	uint id;
	byte type;

public:
	GC_Chat() {};
	GC_Chat(const std::string& buf);
	GC_Chat(byte type, uint id, const std::string& msg);
	std::string get_buf();
	void log(std::ostream& out);

};


class GC_Whisper : public Packet {
private:
	static const byte header = HEADER_GC_WHISPER;
	static const int username_len = 25;
	static const uint size = 4 + username_len;
	byte type;
	std::string username;
	std::string msg;

public:
	GC_Whisper() {};
	GC_Whisper(const std::string& buf);
	GC_Whisper(byte type, const std::string& username, const std::string& msg);
	std::string get_buf();
	void log(std::ostream& out);
};


class GC_ItemUpdate : public Packet {
private:
	static const byte header = HEADER_GC_ITEM_UPDATE;
	static const uint size = 1; // TODO

public:
	GC_ItemUpdate() {};
	GC_ItemUpdate(const std::string& buf);
	void log(std::ostream& out);
};


class GC_ItemDel : public Packet {
private:
	static const byte header = HEADER_GC_ITEM_DEL;
	static const uint size = 1; // TODO

public:
	GC_ItemDel() {};
	GC_ItemDel(const std::string& buf);
	void log(std::ostream& out);
};


class GC_ItemSet : public Packet {
private:
	static const byte header = HEADER_GC_ITEM_SET;
	static const uint size = 1; // TODO

public:
	GC_ItemSet() {};
	GC_ItemSet(const std::string& buf);
	void log(std::ostream& out);
};


class GC_ItemUse : public Packet {
private:
	static const byte header = HEADER_GC_ITEM_USE;
	static const uint size = 1; // TODO

public:
	GC_ItemUse() {};
	GC_ItemUse(const std::string& buf);
	void log(std::ostream& out);
};


class GC_ItemDrop : public Packet {
private:
	static const byte header = HEADER_GC_ITEM_DROP;
	static const uint size = 1; // TODO

public:
	GC_ItemDrop() {};
	GC_ItemDrop(const std::string& buf);
	void log(std::ostream& out);
};
