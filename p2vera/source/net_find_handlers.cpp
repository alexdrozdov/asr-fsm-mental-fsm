/*
 * net_link_handlers.cpp
 *
 *  Created on: 16.05.2012
 *      Author: drozdov
 */

#include <stdio.h>

#include <iostream>
#include <string>

#include "net_find.h"
#include "msg_wrapper.pb.h"
#include "msg_netfind.pb.h"
#include "net_find_handlers.h"

using namespace std;
using namespace p2vera;
using namespace netfind;


NetFindLinkHandler::NetFindLinkHandler(NetFind* nf, int msg_id) {
	if (NULL == nf) {
		cout << "NetFindLinkHandler::NetFindLinkHandler error - NULL pointer to NetFind" << endl;
		return;
	}
	this->nf = nf;

	pthread_mutex_init (&mtx, NULL);
	nmsg_index = 0; //Счетчик идентификаторов сообщения

	rq_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (rq_socket < 0) {
		cout << "NetFindLinkHandler::NetFindLinkHandler error - couldn`t open socket for sending requests" << endl;
		perror("socket");
	}

	rq_bkst_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (rq_bkst_socket < 0) {
		cout << "NetFindLinkHandler::NetFindLinkHandler error - couldn`t open socket for sending broadcast requests" << endl;
		perror("socket");
	}
	int broadcast = 1;
	setsockopt(rq_bkst_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
	memset(&rq_bkst_addr, 0, sizeof(sockaddr_in));
	rq_bkst_addr.sin_family = AF_INET;
	rq_bkst_addr.sin_port = htons((unsigned short)nf->get_server_port());
	rq_bkst_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	broadcast_time_delta = 100; //100мс - минимальный интервал вещательных запросов
}

NetFindLinkHandler::~NetFindLinkHandler() {
}

int NetFindLinkHandler::get_msg_id() {
	pthread_mutex_lock(&mtx);
	int n = nmsg_index++;
	pthread_mutex_unlock(&mtx);
	return n;
}

bool NetFindLinkHandler::HandleMessage(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr) {
	switch (wrpr->dir()) {
		case msg_wrapper_msg_direction_request:
			handle_request(wrpr,remote_addr);
			break;
		case msg_wrapper_msg_direction_response:
			handle_response(wrpr,remote_addr);
			break;
		default:
			cout << "NetFind::server_response_thread warning - unknown message direction" << endl;
			break;
	}
	return true;
}

bool NetFindLinkHandler::handle_request(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr) {
	msg_link_rq mlr;
	if (!mlr.ParseFromString(wrpr->body())) {
		cout << "NetFindLinkHandler::handle_request error - failed to parse message" << endl;
		return false;
	}

	msg_link_rsp mlrs;
	mlrs.set_rq_cookie_id(mlr.rq_cookie_id());
	mlrs.set_rp_cookie_id(nf->get_uniq_id());
	mlrs.set_status(cookie_status_actual);
	string mlrs_str = "";
	mlrs.SerializeToString(&mlrs_str);

	msg_wrapper nwrpr;
	nwrpr.set_msg_id(wrpr->msg_id());
	nwrpr.set_msg_type(msg_wrapper_msg_type_link);
	nwrpr.set_dir(msg_wrapper_msg_direction_response);
	nwrpr.set_body(mlrs_str);
	char *nwrpr_data = new char[nwrpr.ByteSize()];
	if (!nwrpr.SerializeToArray(nwrpr_data, nwrpr.ByteSize())) {
		cout << "NetFindLinkHandler::handle_request error - couldn`t serialize message wrapper" << endl;
		delete[] nwrpr_data;
		return false;
	}
	int rs_socket = socket(AF_INET, SOCK_DGRAM, 0);
	sockaddr_in response_addr;
	memset(&response_addr, 0, sizeof(sockaddr_in));
	response_addr.sin_family = AF_INET;
	response_addr.sin_addr.s_addr = remote_addr->sin_addr.s_addr;
	response_addr.sin_port = htons(mlr.rp_port());
	if (rs_socket < 0) {
		cout << "NetFindLinkHandler::handle_request error - couldn`t create socket" << endl;
		perror("socket");
	} else 	if (sendto(rs_socket, nwrpr_data, nwrpr.ByteSize(), 0, (sockaddr*)&response_addr, sizeof(sockaddr_in)) < 0) {
		cout << "NetFindLinkHandler::handle_request error - couldn`t send response" << endl;
		perror("sendto");
	}
	close(rs_socket);
	delete[] nwrpr_data;
	return true;
}

bool NetFindLinkHandler::handle_response(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr) {
	msg_link_rsp mlrs;
	if (!mlrs.ParseFromString(wrpr->body())) {
		cout << "NetFindLinkHandler::handle_response error - failed to parse message" << endl;
		return false;
	}

	cout << "response from " << mlrs.rp_cookie_id() << endl;
	return true;
}

bool NetFindLinkHandler::InvokeRequest() {
	timeval tv_now;
	gettimeofday(&tv_now, NULL);
	unsigned  int delta = ((1000000-tv_broadcast_request.tv_usec)+(tv_now.tv_usec-1000000)+(tv_now.tv_sec-tv_broadcast_request.tv_sec)*1000000) / 1000;
	if (delta < broadcast_time_delta) {
		return true; //Не прошло минимальное время с последнего вещательного запроса
	}
	tv_broadcast_request = tv_now;

	msg_link_rq mlr;
	mlr.set_rq_cookie_id(nf->get_uniq_id());
	mlr.set_rp_port(nf->get_client_port());
	string mlr_str = "";
	mlr.SerializeToString(&mlr_str);
	msg_wrapper wrpr;

	//Формируем пакет-обертку над сообщением
	wrpr.set_msg_id(get_msg_id());
	wrpr.set_msg_type(msg_wrapper_msg_type_link);
	wrpr.set_body(mlr_str);
	char *wrpr_data = new char[wrpr.ByteSize()];
	if (!wrpr.SerializeToArray(wrpr_data, wrpr.ByteSize())) {
		cout << "NetFindLinkHandler::InvokeRequest error - couldn`t serialize message wrapper" << endl;
		delete[] wrpr_data;
		return false;
	}
	if (sendto(rq_bkst_socket, wrpr_data, wrpr.ByteSize(), 0, (sockaddr *)&rq_bkst_addr, sizeof(sockaddr_in)) < 0) {
		cout << "NetFindLinkHandler::InvokeRequest error - couldn`t send request" << endl;
		perror("sendto");
	}
	delete[] wrpr_data;
	return true;
}

bool NetFindLinkHandler::InvokeRequest(IRemoteNfServer *remote_server) {
	if (!remote_server->ping_allowed()) {
		return true;
	}
	msg_link_rq mlr;
	mlr.set_rq_cookie_id(nf->get_uniq_id());
	mlr.set_rp_port(nf->get_client_port());
	string mlr_str = "";
	mlr.SerializeToString(&mlr_str);
	msg_wrapper wrpr;

	//Формируем пакет-обертку над сообщением
	int ping_id = get_msg_id();
	wrpr.set_msg_id(ping_id);
	wrpr.set_msg_type(msg_wrapper_msg_type_link);
	wrpr.set_body(mlr_str);
	char *wrpr_data = new char[wrpr.ByteSize()];
	if (!wrpr.SerializeToArray(wrpr_data, wrpr.ByteSize())) {
		cout << "NetFindLinkHandler::InvokeRequest error - couldn`t serialize message wrapper" << endl;
		delete[] wrpr_data;
		return false;
	}
	if (sendto(rq_socket, wrpr_data, wrpr.ByteSize(),
			0,
			(sockaddr *)&(remote_server->get_remote_sockaddr()),
			sizeof(sockaddr_in)) < 0) {
		cout << "NetFindLinkHandler::InvokeRequest error - couldn`t send request" << endl;
		perror("sendto");
	} else {
		remote_server->add_ping_request(ping_id);
	}
	delete[] wrpr_data;
	return true;
}


NetFindInfoHandler::NetFindInfoHandler(NetFind* nf, int msg_id) {
}

NetFindInfoHandler::~NetFindInfoHandler() {
}

bool NetFindInfoHandler::HandleMessage(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr) {
	return false;
}


