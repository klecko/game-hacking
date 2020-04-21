#ifndef __HOOKSH__
#define __HOOKSH__

#include <Windows.h>
#include "packet_struct.h"

namespace addr {
	extern DWORD Base;
	extern void* MySend;
	extern void* MyRecv;
	extern void* Chat;
	extern void* GetTime;
	extern void* AppendChat;
	extern void* ChatObject;
}

extern bool ready_to_send;

void sigscan();
void detours();

typedef int(__thiscall *pMySend_t)(packet_struct *_this);
typedef int(__thiscall *pMyRecv_t)(packet_struct *_this);
typedef int(__thiscall *pChat_t)(packet_struct *_this, const char* input, char param_2);
typedef int(*pGetTime_t)();
typedef void(__thiscall *pAppendChat_t)(void* _this, int type, const char *msg);

extern pMySend_t OriginalMySend;
extern pMyRecv_t OriginalMyRecv;
extern pChat_t OriginalChat;
extern pGetTime_t OriginalGetTime; //not hooking
extern pAppendChat_t OriginalAppendChat; //not hooking

int __fastcall HookMySend(packet_struct *_this);
int __fastcall HookMyRecv(packet_struct *_this);
int __fastcall HookChat(packet_struct *_this, int edx, const char* input, char param2);

#endif