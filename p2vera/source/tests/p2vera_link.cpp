/*
 * p2vera_link.cpp
 *
 *  Created on: 30.04.2013
 *      Author: drozdov
 */

#include <iostream>

#include "p2vera_link.h"

using namespace std;

AsrMsg1::AsrMsg1() {
}

AsrMsg1::AsrMsg1(std::string str) {
}

AsrMsg1::AsrMsg1(const IP2VeraMessage& tm) {
}

AsrMsg1::~AsrMsg1() {
}

bool AsrMsg1::get_data(std::string& str) const {
	return false;
}

int AsrMsg1::get_data(void* data, int max_data_size) const {
	return 0;
}

int AsrMsg1::get_data_size() const {
	return 0;
}

bool AsrMsg1::set_data(void* data, int data_size) {
	return true;
}

bool AsrMsg1::set_data(std::string &str) {
	return true;
}


void AsrMsg1::print() {
}


//////////////////////////////////////////////////////////////////


AsrMsg2::AsrMsg2() {
}

AsrMsg2::AsrMsg2(std::string str) {
}

AsrMsg2::AsrMsg2(const IP2VeraMessage& tm) {
}

AsrMsg2::~AsrMsg2() {
}

bool AsrMsg2::get_data(std::string& str) const {
	return false;
}

int AsrMsg2::get_data(void* data, int max_data_size) const {
	return 0;
}

int AsrMsg2::get_data_size() const {
	return 0;
}

bool AsrMsg2::set_data(void* data, int data_size) {
	return true;
}

bool AsrMsg2::set_data(std::string &str) {
	return true;
}


void AsrMsg2::print() {
}


//////////////////////////////////////////////////////////////////

AsrP2vFlush::AsrP2vFlush() {}
AsrP2vFlush::~AsrP2vFlush(){}

bool AsrP2vFlush::get_data(std::string& str) const {
	return false;
}

int AsrP2vFlush::get_data(void* data, int max_data_size) const {
	return 0;
}

int AsrP2vFlush::get_data_size() const {
	return 0;
}

bool AsrP2vFlush::set_data(void* data, int data_size) {
	return true;
}

bool AsrP2vFlush::set_data(std::string &str) {
	return true;
}


//////////////////////////////////////////////////////////////////

AsrP2Stream::AsrP2Stream() {
}

AsrP2Stream::AsrP2Stream(IP2VeraStreamQq* qq) :
P2VeraStream(qq) {}

AsrP2Stream::AsrP2Stream(const P2VeraStream& pvis) :
P2VeraStream(pvis){
}

AsrP2Stream::~AsrP2Stream() {
}

AsrP2Stream& AsrP2Stream::operator=(const AsrP2Stream& pvis) {
	return *this;
}

AsrP2Stream& AsrP2Stream::operator<<(AsrMsg1& p2m) {
	cout << "m1" << endl;
	return *this;
}

AsrP2Stream& AsrP2Stream::operator>>(AsrMsg1& p2m) {
	return *this;
}

AsrP2Stream& AsrP2Stream::operator<<(AsrMsg2& p2m) {
	cout << "m2" << endl;
	return *this;
}

AsrP2Stream& AsrP2Stream::operator>>(AsrMsg2& p2m) {
	return *this;
}

AsrP2Stream& AsrP2Stream::operator<<(AsrP2vFlush& flush) {
	cout << "flushed" << endl;
	return *this;
}


AsrP2vFlush asr_fl;


