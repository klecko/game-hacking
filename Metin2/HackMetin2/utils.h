#pragma once

#include <string>
#include <vector>

// Hex representation of an ascii string
std::string string_to_hex(const std::string& input);

// Unpacking and packing
#define u16(buf) u(2, buf)
#define u32(buf) u(4, buf)
#define u64(buf) u(8, buf)
#define p16(n)   p(2, n)
#define p32(n)   p(4, n)
#define p64(n)   p(8, n)
int u(int bytes, const std::string& buffer);
std::string p(int bytes, int n);

// Split a string using a delimiter
std::vector<std::string> split(const std::string& s, char delimiter);

// Replaces every substring inside a string
std::string replace_all(std::string str, const std::string& from, const std::string& to);

// Returns Metin2 format color modifier for no color
std::string color();

// Returns Metin2 format color modifier for specified color hex code
std::string color(std::string c);

// Prints a general message to the Metin2 chat
void print(std::string msg);

// Prints an error message to the Metin2 chat
void print_err(std::string msg);

// Checks if we are already logged in. TODO: check what happens when relogging
bool ingame();