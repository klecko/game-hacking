#pragma once

#include <string>
#include <vector>


//extern const std::string COLOR_HACK;
//extern const std::string COLOR_ERR;

std::string string_to_hex(const std::string& input);
int u32(const std::string& buffer);
std::string p32(int n);
std::vector<std::string> split(const std::string& s, char delimiter);
std::string replace_all(std::string str, const std::string& from, const std::string& to);
//std::string color(byte r, byte g, byte b);
std::string color();
std::string color(std::string c);
void print(std::string msg);
void print_err(std::string msg);