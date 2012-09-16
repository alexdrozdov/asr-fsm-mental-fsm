/*
 * p2message.cpp
 *
 *  Created on: 15.07.2012
 *      Author: drozdov
 */

#include <string>
#include <iostream>

#include "p2message.h"

using namespace std;

P2VeraTextMessage::P2VeraTextMessage() {
	str = "";
}

P2VeraTextMessage::P2VeraTextMessage(std::string str) {
	this->str = str;
}

P2VeraTextMessage::P2VeraTextMessage(const P2VeraTextMessage& tm) {
	str = tm.str;
}

P2VeraTextMessage::~P2VeraTextMessage() {
}


bool P2VeraTextMessage::get_data(std::string& str) const {
	str = this->str;
	return true;
}

int P2VeraTextMessage::get_data(void* data, int max_data_size) const {
	unsigned int ret = str.copy((char*)data, max_data_size, 0);
	*(((char*)data)+ret) = 0;
	return (int)ret;
}

int P2VeraTextMessage::get_data_size() const {
	return str.size();
}

bool P2VeraTextMessage::set_data(void* data, int data_size) {
	if (NULL == data) return false;
	str.assign((char*)data, (unsigned int)data_size);
	return true;
}

bool P2VeraTextMessage::set_data(std::string &str) {
	this->str = str;
	return true;
}

P2VeraTextMessage::operator std::string() {
	return str;
}

void P2VeraTextMessage::print() {
	cout << str << endl;
}

////////////////////////////////////////////////////////////////////////////

P2VeraBasicMessage::P2VeraBasicMessage() {
	str = "";
}

P2VeraBasicMessage::P2VeraBasicMessage(std::string str) {
	this->str = str;
}

P2VeraBasicMessage::P2VeraBasicMessage(const IP2VeraMessage& tm) {
	tm.get_data(str);
}

P2VeraBasicMessage::~P2VeraBasicMessage() {
}


bool P2VeraBasicMessage::get_data(std::string& str) const {
	str = this->str;
	return true;
}

int P2VeraBasicMessage::get_data(void* data, int max_data_size) const {
	unsigned int ret = str.copy((char*)data, max_data_size, 0);
	*(((char*)data)+ret) = 0;
	return (int)ret;
}

int P2VeraBasicMessage::get_data_size() const {
	return str.size();
}

bool P2VeraBasicMessage::set_data(void* data, int data_size) {
	if (NULL == data) return false;
	str.assign((char*)data, (unsigned int)data_size);
	return true;
}

bool P2VeraBasicMessage::set_data(std::string &str) {
	this->str = str;
	return true;
}


void P2VeraBasicMessage::print() {
	cout << str << endl;
}



