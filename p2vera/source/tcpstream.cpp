/*
 * tcpstream.cpp
 *
 *  Created on: 06.08.2012
 *      Author: drozdov
 */


#include <list>
#include <map>
#include <iostream>

#include "p2stream.h"
#include "tcpstream.h"

using namespace std;


TcpStream::TcpStream(RemoteSrvUnit& rsu, int remote_port) {
	this->rsu = rsu;
	this->remote_port = remote_port;
}

TcpStream::~TcpStream() {
}

void TcpStream::add_hub(IP2VeraStreamHub* p2h, int flow_id) {
	if (NULL == p2h) {
		cout << "TcpStream::add_hub error - zero pointer to hub" << endl;
		return;
	}
	map<int, IP2VeraStreamHub*>::iterator id_it = flow_to_hub.find(flow_id);
	if (id_it != flow_to_hub.end()) {
		cout << "TcpStream::add_hub warning - flow with id " << flow_id << " was already registered" << endl;
		return;
	}
	map<IP2VeraStreamHub*, int>::iterator p2h_it = hub_to_flow.find(p2h);
	if (p2h_it != hub_to_flow.end()) {
		cout << "TcpStream::add_hub warning - hub was already registered" << endl;
		return;
	}
	flow_to_hub[flow_id] = p2h;
	hub_to_flow[p2h] = flow_id;
}

void TcpStream::start_network() {
}

