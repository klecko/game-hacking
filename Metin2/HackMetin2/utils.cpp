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

int u32(const string& buffer){
	int a = 0;
	for (int i = 0; i < 4; i++)
		a = a | (((byte)buffer[i]) << i*8);
		/*int((byte)(buffer[0])|
		(byte)(buffer[1]) << 8 |
		(byte)(buffer[2]) << 16 |
		(byte)(buffer[3]) << 24);*/
	return a;
}

string p32(int n){
	string result;
	for (int i = 0; i < 4; i++)
		result += (byte)((n >> (i*8)) & 0xff);
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
/*
string color(byte r, byte g, byte b){
	
	//|cFFFFFF00|H|hhola|cFFA7FFD4

	//|cFFA7FFD4|H|hhola
	
	//cout << string_to_hex((char)r) << endl;
	stringstream s;
	s << hex << "|cFF";
	s << setw(2) << setfill('0') << (int)r << setw(2) << setfill('0') << (int)g << setw(2) << setfill('0')  << (int)b << "|H|h";
	//cout << "printing color: " << s.str() << endl;
	return s.str();
}*/

string color() {
	string s = "|h|r"; //nocolor
	return s;
}

string color(string c){
	string s;
	if (c.length() == 6)
		s = "|cFF" + c + "|H|h";
	else
		cout << "ERROR CALLING COLOR" << endl;
	return s;
}

void print(string msg) {
	msg = color(COLOR_HACK) + "[HACK]" + color() + msg;
	OriginalAppendChat(addr::ChatObject, 0, msg.c_str());
}

void print_err(string msg){
	print(color(COLOR_ERR) + "[ERROR] " + color() + msg);
}