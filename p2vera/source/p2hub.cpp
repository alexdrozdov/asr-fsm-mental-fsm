/*
 * p2hub.cpp
 *
 *  Created on: 15.07.2012
 *      Author: drozdov
 */

#include <stdlib.h>

#include <iostream>
#include <list>
#include <string>

#include "p2vera.h"
#include "p2hub.h"
#include "p2queue.h"

using namespace std;

IP2VeraStreamHub::~IP2VeraStreamHub() {
}


P2VeraStreamHub::P2VeraStreamHub(std::string name) {
	if (0 != pthread_mutex_init(&mtx, NULL)) {
		cout << "P2VeraStreamHub::P2VeraStreamHub error - couldn`t create mutex. Fatal." << endl;
		exit(1);
	}
}

P2VeraStreamHub::~P2VeraStreamHub() {
	pthread_mutex_lock(&mtx);
	{
		std::list<P2VeraStream> empty_qqs;
		std::swap(qqs,empty_qqs);
	}
	pthread_mutex_unlock(&mtx);

	pthread_mutex_destroy(&mtx);
}

P2VeraStream P2VeraStreamHub::create_stream() {
	P2VeraStreamQq* qq = new P2VeraStreamQq(this);
	P2VeraStream vs(qq);
	qqs.push_back(vs);
	return vs;
}

P2VeraStream P2VeraStreamHub::create_instream() {
	P2VeraStreamQq* qq = new P2VeraInStreamQq(this);
	P2VeraStream vs(qq);
	pthread_mutex_lock(&mtx);
	qqs.push_back(vs);
	pthread_mutex_unlock(&mtx);
	return vs;
}

P2VeraStream P2VeraStreamHub::create_outstream() {
	P2VeraStreamQq* qq = new P2VeraOutStreamQq(this);
	P2VeraStream vs(qq);
	pthread_mutex_lock(&mtx);
	qqs.push_back(vs);
	pthread_mutex_unlock(&mtx);
	return vs;
}

void P2VeraStreamHub::unlink_stream(P2VeraStream p2s) {
	pthread_mutex_lock(&mtx);
	std::list<P2VeraStream> new_qqs;
	for (std::list<P2VeraStream>::iterator it = qqs.begin();qqs.end()!=it;it++) {
		if (p2s == *it) continue;
		new_qqs.push_back(*it);
	}
	std::swap(qqs,new_qqs);
	pthread_mutex_unlock(&mtx);
}

bool P2VeraStreamHub::send_message(IP2VeraMessage& p2m) {
	string data;
	p2m.get_data(data);
	P2VeraBasicMessage p2bm(p2m);
	//Сначала рассылаем сообщение всем очередям, которые подключены к этому хабу
	pthread_mutex_lock(&mtx);
	for (list<P2VeraStream>::iterator it=qqs.begin();it!=qqs.end();it++) {
		IP2VeraStreamQq& qq = *(it->get_qq());
		stream_direction sd = qq.get_stream_direction();
		if (stream_direction_bidir==sd || stream_direction_income==sd) {
			qq._insert_message(p2bm);
		}
	}
	pthread_mutex_unlock(&mtx);
	//Рассылаем сообщение всем удаленным хабам, соединенным с текущим хабом
	return false;
}

bool P2VeraStreamHub::add_message_target(RemoteSrvUnit rsu) {
	return false;
}

bool P2VeraStreamHub::remove_message_target(RemoteSrvUnit rsu) {
	return false;
}


