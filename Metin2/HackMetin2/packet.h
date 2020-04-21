#pragma once

#include <string>
#include "packet_headers.h"
#include "packet_struct.h"

typedef unsigned char byte;

extern packet_struct std_pkt_struct;
extern bool ready_to_send;

class Packet{
protected:
	byte header=0;

public:
	Packet() {};
	Packet(const std::string& buf) {};
	const byte get_header() const { return header; };

	virtual void print();
	virtual std::string get_buf();
	packet_struct* get_packet_struct(packet_struct std); //le he quitado el virtual
	void send();
};

class CG_MovePacket : public Packet{
private:
	byte type;
	byte subtype;
	byte direction;
	int x;
	int y;
	int time;

	std::string type_str;
	void set_type_str();
	
public:
	CG_MovePacket() {};
	CG_MovePacket(byte type, byte subtype, byte direction, int x, int y, int time);
	CG_MovePacket(const std::string& buf);

	void print();
	std::string get_buf();
};



class GC_MovePacket : public Packet {
private:
	byte type;
	byte subtype;
	byte direction;
	int id;
	int x;
	int y;
	int time;
	int duration;

	std::string type_str;
	void set_type_str();

public:
	GC_MovePacket() {};
	//GC_MovePacket(byte type, byte subtype, byte direction, int id, int x, int y, int time, int duration);
	GC_MovePacket(const std::string& buf);

	void print();
	std::string get_buf();
};



class CG_ChatPacket : public Packet {
private:
	int length;
	std::string msg;

public:
	CG_ChatPacket() {};
	CG_ChatPacket(const std::string& buf);

	void print();
	std::string get_buf();
};



class CG_TargetPacket : public Packet {
private:
	int id;

public:
	CG_TargetPacket() {};
	CG_TargetPacket(const std::string& buf);

	int get_id() { return id; }
	void print();
	std::string get_buf();
};


class CG_AttackPacket : public Packet {
private:
	byte type;
	int id;
	byte unk1;
	byte unk2;

public:
	CG_AttackPacket() {};
	CG_AttackPacket(byte type, int id, byte unk1, byte unk2);
	CG_AttackPacket(const std::string& buf);

	void print();
	std::string get_buf();
};


Packet* parse_packet_send(const std::string& buf);
Packet* parse_packet_recv(const std::string& buf);