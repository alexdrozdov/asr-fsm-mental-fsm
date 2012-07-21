/*
 * p2stream.cpp
 *
 *  Created on: 08.07.2012
 *      Author: drozdov
 */

#include <string>
#include <iostream>
#include "p2stream.h"

using namespace std;

IP2VeraMessage::~IP2VeraMessage() {
}

IP2VeraStreamQq::~IP2VeraStreamQq() {
}

P2VeraStream::P2VeraStream() {
	qq = NULL;
}

P2VeraStream::P2VeraStream(IP2VeraStreamQq* qq) {
	if (NULL == qq) {
		cout << "P2VeraStream::P2VeraStream error - null pointer to stream queue" << endl;
		return;
	}
	this->qq = qq;
	qq->increase_ref_count();
}

P2VeraStream::P2VeraStream(const P2VeraStream& pvis) {
	if (this == &pvis) {
		return;
	}
	qq = pvis.qq;
	qq->increase_ref_count();
}

P2VeraStream& P2VeraStream::operator=(const P2VeraStream& pvis) {
	if (this == &pvis) {
		return *this;
	}
	qq = pvis.qq;
	qq->increase_ref_count();
	return *this;
}

P2VeraStream& P2VeraStream::operator<<(IP2VeraMessage& p2m) {
	qq->push_message(p2m);
	return *this;
}

P2VeraStream& P2VeraStream::operator>>(IP2VeraMessage& p2m) {
	qq->pop_message(p2m);
	return *this;
}

bool P2VeraStream::is_opened() {
	return qq->is_opened();
}

bool P2VeraStream::is_connected() {
	return qq->is_connected();
}

void P2VeraStream::close() {
	qq->close();
}

void P2VeraStream::flush() {
	qq->flush();
}

void P2VeraStream::discard_incomming() {
	qq->discard_incomming();
}

void P2VeraStream::discard_outcomming() {
	qq->discard_outcomming();
}

bool P2VeraStream::has_incomming() {
	return qq->has_incomming();
}

bool P2VeraStream::has_outcomming() {
	return qq->has_outcomming();
}

stream_direction P2VeraStream::get_stream_direction() {
	return stream_direction_bidir;
}

int P2VeraStream::get_fd() {
	return qq->get_fd();
}

IP2VeraStreamQq* P2VeraStream::get_qq() {
	return qq;
}

bool P2VeraStream::is_equal(const P2VeraStream& pvis) {
	if (qq == pvis.qq) return true;
	return false;
}

bool operator==(const P2VeraStream& ls, const P2VeraStream& rs) {
	if (ls.qq == rs.qq) return true;
	return false;
}



