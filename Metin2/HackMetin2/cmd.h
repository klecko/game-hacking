#pragma once

#include <string>

extern uint ID_ATTACK;

/*
 * Handles user input from chat. The command is s. User must type @s, and s
 * gets here from the hook of Chat.
 *
 * Current commands:
 * - move type x y: moves to coords (x, y)
 * - attack: attacks the chosen target
 * - msg type message: sends the message as selected type
 * - send hexbuf: sends the packet built with the hextbuf
 *
 * TODO: handle errors PL0X
*/
void command(std::string s);