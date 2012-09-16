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

P2VeraStream::~P2VeraStream() {
	if (NULL == qq) return; // || 0xFFFFFFFF == (unsigned int)qq
	if (0 == qq->decrease_ref_count()) { //Больше на этот объект ссылок не осталось. Удаляем его.
		delete qq;
	}
}

P2VeraStream::P2VeraStream(IP2VeraStreamQq* qq) {
	if (NULL == qq) {
		cout << "P2VeraStream::P2VeraStream error - null pointer to stream queue" << endl;
		return;
	}
	this->qq = qq;
	if (NULL==qq ) return; //|| 0xFFFFFFFF == (unsigned int)qq
	qq->increase_ref_count();
}

P2VeraStream::P2VeraStream(const P2VeraStream& pvis) {
	if (this == &pvis) {
		return;
	}
	qq = pvis.qq;
	if (NULL == pvis.qq) {
	} else {
		qq->increase_ref_count();
	}
}

P2VeraStream& P2VeraStream::operator=(const P2VeraStream& pvis) {
	if (this == &pvis) {
		return *this;
	}

	//if (NULL!=qq && 0 == qq->decrease_ref_count()) { //Уменьшаем количество ссылок на текущий объект.
	//	delete qq; //При необходимости удаляем его
	//}

	qq = pvis.qq;
	if (NULL != pvis.qq) {
		qq->increase_ref_count();
	}
	return *this;
}

P2VeraStream& P2VeraStream::operator<<(IP2VeraMessage& p2m) {
	qq->push_message(p2m);
	return *this;
}

P2VeraStream& P2VeraStream::operator>>(IP2VeraMessage& p2m) {
	if (NULL!=qq) qq->pop_message(p2m);
	return *this;
}

bool P2VeraStream::is_opened() {
	if (NULL == qq) return false;
	return qq->is_opened();
}

bool P2VeraStream::is_connected() {
	if (NULL == qq) return false;
	return qq->is_connected();
}

void P2VeraStream::close() {
	qq->close();
}

void P2VeraStream::flush() {
	if (NULL == qq) return;
	qq->flush();
}

void P2VeraStream::discard_incomming() {
	if (NULL == qq) return;
	qq->discard_incomming();
}

void P2VeraStream::discard_outcomming() {
	if (NULL == qq) return;
	qq->discard_outcomming();
}

bool P2VeraStream::has_incomming() {
	if (NULL == qq) return false;
	return qq->has_incomming();
}

bool P2VeraStream::has_outcomming() {
	if (NULL == qq) return false;
	return qq->has_outcomming();
}

stream_direction P2VeraStream::get_stream_direction() {
	if (NULL == qq) return stream_direction_loopback;
	return qq->get_stream_direction();
}

int P2VeraStream::get_fd() {
	if (NULL == qq) return 0;
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



