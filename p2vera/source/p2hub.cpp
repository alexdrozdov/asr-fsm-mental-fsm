/*
 * p2hub.cpp
 *
 *  Created on: 15.07.2012
 *      Author: drozdov
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
	this->name = name;
	if (0 != pthread_mutex_init(&mtx, NULL)) {
		cout << "P2VeraStreamHub::P2VeraStreamHub error - couldn`t create mutex. Fatal." << endl;
		exit(1);
	}
	snd_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (snd_sock < 0) {
		cout << "P2VeraStreamHub::P2VeraStreamHub error - couldn`t open socket for sending messages" << endl;
		perror("socket");
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

std::string P2VeraStreamHub::get_name() {
	return name;
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
	//P2VeraStream vs_empty((P2VeraStreamQq*)0xFFFFFFFF);
	//qqs.push_back(vs_empty);
	//list<P2VeraStream>::reverse_iterator it = qqs.rbegin();
	//*it = vs;
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
	//Рассылаем сообщение всем удаленным хабам, соединенным с текущим хабом
	for (list<remote_hub>::iterator it=remote_hubs.begin();it!=remote_hubs.end();it++) {
		sockaddr_in& sa = it->sa;
		if (sendto(snd_sock, data.data(), data.size(), 0, (sockaddr *)&sa, sizeof(sockaddr_in)) < 0) {
			cout << "P2VeraStreamHub::send_message error - couldn`t send message" << endl;
			perror("sendto");
		}
	}
	//Рассылаем сообщение через какналы tcp
	for (list<TcpStream*>::iterator it=tcsl.begin();it!=tcsl.end();it++) {
		TcpStream* tcs = *it;
		tcs->send_message(p2m, this);
	}
	pthread_mutex_unlock(&mtx);
	return true;
}

bool P2VeraStreamHub::receive_message(IP2VeraMessage& p2m) {
	string data;
	p2m.get_data(data);
	P2VeraBasicMessage p2bm(p2m);
	pthread_mutex_lock(&mtx);
	for (list<P2VeraStream>::iterator it=qqs.begin();it!=qqs.end();it++) {
		IP2VeraStreamQq& qq = *(it->get_qq());
		stream_direction sd = qq.get_stream_direction();
		if (stream_direction_bidir==sd || stream_direction_income==sd) {
			qq._insert_message(p2bm);
		}
	}
	pthread_mutex_unlock(&mtx);
	return true;
}

bool P2VeraStreamHub::add_message_target(RemoteSrvUnit rsu, int port) {
	//cout << "P2VeraStreamHub::add_message_target info - launched for " << rsu.get_uniq_id() << " at port " << port << endl;
	pthread_mutex_lock(&mtx);
	remote_hub rh;
	rh.rsu = rsu;
	rh.port = port;
	memset(&rh.sa, 0, sizeof(sockaddr_in));
	rh.sa.sin_addr.s_addr = rsu.get_remote_sockaddr().sin_addr.s_addr;
	rh.sa.sin_port = htons(port);
	remote_hubs.push_back(rh);
	pthread_mutex_unlock(&mtx);
	return true;
}

bool P2VeraStreamHub::remove_message_target(RemoteSrvUnit rsu) {
	//cout << "P2VeraStreamHub::remove_message_target info - launched for " << rsu.get_uniq_id() << endl;
	pthread_mutex_lock(&mtx);
	for (list<remote_hub>::iterator rh_it=remote_hubs.begin();rh_it!=remote_hubs.end();) {
		if (rh_it->rsu != rsu) {
			++rh_it;
			continue;
		}
		rh_it = remote_hubs.erase(rh_it);
	}
	pthread_mutex_unlock(&mtx);
	return true;
}

bool P2VeraStreamHub::add_tcp_stream(TcpStream* tcs) {
	pthread_mutex_lock(&mtx);
	tcsl.push_back(tcs);
	pthread_mutex_unlock(&mtx);
	return true;
}


