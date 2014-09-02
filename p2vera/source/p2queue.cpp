/*
 * p2queue.cpp
 *
 *  Created on: 15.07.2012
 *      Author: drozdov
 */

#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <iostream>

#include "p2queue.h"

using namespace std;

const unsigned int max_incoming_qq_size = 1000;

P2VeraStreamQq::P2VeraStreamQq(IP2VeraStreamHub *hub) {
	if (NULL == hub) {
		cout << "P2VeraStreamQq::P2VeraStreamQq error - Zero pointer to hub" << endl;
		return;
	}
	if (0 != pthread_mutex_init(&mtx, NULL)) {
		cout << "P2VeraStreamQq::P2VeraStreamQq error - couldn`t create mutex. Fatal." << endl;
		exit(1);
	}
	create_poll_pipe();

	this->hub = hub;
	ref_count = 1;
}

P2VeraStreamQq::~P2VeraStreamQq() {
	pthread_mutex_destroy(&mtx);
	::close(pipe_fd[1]);
	::close(pipe_fd[0]);
}

void P2VeraStreamQq::create_poll_pipe() {
	if (0 != pipe(pipe_fd)) {
		cout << "P2VeraStreamQq::create_poll_pipe error - couldn`t create pipe for input polling" << endl;
		return;
	}
	fd = pipe_fd[0];
}

bool P2VeraStreamQq::is_opened() {
	return opened;
}

bool P2VeraStreamQq::is_connected() {
	return true;
}

void P2VeraStreamQq::close() {
	::close(pipe_fd[1]);
	::close(pipe_fd[0]);
}

void P2VeraStreamQq::flush() {
}

void P2VeraStreamQq::discard_incomming() {
	pthread_mutex_lock(&mtx);
	queue<P2VeraBasicMessage> empty_qq;
	std::swap(in_qq, empty_qq);
	pthread_mutex_unlock(&mtx);
}

void P2VeraStreamQq::discard_outcomming() {
}

bool P2VeraStreamQq::has_incomming() {
	pthread_mutex_lock(&mtx);
	int qq_size = in_qq.size();
	pthread_mutex_unlock(&mtx);
	return (qq_size > 0);
}

bool P2VeraStreamQq::has_outcomming() {
	return false;
}

stream_direction P2VeraStreamQq::get_stream_direction() {
	return stream_direction_bidir;
}

int P2VeraStreamQq::get_fd() {
	return fd;
}

int P2VeraStreamQq::pop_message(IP2VeraMessage& p2m) {
	int res = 0;
	int v = 0;
	read(pipe_fd[0], &v, 1);
	pthread_mutex_lock(&mtx);
	if (in_qq.size() < 0) {
		res = -1;
	} else {
		P2VeraBasicMessage p2bm = in_qq.front();
		std::string str;
		p2bm.get_data(str);
		p2m.set_data(str);
		in_qq.pop();
	}
	pthread_mutex_unlock(&mtx);
	return res;
}

int P2VeraStreamQq::push_message(IP2VeraMessage& p2m) {
	if (hub->send_message(p2m)) return 1;
	return 0;
}


bool P2VeraStreamQq::increase_ref_count() {
	pthread_mutex_lock(&mtx);
	bool succed = (ref_count > 0);
	if (ref_count > 0) ref_count++;
	pthread_mutex_unlock(&mtx);
	return succed;
}

int P2VeraStreamQq::decrease_ref_count() {
	int tmp = 0;
	pthread_mutex_lock(&mtx);
	ref_count--;
	tmp = ref_count;
	pthread_mutex_unlock(&mtx);
	return tmp;
}

bool P2VeraStreamQq::is_referenced() {
	if (ref_count > 0) return true;
	return false;
}

void  P2VeraStreamQq::_insert_message(P2VeraBasicMessage& p2m) {
	pthread_mutex_lock(&mtx);
	if (in_qq.size() < max_incoming_qq_size) {
		string str; //Избавляемся от поведения copy on write, реализованого в std::string
		p2m.get_data(str);
		P2VeraBasicMessage p2m_copy; //FIXME Оригинальная строка P2VeraBasicMessage p2m_copy(str.data()); Выяснить безовасность с точки зрения copyon write
		p2m_copy.set_data((void*)str.data(), (int)str.size());
		in_qq.push(p2m_copy);
	} else {
		cout << "P2VeraStreamQq::_insert_message warning - queue overflow, message lost" << endl;
	}
	pthread_mutex_unlock(&mtx);
	int v = 0;
	write(pipe_fd[1], &v, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

P2VeraInStreamQq::P2VeraInStreamQq(IP2VeraStreamHub *hub) : P2VeraStreamQq(hub) {
}

P2VeraInStreamQq::~P2VeraInStreamQq() {
}

stream_direction P2VeraInStreamQq::get_stream_direction() {
	return stream_direction_income;
}

int P2VeraInStreamQq::push_message(IP2VeraMessage& p2m) {
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

P2VeraOutStreamQq::P2VeraOutStreamQq(IP2VeraStreamHub *hub) : P2VeraStreamQq(hub) {
}

P2VeraOutStreamQq::~P2VeraOutStreamQq() {
}

stream_direction P2VeraOutStreamQq::get_stream_direction() {
	return stream_direction_outcome;
}

int P2VeraOutStreamQq::pop_message(IP2VeraMessage& p2m) {
	return 0;
}

void P2VeraOutStreamQq::_insert_message(P2VeraBasicMessage& p2m) {
}


