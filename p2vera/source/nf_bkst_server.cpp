/*
 * nf_bkst_server.cpp
 *
 *  Created on: 02.06.2012
 *      Author: drozdov
 */

#include <iostream>
#include <string>
#include <vector>

#include "nf_bkst_server.h"

using namespace std;

#define MIN_BKST_PING_DELTA 400

BkstNfServer::BkstNfServer(int id, net_find_config* rnfc) {
	local_id = id;
	enabled = true;

	memset(&remote_addr, 0, sizeof(sockaddr_in));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(atoi(rnfc->nf_port.c_str()));
	remote_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	min_ping_time_delta = MIN_BKST_PING_DELTA;
}

bool BkstNfServer::is_alive() {
	return false;
}

void BkstNfServer::forbide_usage() {
	enabled = false;
}
//Разрешает работу с этим сервером, т.е. с любым сервером,
//расположенным на этой удаленной машине
void BkstNfServer::enable_usage() {
	enabled = true;
}

int BkstNfServer::get_id() {
	return local_id;
}

string BkstNfServer::get_uniq_id() {
	return 0;
}

sockaddr_in&  BkstNfServer::get_remote_sockaddr() {
	return remote_addr;
}

void BkstNfServer::add_ping_request(int ping_id) {
	gettimeofday(&tv_request, NULL);
}

void BkstNfServer::add_ping_response(int ping_id) {
}

void BkstNfServer::validate_alive() {
}

void BkstNfServer::add_alternate_addr(sockaddr_in& alt_addr) {
}

bool BkstNfServer::validate_addr(sockaddr_in& alt_addr) {
	return false;
}

bool  BkstNfServer::ping_allowed() {
	if (!enabled) return false;

	timeval tv_now;
	gettimeofday(&tv_now, NULL);
	unsigned  int delta = ((1000000-tv_request.tv_usec)+(tv_now.tv_usec-1000000)+(tv_now.tv_sec-tv_request.tv_sec)*1000000) / 1000;
	if (delta < min_ping_time_delta) {
		return false; //Не прошло минимальное время с последнего опроса этого сервера
	}
	return true; //Можно пиговать
}

bool BkstNfServer::requires_info_request() {
	return false;
}

//Сервер не существует в реальности а представляет все сервера, которые можно обнаружить вещательным запросом
bool BkstNfServer::is_broadcast() {
	return true;
}

void BkstNfServer::print_info() {
	cout << "server type: broadcast" << endl;
	cout << "address: " << inet_ntoa(remote_addr.sin_addr) << ":" << htons(remote_addr.sin_port) << endl;
}


