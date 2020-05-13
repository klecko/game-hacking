#ifndef __HOOKSH__
#define __HOOKSH__

#include <Windows.h>
#include <vector>
#include "packet_struct.h"
#include "player.h"

// Namespace for game objects
namespace objects {
	extern void* Chat;
	extern player* Player;
}

// Reads the pointer list, being the first pointer the base addr + offsets[0]
// Example: {1,2,3} --> [[base + 1] + 2] + 3
void* read_pointer_list(std::vector<DWORD> offsets);

// Gets addresses of game objects
void get_objects_addresses();

// Gets addresses of game functions through sigscan
bool sigscan();

// Performs detours and updates addresses of original functions
bool detours();

// Checks if we are already logged in. TODO: check what happens when relogging
bool ingame();

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
typedef uint(*pGetTime_t)();
typedef void(__thiscall *pAppendChat_t)(void* _this, int type, const char *msg);
typedef byte(*pGetAttackByte_t)();

extern pMySend_t OriginalMySend;
extern pMyRecv_t OriginalMyRecv;
extern pChat_t OriginalChat;
extern pGetTime_t OriginalGetTime;       //not hooking
extern pAppendChat_t OriginalAppendChat; //not hooking
extern pGetAttackByte_t OriginalGetAttackByte; //not hooking

// We don't really need them here, could be in the .cpp
int __fastcall HookMySend(packet_struct *_this);
int __fastcall HookMyRecv(packet_struct *_this);
int __fastcall HookChat(packet_struct *_this, int edx, const char* input, char param2);


#endif