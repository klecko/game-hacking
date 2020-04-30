#include <string>
#include <vector>
#include <sstream>
#include <iostream> //quitar
#include <sstream>
#include <iomanip>

#include "hooks.h"

using namespace std;

typedef unsigned char byte;

const string COLOR_HACK = "00ffff";
const string COLOR_ERR = "ff0000";

string string_to_hex(const string& input){
	static const char* const lut = "0123456789ABCDEF";

	string output;
	size_t len = input.length();
	output.reserve(2 * len);
	for (size_t i = 0; i < len; ++i)
	{
		const byte c = input[i];
		output.push_back(lut[c >> 4]);
		output.push_back(lut[c & 15]);
	}
	return output;
}

string hex_to_string(const string& input){
	string output;
	string byte_str;
	for (size_t i = 0; i < input.length(); i+=2){
		byte_str = input.substr(i, 2);
		output += (byte)stol(byte_str, NULL, 16);
	}
	return output;
}

int u(int bytes, const string& buffer){
	int result = 0;
	for (int i = 0; i < bytes; i++)
		result |= (((byte)buffer[i]) << i*8);
	return result;
}

string p(int bytes, int n){
	string result;
	for (int i = 0; i < bytes; i++)
		result += (byte)((n >> (i*8)) & 0xFF);
	return result;
}

vector<string> split(const string& s, char delimiter)
{
	vector<std::string> tokens;
	string token;
	istringstream tokenStream(s);
	while (getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

std::string replace_all(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

string color() {
	string s = "|h|r"; //nocolor
	return s;
}

string color(string c){
	string s = "";
	if (c.length() == 6)
		s = "|cFF" + c + "|H|h";
	else
		cout << "ERROR CALLING COLOR" << endl;
	return s;
}

void print(string msg) {
	msg = color(COLOR_HACK) + "[HACK] " + color() + msg;
	OriginalAppendChat(addr::ChatObject, 0, msg.c_str());
}

void print_err(string msg){
	print(color(COLOR_ERR) + "[ERROR] " + color() + msg);
}

bool ingame(){
	// When we are inside the game, this becomes 0xCA
	return ( *(byte*)((DWORD)addr::ChatObject + 0x54) == 0xCA );
}