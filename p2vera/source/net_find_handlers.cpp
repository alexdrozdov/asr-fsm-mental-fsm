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
#include <sstream>
#include <string>
#include <list>

#include "net_find.h"
#include "msg_wrapper.pb.h"
#include "msg_netfind.pb.h"
#include "net_find_handlers.h"
#include "nf_remote_server.h"

using namespace std;
using namespace p2vera;
using namespace netfind;

#define MIN_FULL_LIST_MESSAGES 5
#define MIN_LIST_RQ_TIME 5000

bool NetFindLinkHandler::cmp_addr(sockaddr_in& sa_1, sockaddr_in& sa_2) {
	if (sa_1.sin_port==sa_2.sin_port && sa_1.sin_addr.s_addr==sa_2.sin_addr.s_addr) {
		return true;
	}
	return false;
}

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
	close(rq_socket);
	close(rq_bkst_socket);
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

	//Проверяем статус этого сервера, ищем его идентификатор среди ранее зарегистрированных
	try {
		RemoteSrvUnit rsu = nf->get_by_uniq_id(mlr.rq_cookie_id());
		try {
			RemoteSrvUnit rsu_ns = nf->get_by_sockaddr(sa_rq);
			if (0==memcmp(&rsu.get_remote_sockaddr(), &rsu_ns.get_remote_sockaddr(), sizeof(sockaddr_in))) {
				remote_status = cookie_status_actual; //Параметры сервера не изменились. С сервером все в порядке
			} else {
				remote_status = cookie_status_exists; //Такой сервер был зарегистрирован, но по другому адресу
			}
		} catch (...) {
			remote_status = cookie_status_newbie;   //Странно! Сервер уже был найден раньше, но почему-то изменил свой адрес. Найти его в списках не удалось
		}
	} catch (...) {
		try {
			RemoteSrvUnit rsu_ns = nf->get_by_sockaddr(sa_rq);
			remote_status = cookie_status_modified; //Сервер зарегистрировался по адресу, ранее принадлежавшему другому приложению
		} catch (...) {
			remote_status = cookie_status_newbie;   //Новый сервер зарегистрировался по новому адресу. Он совсем новый
		}
		nf->add_discovered_server(sa_rq, mlr.rq_cookie_id());
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

	try {
		RemoteSrvUnit rsu = nf->get_by_uniq_id(mlrs.rp_cookie_id());
		//Сервер с таким идентификатором уже встречался ранее
		IRemoteNfServer* irnfs = rsu.irnfs_ptr();
		if (irnfs->validate_addr(sa_rq)) {
			irnfs->add_ping_response(wrpr->msg_id());
		} else {
			cout << "NetFindLinkHandler::handle_response warning - remote address changed for " << mlrs.rp_cookie_id() << endl;
			cout << "Old value " << inet_ntoa(rsu.get_remote_sockaddr().sin_addr) << ":" << rsu.get_remote_sockaddr().sin_port << ", new value " << inet_ntoa(sa_rq.sin_addr) <<  ":" << sa_rq.sin_port<< endl;
		}
	} catch (...) {
		nf->add_discovered_server(sa_rq, mlrs.rp_cookie_id()); //Такой сервер ранее не вустречался. Добавляем его
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
	if (NULL == remote_server) return false;

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
	if (NULL == nf) {
		cout << "NetFindInfoHandler::NetFindInfoHandler error - NULL pointer to NetFind" << endl;
		return;
	}
	this->nf = nf;

	pthread_mutex_init (&mtx, NULL);
	nmsg_index = 0; //Счетчик идентификаторов сообщения

	rq_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (rq_socket < 0) {
		cout << "NetFindInfoHandler::NetFindInfoHandler error - couldn`t open socket for sending requests" << endl;
		perror("socket");
	}
}

NetFindInfoHandler::~NetFindInfoHandler() {
	close(rq_socket);
}

int NetFindInfoHandler::get_msg_id() {
	pthread_mutex_lock(&mtx);
	int n = nmsg_index++;
	pthread_mutex_unlock(&mtx);
	return n;
}

bool NetFindInfoHandler::HandleMessage(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr) {
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

bool NetFindInfoHandler::InvokeRequest(IRemoteNfServer *remote_server) {
	if (NULL==remote_server || remote_server->is_broadcast()) return false;

	msg_info_rq mir;
	mir.set_rq_cookie_id(nf->get_uniq_id());
	mir.set_rp_port(nf->get_client_port());
	string mir_str = "";
	mir.SerializeToString(&mir_str);
	msg_wrapper wrpr;

	//Формируем пакет-обертку над сообщением
	int rq_id = get_msg_id();
	wrpr.set_msg_id(rq_id);
	wrpr.set_msg_type(msg_wrapper_msg_type_info);
	wrpr.set_body(mir_str);
	char *wrpr_data = new char[wrpr.ByteSize()];
	if (!wrpr.SerializeToArray(wrpr_data, wrpr.ByteSize())) {
		cout << "NetFindInfoHandler::InvokeRequest error - couldn`t serialize message wrapper" << endl;
		delete[] wrpr_data;
		return false;
	}
	if (sendto(rq_socket, wrpr_data, wrpr.ByteSize(),
			0,
			(sockaddr *)&(remote_server->get_remote_sockaddr()),
			sizeof(sockaddr_in)) < 0) {
		cout << "NetFindInfoHandler::InvokeRequest error - couldn`t send request" << endl;
		perror("sendto");
	}
	delete[] wrpr_data;
	return true;
}

bool NetFindInfoHandler::handle_request(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr) {
	msg_info_rq mir;
	if (!mir.ParseFromString(wrpr->body())) {
		cout << "NetFindInfoHandler::handle_request error - failed to parse message" << endl;
		return false;
	}

	msg_info_rsp mirs;
	mirs.set_rq_cookie_id(mir.rq_cookie_id());
	mirs.set_rp_cookie_id(nf->get_uniq_id());
	mirs.set_caption(nf->get_caption());
	mirs.set_name(nf->get_name());
	mirs.set_cluster(nf->get_cluster());

	//Заполняем список каналов, поддерживаемых этим экземпляром библиотеки
	for (list<stream_full_cfg>::const_iterator it=nf->streams_begin();it!=nf->streams_end();it++) {
		p2v_channel *p2vc = mirs.add_channels();
		p2vc->set_name(it->stream_cfg.name);
		p2vc->set_port(it->port);
		switch (it->stream_cfg.type) {
			case stream_type_dgram:
				p2vc->set_ch_mode(p2v_channel_channel_mode_m2m);
				p2vc->set_ch_integrity(p2v_channel_channel_integrity_any);
				break;
			case stream_type_flow:
				p2vc->set_ch_mode(p2v_channel_channel_mode_m2m);
				p2vc->set_ch_integrity(p2v_channel_channel_integrity_complete);
				break;
		}
		switch (it->stream_cfg.direction) {
				case stream_direction_income:
					p2vc->set_ep_mode(p2v_channel_endpoint_mode_receiver);
					break;
				case stream_direction_outcome:
					p2vc->set_ep_mode(p2v_channel_endpoint_mode_transmitter);
					break;
				case stream_direction_bidir:
					p2vc->set_ep_mode(p2v_channel_endpoint_mode_tranceiver);
					break;
				case stream_direction_loopback:
					continue; //FIXME Нельзя добавлять поле, прежде чем оно не будет проверено на адекватность
					break;
		}
	}

	string mirs_str = "";
	if (!mirs.SerializeToString(&mirs_str)) {
		cout << "NetFindInfoHandler::handle_request error - couldn`t serialize info to string" << endl;
		return false;
	}

	msg_wrapper nwrpr;
	nwrpr.set_msg_id(wrpr->msg_id());
	nwrpr.set_msg_type(msg_wrapper_msg_type_info);
	nwrpr.set_dir(msg_wrapper_msg_direction_response);
	nwrpr.set_body(mirs_str);
	char *nwrpr_data = new char[nwrpr.ByteSize()];
	if (!nwrpr.SerializeToArray(nwrpr_data, nwrpr.ByteSize())) {
		cout << "NetFindInfoHandler::handle_request error - couldn`t serialize message wrapper" << endl;
		delete[] nwrpr_data;
		return false;
	}
	sockaddr_in response_addr;
	memset(&response_addr, 0, sizeof(sockaddr_in));
	response_addr.sin_family = AF_INET;
	response_addr.sin_addr.s_addr = remote_addr->sin_addr.s_addr;
	response_addr.sin_port = htons(mir.rp_port());
	if (sendto(rq_socket, nwrpr_data, nwrpr.ByteSize(), 0, (sockaddr*)&response_addr, sizeof(sockaddr_in)) < 0) {
		cout << "NetFindInfoHandler::handle_request error - couldn`t send response" << endl;
		perror("sendto");
	}
	delete[] nwrpr_data;
	return true;
}

bool NetFindInfoHandler::handle_response(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr) {
	msg_info_rsp mirs;
	if (!mirs.ParseFromString(wrpr->body())) {
		cout << "NetFindInfoHandler::handle_response error - failed to parse message" << endl;
		return false;
	}

	try {
		RemoteSrvUnit rsu = nf->get_by_uniq_id(mirs.rp_cookie_id());
		RemoteNfServer* rnfs = reinterpret_cast<RemoteNfServer*>(rsu.irnfs_ptr());
		if (NULL == rnfs) { //Сервер с таким идентификатором не найден.
			return false;
		}
		if (rnfs->is_self()) { //Пришел ответ от этого экземляра программы. Соединяться с ним нет смысла
			return true;
		}

		net_find_config nfc;
		if (mirs.has_caption()) {
			nfc.nf_caption = mirs.caption();
		} else {
			nfc.nf_caption = "";
		}
		if (mirs.has_name()) {
			nfc.nf_name = mirs.name();
		} else {
			nfc.nf_name = "";
		}
		if (mirs.has_cluster()) {
			nfc.nf_cluster = mirs.cluster();
		} else {
			nfc.nf_name = "";
		}
		rnfs->update_info(&nfc);
		for (int i=0;i<mirs.channels_size();i++) {
			const p2v_channel& p2vc = mirs.channels(i);
			if (p2v_channel_endpoint_mode_transmitter == p2vc.ep_mode()) continue; //Удаленный абонент является источником сообщений и сам их не принимает. Регистрировать его не надо. При необходимости он сам к нам обратится
			remote_endpoint re;
			re.remote_port = p2vc.port();
			switch (p2vc.ch_integrity()) { //В зависимости от заявленной надежности каналу устанавливаем способ доставки данных до конечной точки
				case p2v_channel_channel_integrity_any:
					re.str_type = stream_type_dgram;
					break;
				default:
					re.str_type = stream_type_flow;
			}
			re.rsu = rsu;
			nf->register_remote_endpoint(p2vc.name(), re);
			//cout << "Channel: " << p2vc.name() << "; port: " << p2vc.port() << endl;
		}
	} catch (...) {
		cout << "NetFindInfoHandler::handle_response error - unknown exception occured" << endl;
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////


NetFindListHandler::NetFindListHandler(NetFind* nf, int msg_id) {
	if (NULL == nf) {
		cout << "NetFindInfoHandler::NetFindInfoHandler error - NULL pointer to NetFind" << endl;
		return;
	}
	this->nf = nf;
	gettimeofday(&tv_prev, NULL);

	pthread_mutex_init (&mtx, NULL);
	nmsg_index = 100; //Счетчик идентификаторов сообщения
	                  //Сообщения-ответы, имеющие идентификаторы с меньшими адресами считаются ответами
	                  //от сервера. По приходу первых MIN_FULL_LIST_MESSAGES сообщений считается, что основной список
	                  //серверов уже построен
	full_list_msg_count = 0; //Количество ответных сообщений на запросы о списке доступных локальных серверов

	rq_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (rq_socket < 0) {
		cout << "NetFindInfoHandler::NetFindInfoHandler error - couldn`t open socket for sending requests" << endl;
		perror("socket");
	}
}

NetFindListHandler::~NetFindListHandler() {
	close(rq_socket);
}

int NetFindListHandler::get_msg_id() {
	pthread_mutex_lock(&mtx);
	int n = nmsg_index++;
	pthread_mutex_unlock(&mtx);
	return n;
}

bool NetFindListHandler::HandleMessage(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr) {
	switch (wrpr->dir()) {
		case msg_wrapper_msg_direction_request:
			handle_request(wrpr,remote_addr);
			break;
		case msg_wrapper_msg_direction_response:
			handle_response(wrpr,remote_addr);
			break;
		default:
			cout << "NetFindListHandlerq::server_response_thread warning - unknown message direction" << endl;
			break;
	}
	return true;
}

bool NetFindListHandler::InvokeRequest(IRemoteNfServer *remote_server) {
	if (NULL==remote_server || remote_server->is_broadcast()) return false;

	msg_srvlist_rq mslr;
	mslr.set_rp_port(nf->get_client_port());
	string mslr_str = "";
	mslr.SerializeToString(&mslr_str);
	msg_wrapper wrpr;

	//Формируем пакет-обертку над сообщением
	int rq_id = get_msg_id();
	wrpr.set_msg_id(rq_id);
	wrpr.set_msg_type(msg_wrapper_msg_type_list);
	wrpr.set_body(mslr_str);
	char *wrpr_data = new char[wrpr.ByteSize()];
	if (!wrpr.SerializeToArray(wrpr_data, wrpr.ByteSize())) {
		cout << "NetFindListHandler::InvokeRequest error - couldn`t serialize message wrapper" << endl;
		delete[] wrpr_data;
		return false;
	}
	if (sendto(rq_socket, wrpr_data, wrpr.ByteSize(),
			0,
			(sockaddr *)&(remote_server->get_remote_sockaddr()),
			sizeof(sockaddr_in)) < 0) {
		cout << "NetFindListHandler::InvokeRequest error - couldn`t send request" << endl;
		perror("sendto");
	}
	delete[] wrpr_data;
	return true;
}

bool NetFindListHandler::RequestLocalhost() {
	msg_srvlist_rq mslr;
	mslr.set_rp_port(nf->get_client_port());
	mslr.set_list_non_localhost(true); //Запрашиваем все доступные сервера, в т.ч. расположенные на других машинах
	string mslr_str = "";
	mslr.SerializeToString(&mslr_str);
	msg_wrapper wrpr;

	//Формируем пакет-обертку над сообщением
	wrpr.set_msg_id(0);
	wrpr.set_msg_type(msg_wrapper_msg_type_list);
	wrpr.set_body(mslr_str);
	char *wrpr_data = new char[wrpr.ByteSize()];
	if (!wrpr.SerializeToArray(wrpr_data, wrpr.ByteSize())) {
		cout << "NetFindListHandler::RequestLocalhost error - couldn`t serialize message wrapper" << endl;
		delete[] wrpr_data;
		return false;
	}
	sockaddr_in response_addr;
	memset(&response_addr, 0, sizeof(sockaddr_in));
	response_addr.sin_family = AF_INET;
	response_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	response_addr.sin_port = htons(nf->get_server_port());
	if (sendto(rq_socket, wrpr_data, wrpr.ByteSize(),
			0,
			(sockaddr *)&(response_addr),
			sizeof(sockaddr_in)) < 0) {
		cout << "NetFindListHandler::RequestLocalhost error - couldn`t send request" << endl;
		perror("sendto");
	}
	delete[] wrpr_data;
	return true;
}

bool NetFindListHandler::requires_server_list() {
	if (nf->is_server()) return false;

	if (full_list_msg_count<MIN_FULL_LIST_MESSAGES) {
		gettimeofday(&tv_prev, NULL);
		return true;
	}

	timeval tv_now;
	gettimeofday(&tv_now, NULL);
	unsigned  int delta = ((1000000-tv_prev.tv_usec)+(tv_now.tv_usec-1000000)+(tv_now.tv_sec-tv_prev.tv_sec)*1000000) / 1000;
	if (delta > MIN_LIST_RQ_TIME) {
		memcpy(&tv_prev, &tv_now, sizeof(timeval));
		return true;
	}
	return false;
}

void NetFindListHandler::server_chanded() {
	full_list_msg_count = 0;
}

bool NetFindListHandler::handle_request(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr) {
	msg_srvlist_rq mslr;
	if (!mslr.ParseFromString(wrpr->body())) {
		cout << "NetFindListHandler::handle_request error - failed to parse message" << endl;
		return false;
	}

	bool list_non_localhost = false;
	if (mslr.has_list_non_localhost() && mslr.list_non_localhost()) {
		list_non_localhost = true;
	}

	list<RemoteSrvUnit> srv_list;
	list<RemoteSrvUnit>::iterator it;
	nf->get_alive_servers(srv_list);
	msg_srvlist_rsp mslrs;
	for (it=srv_list.begin();it!=srv_list.end();it++) {
		RemoteSrvUnit& rsu = *it;
		if (rsu.is_broadcast()) continue;  //Вещательные сервера не отправляются, т.к. не могут быть действующими по определению
		if (!rsu.is_localhost() && !list_non_localhost) continue; //По умолчанию отправляются только адреса локальных машин.
		                                                             //адреса всех остальных машин отправляются только по специальному запросу

		msg_srvlist_rsp_srv_addr* addr = mslrs.add_srv_addrs();
		ostringstream ostr;
		sockaddr_in sa = rsu.get_remote_sockaddr();
		ostr << htons(sa.sin_port);
		addr->set_port(ostr.str());
		ostr.str("");
		unsigned int ip_addr = htonl(sa.sin_addr.s_addr);
		ostr << ((ip_addr&0xFF000000)>>24) << "." << ((ip_addr&0x00FF0000)>>16) << "." << ((ip_addr&0x0000FF00)>>8) << "." << (ip_addr&0x000000FF);
		addr->set_addr(ostr.str());
		addr->set_uniq_id(rsu.get_uniq_id());
	}


	string mslrs_str = "";
	mslrs.SerializeToString(&mslrs_str);

	msg_wrapper nwrpr;
	nwrpr.set_msg_id(wrpr->msg_id());
	nwrpr.set_msg_type(msg_wrapper_msg_type_list);
	nwrpr.set_dir(msg_wrapper_msg_direction_response);
	nwrpr.set_body(mslrs_str);
	char *nwrpr_data = new char[nwrpr.ByteSize()];
	if (!nwrpr.SerializeToArray(nwrpr_data, nwrpr.ByteSize())) {
		cout << "NetFindListHandler::handle_request error - couldn`t serialize message wrapper" << endl;
		delete[] nwrpr_data;
		return false;
	}
	sockaddr_in response_addr;
	memset(&response_addr, 0, sizeof(sockaddr_in));
	response_addr.sin_family = AF_INET;
	response_addr.sin_addr.s_addr = remote_addr->sin_addr.s_addr;
	response_addr.sin_port = htons(mslr.rp_port());
	if (sendto(rq_socket, nwrpr_data, nwrpr.ByteSize(), 0, (sockaddr*)&response_addr, sizeof(sockaddr_in)) < 0) {
		cout << "NetFindListHandler::handle_request error - couldn`t send response" << endl;
		perror("sendto");
	}
	delete[] nwrpr_data;

	srv_list.clear();
	return true;
}

bool NetFindListHandler::handle_response(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr) {
	msg_srvlist_rsp mslrs;
	if (!mslrs.ParseFromString(wrpr->body())) {
		cout << "NetFindListHandler::handle_response error - failed to parse message" << endl;
		return false;
	}

	if (wrpr->msg_id() < 100) {
		full_list_msg_count++; //Этот ответ пришел в ответ на запрос полного списка серверов, доступных на локальномм хосте
	}

	for (int i=0;i<mslrs.srv_addrs_size();i++) {
		const msg_srvlist_rsp_srv_addr& srv_addr = mslrs.srv_addrs(i);

		sockaddr_in sa;
		memset(&sa,0, sizeof(sockaddr_in));
		sa.sin_family = AF_INET;
		if (0 == inet_aton(srv_addr.addr().c_str(), &(sa.sin_addr))) {
			cout << "NetFindListHandler::handle_response error: wrong server address" << endl;
			continue;
		}
		sa.sin_port = htons(atoi(srv_addr.port().c_str()));
		nf->add_discovered_server(sa,srv_addr.uniq_id());
	}

	return true;
}






