#pragma once

typedef unsigned char   undefined;

typedef unsigned int    ImageBaseOffset32;
typedef unsigned char    byte;
typedef unsigned int    dword;
typedef long long    longlong;
typedef unsigned char    uchar;
typedef unsigned int    uint;
typedef unsigned long    ulong;
typedef unsigned char    undefined1;
typedef unsigned short    undefined2;
typedef unsigned int    undefined4;
typedef unsigned long long    undefined6;
typedef unsigned long long    undefined8;
typedef unsigned short    ushort;
typedef unsigned short    word;
typedef struct packet_struct packet_struct, *Ppacket_struct;

typedef uint UINT;

typedef uint UINT_PTR;

typedef UINT_PTR SOCKET;

struct packet_struct {
	void * vftable;
	int baadfood;
	void * field_0x8; /* dynamic */
	undefined4 field_0xc; /* 0 */
	undefined4 field_0x10; /* dynamic */
	undefined4 field_0x14; /* 0 */
	undefined4 field_0x18; /* 0x20008 */
	char * buf_recv; /* dynamic */
	uint buf_recv_len; /* 0x20000 */
	uint buf_recv_offset; /* dynamic (suele ser 18, a veces 48) */
	UINT field_0x28; /* parece ser buf_recv_offset copy? NOP, pero esta relacionado*/
	char * buf_send; /* dynamic */
	uint field_0x30; /* 0x1000 */
	uint buf_send_len; /* 0x1100 */
	uint buf_send_offset; /* 0 */
	void * field_0x3c; /* dynamic */
	uint field_0x40; /* 0x1008 */
	uint field_0x44; /* 0 */
	char field_0x48; /* 1 */
	undefined field_0x49;
	undefined field_0x4a;
	undefined field_0x4b;
	char flag_has_to_be_1; /* 1 */
	undefined field_0x4d;
	undefined field_0x4e;
	undefined field_0x4f;
	void * pointer_crypto; /* dynamic */
	void * pointer_crypto2; /* dynamic */
	uint field_0x58; /* 0 */
	SOCKET socket; /* dynamic */
	undefined field_0x60;
	undefined field_0x61;
	undefined field_0x62;
	undefined field_0x63;
	undefined field_0x64;
	undefined field_0x65;
	undefined field_0x66;
	undefined field_0x67;
	undefined field_0x68;
	undefined field_0x69;
	undefined field_0x6a;
	undefined field_0x6b;
	undefined field_0x6c;
	undefined field_0x6d;
	undefined field_0x6e;
	undefined field_0x6f;
	uint check_index;
	undefined field_0x74;
	undefined field_0x75;
	undefined field_0x76;
	undefined field_0x77;
	void * pointer_to_checks;
};

