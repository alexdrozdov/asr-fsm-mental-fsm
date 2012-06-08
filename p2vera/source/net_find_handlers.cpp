/*
 * net_link_handlers.cpp
 *
 *  Created on: 16.05.2012
 *      Author: drozdov
 */

#include <stdio.h>

#include <ifaddrs.h>
#include <netinet/in.h>

#include <iostream>
#include <string>

#include "net_find.h"
#include "msg_wrapper.pb.h"
#include "msg_netfind.pb.h"
#include "net_find_handlers.h"

using namespace std;
using namespace p2vera;
using namespace netfind;

bool NetFindLinkHandler::cmp_addr(sockaddr_in& sa_1, sockaddr_in& sa_2) {
	if (sa_1.sin_port==sa_2.sin_port && sa_1.sin_addr.s_addr==sa_2.sin_addr.s_addr) {
		return true;
	}
	return false;
}

void NetFindLinkHandler::load_ifinfo() {
	struct ifaddrs *ifa_ptr, *ifa_ptr_tmp;

	if (getifaddrs(&ifa_ptr)!=0) {
		perror("ERROR getifaddrs: ");
		return;
	}
	for (ifa_ptr_tmp=ifa_ptr; ifa_ptr_tmp->ifa_next!=NULL; ifa_ptr_tmp=ifa_ptr_tmp->ifa_next) {
		if (ifa_ptr_tmp->ifa_addr->sa_family!=AF_INET)
			continue;
		printf("Address = %s\n", inet_ntoa(((struct sockaddr_in *)(ifa_ptr_tmp->ifa_addr))->sin_addr));
		local_ips.push_back(*((struct sockaddr_in *)(ifa_ptr_tmp->ifa_addr)));
	}
	freeifaddrs(ifa_ptr);
}

bool NetFindLinkHandler::is_localhost(sockaddr_in& sa) {
	vector<sockaddr_in>::iterator it;
	for (it=local_ips.begin();it!=local_ips.end();it++) {
		if (it->sin_addr.s_addr==sa.sin_addr.s_addr) {
			return true;
		}
	}
	return false;
}

NetFindLinkHandler::NetFindLinkHandler(NetFind* nf, int msg_id) {
	if (NULL == nf) {
		cout << "NetFindLinkHandler::NetFindLinkHandler error - NULL pointer to NetFind" << endl;
		return;
	}
	this->nf = nf;

	load_ifinfo();

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

	cookie_status remote_status = cookie_status_none;

	//Ищем пару ip-адрес--порт-ответа среди ранее зарегистрированных
	sockaddr_in sa_rq;
	memcpy(&sa_rq, remote_addr, sizeof(sockaddr_in));
	sa_rq.sin_port = htons(mlr.rp_port());
	bool is_known_addr = true;
	IRemoteNfServer* sa_rnfs = nf->by_sockaddr(sa_rq);
	if (NULL == sa_rnfs) {
		is_known_addr = false;
	}

	//Проверяем статус этого сервера, ищем его идентификатор среди ранее зарегистрированных
	bool is_known_server = true;
	IRemoteNfServer* rnfs = nf->by_uniq_id(mlr.rq_cookie_id());
	if (NULL == rnfs) {
		is_known_server = false; //Такой сервер ранее не встречался
		cout << "NetFindLinkHandler::handle_request info - discovered new server" << endl;
		cout << "\tAddress: " << inet_ntoa(sa_rq.sin_addr) << ":" << mlr.rp_port() << endl;
		cout << "\tUniq id: " << mlr.rq_cookie_id() << endl;

		int nid = nf->add_discovered_server(sa_rq, mlr.rq_cookie_id());
		if (is_localhost(sa_rq)) {
			IRemoteNfServer* i_rnfs = nf->by_id(nid);
			vector<sockaddr_in>::iterator it;
			for (it=local_ips.begin();it!=local_ips.end();it++) {
				i_rnfs->add_alternate_addr(*it);
			}
		}

		if (is_known_addr) {
			remote_status = cookie_status_modified; //Сервер зарегистрировался по адресу, ранее принадлежавшему другому приложению
		} else {
			remote_status = cookie_status_newbie;   //Новый сервер зарегистрировался по новому адресу. Он совсем новый
		}
	} else {
		//Сервер с таким идентификаторм уже существует...
		if (sa_rnfs && 0==memcmp(&rnfs->get_remote_sockaddr(), &sa_rnfs->get_remote_sockaddr(), sizeof(sockaddr_in))) {
			remote_status = cookie_status_actual; //Параметры сервера не изменились. С сервером все в порядке
		} else {
			remote_status = cookie_status_exists; //Такой сервер был зарегистрирован, но по другому адресу
		}
	}

	msg_link_rsp mlrs;
	mlrs.set_rq_cookie_id(mlr.rq_cookie_id());
	mlrs.set_rp_cookie_id(nf->get_uniq_id());
	mlrs.set_status(remote_status);
	mlrs.set_rq_port(nf->get_client_port());
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

	sockaddr_in sa_rq;
	memcpy(&sa_rq, remote_addr, sizeof(sockaddr_in));
	sa_rq.sin_port = htons(mlrs.rq_port());

	IRemoteNfServer* rnfs = nf->by_uniq_id(mlrs.rp_cookie_id());
	if (NULL == rnfs) { //Сервер с таким идентификатором не найден. Регистрируем его
		cout << "NetFindLinkHandler::handle_response info - discovered new server" << endl;
		cout << "\tAddress: " << inet_ntoa(sa_rq.sin_addr) << ":" << mlrs.rq_port() << endl;
		cout << "\tUniq id: " << mlrs.rp_cookie_id() << endl;
		int nid = nf->add_discovered_server(sa_rq, mlrs.rp_cookie_id());
		if (is_localhost(sa_rq)) {
			IRemoteNfServer* i_rnfs = nf->by_id(nid);
			vector<sockaddr_in>::iterator it;
			for (it=local_ips.begin();it!=local_ips.end();it++) {
				i_rnfs->add_alternate_addr(*it);
			}
		}
		return true;
	}

	//Сервер с таким идентификатором уже встречался ранее
	if (rnfs->validate_addr(sa_rq)) {
		rnfs->add_ping_response(wrpr->msg_id());
	} else {
		cout << "NetFindLinkHandler::handle_response warning - remote address changed for " << mlrs.rp_cookie_id() << endl;
		//cout << "Old value " << inet_ntoa(rnfs->get_remote_sockaddr().sin_addr) << ":" << rnfs->get_remote_sockaddr().sin_port << ", new value " << inet_ntoa(sa_rq.sin_addr) <<  ":" << sa_rq.sin_port<< endl;
		cout << "Old value " << rnfs->get_remote_sockaddr().sin_addr.s_addr << ":" << rnfs->get_remote_sockaddr().sin_port << ", new value " << sa_rq.sin_addr.s_addr <<  ":" << sa_rq.sin_port<< endl;
	}

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

	int sock = 0;
	if (remote_server->is_broadcast()) {
		sock = rq_bkst_socket;
	} else {
		sock = rq_socket;
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
	if (sendto(sock, wrpr_data, wrpr.ByteSize(),
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


