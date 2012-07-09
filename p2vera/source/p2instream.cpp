/*
 * p2instream.cpp
 *
 *  Created on: 09.07.2012
 *      Author: drozdov
 */

#include <string>
#include <vector>
#include <queue>

#include "p2stream.h"
#include "p2instream.h"

using namespace std;

P2VeraInStream::P2VeraInStream() {
}

P2VeraInStream::P2VeraInStream(const P2VeraInStream& pvis) {
	pqq = pvis.pqq;
	pqq->incr_refs();
}

P2VeraInStream::~P2VeraInStream() {
	int nref = pqq->decr_refs();
	if (0 == nref) {
		delete pqq;
	}
}

IP2VeraStream& P2VeraInStream::operator<<(IP2VeraMessage& p2m) {
	cout << "P2VeraInStream::operator<< warning - stream is input only" << endl;
	return *this;
}

IP2VeraStream& P2VeraInStream::operator>>(IP2VeraMessage& p2m) {
	pqq->pop_message(p2m);
	return *this;
}

bool P2VeraInStream::is_opened() {
	return pqq->is_opened();
}

bool P2VeraInStream::is_connected() {
	return pqq->is_connected();
}

void P2VeraInStream::close() {
	pqq->close();
}

void P2VeraInStream::flush() {
	pqq->flush();
}

void P2VeraInStream::discard_incomming() {
	pqq->discard_incomming();
}

void P2VeraInStream::discard_outcomming() {
	pqq->discard_outcomming();
}

bool P2VeraInStream::has_incomming() {
	return pqq->has_incomming();
}

bool P2VeraInStream::has_outcomming() {
	return pqq->has_outcomming();
}

stream_direction P2VeraInStream::get_stream_direction() {
	return pqq->get_stream_direction();
}

int P2VeraInStream::get_fd() {
	return pqq->get_fd();
}

P2VeraInStreamQq::~P2VeraInStreamQq() {
}

P2VeraInStreamQq::P2VeraInStreamQq() {
}

P2VeraInStreamQq::P2VeraInStreamQq(const P2VeraInStreamQq& tmplt) {
}

bool P2VeraInStreamQq::is_opened() {
	return true;
}

bool P2VeraInStreamQq::is_connected() {
	return true;
}

void P2VeraInStreamQq::close() {
}

void P2VeraInStreamQq::flush() {
}

void P2VeraInStreamQq::discard_incomming() {
}

void P2VeraInStreamQq::discard_outcomming() {
}

bool P2VeraInStreamQq::has_incomming() {
	return true;
}

bool P2VeraInStreamQq::has_outcomming() {
	return false;
}

stream_direction P2VeraInStreamQq::get_stream_direction() {
	return stream_direction_income;
}

int P2VeraInStreamQq::get_fd() {
	return 0;
}


