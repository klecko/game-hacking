#ifndef __HOOKSH__
#define __HOOKSH__

#include <Windows.h>
#include "packet_struct.h"

// Namespace for every address scanned by sigscan
namespace addr {
	extern DWORD Base;
	extern void* MySend;
	extern void* MyRecv;
	extern void* Chat;
	extern void* GetTime;
	extern void* AppendChat;
	extern void* ChatObject;
}

// Sigscans every function and object and saves its address in namespace addr
void sigscan();

// Performs detours and updates addresses of original functions
void detours();

/* Original and hook functions definitions.
 *
 * Note that original and hook functions do not use the same calling convention.
 * Since original ones use thiscall, hooks should too, but compiler doesn't 
 * allow defining thiscall functions outside classes, so we use fastcall.
 *
 * Thiscall passes `this` pointer in ecx, and the rest of arguments in the 
 * stack. Fastcall passes first two arguments in ecx and edx, and the rest
 * in the stack. If we use fastcall when it's truly thiscall, we get a dummy
 * second argument we are not interested in (edx).
 *
 * We can make both original and hook funcs have the same calling convention
 * using fastcall in originals, but then we would have to pass a dummy arg
 * for edx when calling them, so I considered this better.
*/
typedef int(__thiscall *pMySend_t)(packet_struct *_this);
typedef int(__thiscall *pMyRecv_t)(packet_struct *_this);
typedef int(__thiscall *pMyRecv_t)(packet_struct *_this);
typedef int(__thiscall *pChat_t)(packet_struct *_this, const char* input, char param_2);
typedef int(*pGetTime_t)();
typedef void(__thiscall *pAppendChat_t)(void* _this, int type, const char *msg);

extern pMySend_t OriginalMySend;
extern pMyRecv_t OriginalMyRecv;
extern pChat_t OriginalChat;
extern pGetTime_t OriginalGetTime;       //not hooking
extern pAppendChat_t OriginalAppendChat; //not hooking

// We don't really need them here, could be in the .cpp
int __fastcall HookMySend(packet_struct *_this);
int __fastcall HookMyRecv(packet_struct *_this);
int __fastcall HookChat(packet_struct *_this, int edx, const char* input, char param2);


#endif