/*
 * net_find.cpp
 *
 *  Created on: 14.05.2012
 *      Author: drozdov
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <ifaddrs.h>
#include <sys/poll.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "net_find.h"
#include "nf_remote_server.h"
#include "nf_bkst_server.h"
#include "net_find_ifaces.h"
#include "net_find_handlers.h"
#include "msg_wrapper.pb.h"
#include "msg_netfind.pb.h"

#include "tcpstream.h"

#include "mtx_containers.h"
#include "mtx_containers.hpp"

#define MAX_RSP_RCV_BUFLEN 65536
#define MIN_REVIEW_PERIOD 330

using namespace std;
using namespace p2vera;
using namespace netfind;

#ifdef DBG_PRINT
#define _D(str) str
#else
#define _D(str) {}
#endif


_rmt_ping& _rmt_ping::operator=(const _rmt_ping& rmtp) {
	ping_id = rmtp.ping_id;
	memcpy(&ping_send_time, &rmtp.ping_send_time, sizeof(timeval));
	return *this;
}

_rmt_ping::_rmt_ping(const _rmt_ping& rmtp) {
	ping_id = rmtp.ping_id;
	memcpy(&ping_send_time, &rmtp.ping_send_time, sizeof(timeval));
}

_rmt_ping::_rmt_ping() {
	ping_id = 0;
	memset(&ping_send_time,0,sizeof(timeval));
}

NetFind::NetFind(net_find_config *nfc) {
	name = nfc->nf_name;
	caption = nfc->nf_caption;
	hash = nfc->nf_hash;
	cluster = nfc->nf_cluster;

	last_remote_id = 0; //Идентификатор последнего найденного сервера

	load_ifinfo(); //Получаем информацию о локальных сетевых адресах

	//Создаем мутексы
	pthread_mutex_init (&mtx, NULL);

	server_port = atoi(nfc->nf_port.c_str());
	memset(&ownaddr, 0x00,sizeof(sockaddr_in));
	ownaddr.sin_family = AF_INET;
	ownaddr.sin_port = htons((unsigned short)server_port);
	ownaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//Создаем обработчики сообщений
	link_handler = new NetFindLinkHandler(this, 0);
	info_handler = new NetFindInfoHandler(this, 1);
	list_handler = new NetFindListHandler(this, 2);
	msg_handlers[0] = link_handler;
	msg_handlers[1] = info_handler;
	msg_handlers[2] = list_handler;

	tcm = new TcpStreamManager(this, 8000);

	//Создаем потоки
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, &nf_server_rsp_thread_fcn, this); //Поток, выполняющий роль сервера
	pthread_create(&thread_id, NULL, &nf_client_thread_fcn, this); //Поток, выполняющий роль сервера
}

void NetFind::load_ifinfo() {
	struct ifaddrs *ifa_ptr, *ifa_ptr_tmp;

	if (getifaddrs(&ifa_ptr)!=0) {
		perror("ERROR getifaddrs: ");
		return;
	}
	for (ifa_ptr_tmp=ifa_ptr; ifa_ptr_tmp->ifa_next!=NULL; ifa_ptr_tmp=ifa_ptr_tmp->ifa_next) {
		if (ifa_ptr_tmp->ifa_addr->sa_family!=AF_INET)
			continue;
		local_ips.push_back(*((struct sockaddr_in *)(ifa_ptr_tmp->ifa_addr)));
	}
	freeifaddrs(ifa_ptr);
}

bool NetFind::is_localhost(sockaddr_in& sa) {
	vector<sockaddr_in>::iterator it;
	for (it=local_ips.begin();it!=local_ips.end();it++) {
		if (it->sin_addr.s_addr==sa.sin_addr.s_addr) {
			return true;
		}
	}
	return false;
}

int NetFind::register_dgram_stream(_stream_config& stream_cfg, IP2VeraStreamHub* sh) {
	pthread_mutex_lock(&mtx);
	if (streams.end() != find_stream(stream_cfg.name)) {
		cout << "NetFind::register_dgram_stream error - stream " << stream_cfg.name << " was already registered" << endl;
		pthread_mutex_unlock(&mtx);
		throw;
	}
	stream_full_cfg sfc;
	sfc.stream_cfg.name = stream_cfg.name;
	sfc.stream_cfg.direction = stream_cfg.direction;
	sfc.stream_cfg.order = stream_cfg.order;
	sfc.stream_cfg.type = stream_cfg.type;

	sfc.sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&sfc.local_sa, 0, sizeof(sockaddr_in));
	sfc.local_sa.sin_family = AF_INET;
	sfc.local_sa.sin_addr.s_addr = htonl(INADDR_ANY);

	_D(cout << "NetFind::register_dgram_stream info - поиск свободного порта для " << stream_cfg.name << endl);
	int rc = -1;
	sfc.port =  server_port + 10;
	while (rc < 0) {
		usleep(1000);
		sfc.port++;
		if (sfc.port >= 65535) {
			cout << "NetFind::register_dgram_stream error - couldn`t find free port" << endl;
			return 0;
		}
		sfc.local_sa.sin_port = htons((unsigned short)sfc.port);
		rc = bind(sfc.sock_fd, (struct sockaddr *)&sfc.local_sa, sizeof(sockaddr_in));
	}
	_D(cout << "NetFind::register_dgram_stream info - binded stream " << stream_cfg.name << " at port " << sfc.port << endl);

	sfc.sh = sh;
	streams.push_back(sfc);
	pthread_mutex_unlock(&mtx);
	return sfc.sock_fd;
}

int NetFind::register_flow_stream(_stream_config& stream_cfg, IP2VeraStreamHub* sh) {
	pthread_mutex_lock(&mtx);
	if (streams.end() != find_stream(stream_cfg.name)) {
		cout << "NetFind::register_flow_stream error - stream " << stream_cfg.name << " was already registered" << endl;
		pthread_mutex_unlock(&mtx);
		throw;
	}
	stream_full_cfg sfc;
	sfc.stream_cfg.name = stream_cfg.name;
	sfc.stream_cfg.direction = stream_cfg.direction;
	sfc.stream_cfg.order = stream_cfg.order;
	sfc.stream_cfg.type = stream_cfg.type;

	sfc.sock_fd = tcm->get_fd();
	sfc.port = tcm->get_port();

	sfc.sh = sh;
	streams.push_back(sfc);
	pthread_mutex_unlock(&mtx);
	return 0;
}

int NetFind::register_stream(_stream_config& stream_cfg, IP2VeraStreamHub* sh) {
	if (NULL == sh) {
		cout << "NetFind::register_stream error - null pointer to IP2VeraStreamHub" << endl;
		return 0;
	}

	switch(stream_cfg.type) {
		case stream_type_dgram:
			return register_dgram_stream(stream_cfg, sh);
			break;
		case stream_type_flow:
			return register_flow_stream(stream_cfg, sh);
			break;
		default:
			cout << "NetFind::register_stream error - unsupported stream type" << endl;
	}
	return 0;
}

void NetFind::register_remote_endpoint(std::string stream_name, remote_endpoint& re) {
	if (stream_type_dgram == re.str_type) {
		_D(cout << "NetFind::register_remote_endpoint info - requested to register dgram stream " << stream_name << endl);
		pthread_mutex_lock(&mtx);
		list<stream_full_cfg>::iterator it = find_stream(stream_name);
		if (streams.end() == it) { //Такой поток не зарегистрирован.
			pthread_mutex_unlock(&mtx);
			_D(cout << "NetFind::register_remote_endpoint info - stream " << stream_name << " is undefined" << endl);
			return;
		}
		stream_full_cfg& sfc = *it;

		//Ищем, переданный endpoint среди ранее зарегистрированных
		for (list<remote_endpoint>::iterator re_it=sfc.remote_endpoints.begin();re_it!=sfc.remote_endpoints.end();re_it++) {
			if (re_it->rsu == re.rsu) { //Такой сервер уже обработан и зарегистрирован
				pthread_mutex_unlock(&mtx);
				_D(cout << "NetFind::register_remote_endpoint info - stream " << stream_name << " was allready aasociated with hub" << endl);
				return;
			}
		}
		sfc.remote_endpoints.push_back(re);
		if (NULL == sfc.sh) {
			cout << "NetFind::register_remote_endpoint error - zero pointer to stream hub" << endl;
			pthread_mutex_unlock(&mtx);
			_D(cout << "NetFind::register_remote_endpoint info - stream " << stream_name << " is undefined" << endl);
			return;
		}
		sfc.sh->add_message_target(re.rsu, re.remote_port);
		pthread_mutex_unlock(&mtx);
	} else if (stream_type_flow == re.str_type) {
		_D(cout << "NetFind::register_remote_endpoint info - requested to register dgram stream " << stream_name << endl);
		pthread_mutex_lock(&mtx);
		list<stream_full_cfg>::iterator it = find_stream(stream_name);
		if (streams.end() == it) { //Такой поток не зарегистрирован.
			pthread_mutex_unlock(&mtx);
			return;
		}

		tcm->add_server(re.rsu, re.remote_port);  //Добавляем вновь найденный сервер (при его отсутствии)
		TcpStream tc = tcm->find_stream(re.rsu); //Ищем вновь созданный поток
		//FIXME Сервер может оказаться недоступным. Проверять возвращенное значение
		tc.add_hub(it->sh); //Добавляем хаб с указанным именем к списку хабов, связанных с этим сервером
		pthread_mutex_unlock(&mtx);
	}
}

void NetFind::unlink_server_stream(IRemoteNfServer* irnfs) {
	//Просматриваем все зарегистрированные потоки
	for (list<stream_full_cfg>::iterator it_sfc=streams.begin();it_sfc!=streams.end();it_sfc++) {
		stream_full_cfg& sfc = *it_sfc;

		//В каждом зарегистрированном потоке ищем удаляемый сервер
		for (list<remote_endpoint>::iterator re_it=sfc.remote_endpoints.begin();re_it!=sfc.remote_endpoints.end();) {
			if (re_it->rsu != irnfs) {
				++re_it;
				continue;
			}
			sfc.sh->remove_message_target(re_it->rsu);
			re_it = sfc.remote_endpoints.erase(re_it);
		}
	}
}

std::list<stream_full_cfg>::const_iterator NetFind::streams_begin() {
	return streams.begin();
}

std::list<stream_full_cfg>::const_iterator NetFind::streams_end() {
	return streams.end();
}

std::list<stream_full_cfg>::iterator NetFind::find_stream(std::string name) {
	for (std::list<stream_full_cfg>::iterator it=streams.begin();it!=streams.end();it++) {
		if (it->stream_cfg.name == name) return it;
	}
	return streams.end();
}

IP2VeraStreamHub* NetFind::find_stream_hub(std::string stream_name) {
	std::list<stream_full_cfg>::iterator it = find_stream(stream_name);
	if (streams.end()==it) return NULL;
	return it->sh;
}


//Позволяет проверить текущий режим этой копии библиотеки
bool NetFind::is_server() {
	return is_local_server;
}

int NetFind::generate_remote_id() {
	pthread_mutex_lock(&mtx);
	int n = last_remote_id++;
	pthread_mutex_unlock(&mtx);
	return n;
}

int NetFind::add_discovered_server(sockaddr_in& addr, std::string uniq_id) {
	bool b_localhost = is_localhost(addr);
	if (b_localhost &&  htons(addr.sin_port)==get_server_port()) {
		return -1; //Это локальный сервер. Как он оказался среди всего этого непонятно, но добавлять его не стоит
	}
	int remote_id = generate_remote_id();

	pthread_mutex_lock(&mtx);
	map<std::string, IRemoteNfServer*>::iterator it = m_str_servers.find(uniq_id);
	if (it != m_str_servers.end()) {
		pthread_mutex_unlock(&mtx);
		return -1;
	}

	net_find_config nfc;
	nfc.nf_caption = "unknown";
	nfc.nf_name = "unknown";
	nfc.nf_hash = uniq_id;
	nfc.scanable = true;
	nfc.usage = 0;

	RemoteNfServer* rnfs = new RemoteNfServer(remote_id, &nfc, addr);
	if (b_localhost) { //Для серверов, работающих на локальной машине добавляем все адреса, с которых могут приходить ответы
		rnfs->is_localhost(true);
		if (uniq_id == get_uniq_id()) {
			rnfs->is_self(true);
		}
		vector<sockaddr_in>::iterator it;
		for (it=local_ips.begin();it!=local_ips.end();it++) {
			rnfs->add_alternate_addr(*it);
		}
	}

	remote_servers.push_back(rnfs);
	m_str_servers[uniq_id] = rnfs;
	reg_to_sockaddr(addr, rnfs);
	pthread_mutex_unlock(&mtx);

	if (rnfs->requires_info_request()) { //FIXME Возможны гонки при добавлении серверов из нескольких потоков
		info_list.push_back(rnfs);
	}

	cout << "NetFind::add_discovered_server info - server registered for " << uniq_id << endl;

	return remote_id;
}

int NetFind::add_scanable_server(std::string address, std::string port) {
	net_find_config nfc;
	nfc.nf_address = address;
	nfc.nf_port = port;
	nfc.nf_caption = "unknown";
	nfc.nf_name = "unknown";
	nfc.nf_hash = "";
	nfc.scanable = true;
	nfc.usage = 0;

	int remote_id = generate_remote_id();
	RemoteNfServer* rnfs = new RemoteNfServer(remote_id, &nfc);
	pthread_mutex_lock(&mtx);
	remote_servers.push_back(rnfs);
	pthread_mutex_unlock(&mtx);

	return remote_id;
}

int NetFind::add_broadcast_servers(std::string port) {
	net_find_config nfc;
	nfc.nf_port = port;

	int remote_id = generate_remote_id();
	BkstNfServer* bnfs = new BkstNfServer(remote_id, &nfc);
	pthread_mutex_lock(&mtx);
	remote_servers.push_back(bnfs);
	pthread_mutex_unlock(&mtx);
	return 0;
}

void NetFind::reg_to_sockaddr(sockaddr_in& sa, IRemoteNfServer* irnfs) {
	unsigned long long lsa = ((unsigned long long)sa.sin_port << 32) | ((unsigned int)sa.sin_addr.s_addr);
	map<unsigned long long, IRemoteNfServer*>::iterator it = m_sa_servers.find(lsa);
	if (it!=m_sa_servers.end()) {
		//Сервер с таким сочетанием ip--слушающий-порт уже был зарегистрирован ранее
		//это означает, что он завершил свою работу. Теперь его необходимо пометить, как
		//неактивный
		cout << "NetFind::reg_to_sockaddr info - server at " << inet_ntoa(sa.sin_addr) << ":" << htons(sa.sin_port) << " was replaced by " << irnfs->get_uniq_id() << endl;
	}
	lsa = ((unsigned long long)sa.sin_port << 32) | ((unsigned int)sa.sin_addr.s_addr);
	m_sa_servers[lsa] = irnfs;
}

//Удаление сервера из списка. Сервер может быть снова найден и получит новый id
void NetFind::remove_remote_server(int id) {
}
//Вывод в консоль списка известных серверов с их статусами
void NetFind::print_servers() {
	pthread_mutex_lock(&mtx);
	list<IRemoteNfServer*>::iterator it;
	int count = 0;
	for (it=remote_servers.begin();it!=remote_servers.end();it++) {
		IRemoteNfServer* irnfs = *it;
		if (NULL == irnfs) continue;
		irnfs->print_info();
		cout << endl;
		count++;
	}
	cout << "total: " << count << endl;
	pthread_mutex_unlock(&mtx);
}

void NetFind::get_alive_servers(std::list<RemoteSrvUnit>& srv_list) {
	srv_list.clear();
	pthread_mutex_lock(&mtx);
	list<IRemoteNfServer*>::iterator it;
	for (it=remote_servers.begin();it!=remote_servers.end();it++) {
		IRemoteNfServer* irnfs = *it;
		if (NULL == irnfs || irnfs->is_broadcast()) continue;
		RemoteSrvUnit rs(irnfs);
		srv_list.push_back(rs);
	}
	pthread_mutex_unlock(&mtx);
}

RemoteSrvUnit NetFind::get_by_sockaddr(sockaddr_in& sa) {
	pthread_mutex_lock(&mtx);
	unsigned long long lsa = ((unsigned long long)sa.sin_port << 32) | ((unsigned int)sa.sin_addr.s_addr);
	map<unsigned long long, IRemoteNfServer*>::iterator it = m_sa_servers.find(lsa);
	if (it == m_sa_servers.end() || NULL==it->second) {
		pthread_mutex_unlock(&mtx);
		throw server_not_found();
	}
	IRemoteNfServer* irnfs = it->second;
	RemoteSrvUnit rs(irnfs);
	pthread_mutex_unlock(&mtx);
	return rs;
}

RemoteSrvUnit NetFind::get_by_uniq_id(std::string uniq_id) {
	pthread_mutex_lock(&mtx);
	map<std::string, IRemoteNfServer*>::iterator it = m_str_servers.find(uniq_id);
	if (it == m_str_servers.end() || NULL==it->second) {
		pthread_mutex_unlock(&mtx);
		throw server_not_found();
	}
	IRemoteNfServer* irnfs = it->second;
	RemoteSrvUnit rs(irnfs);
	pthread_mutex_unlock(&mtx);
	return rs;
}

unsigned int NetFind::get_server_port() {
	return server_port;
}

unsigned int NetFind::get_client_port() {
	return client_port;
}

std::string NetFind::get_uniq_id() {
	return hash;
}

std::string NetFind::get_name() {
	return name;
}

std::string NetFind::get_caption() {
	return caption;
}

std::string NetFind::get_cluster() {
	return cluster;
}

//Поток сервера. Формирует ответы на запросы клиентов
int NetFind::server_response_thread() {
	is_local_server = false;
	while (true) {
		//Пытаемся открыть слушающий UDP сокет.
		sock_srv_rsp = socket(AF_INET, SOCK_DGRAM, 0);
		int rc = -1;
		cout << "NetFind::server_response_thread info - запуск в режиме ведомого" << endl;
		while (rc < 0) {
			rc = bind(sock_srv_rsp, (struct sockaddr *)&ownaddr, sizeof(sockaddr_in));
			usleep(10000);
		}

		//Сокет открылся. Значит, мы стали сервером. Переходим в основной цикл сервера
		cout << "NetFind::server_response_thread info - переход в режим ведущего" << endl;
		is_local_server = true;
		while (receive_udp_message(sock_srv_rsp));
		is_local_server = false;
	}
	return 0;
}
//Поток сервера. Обеспечивает построение маршрутов (пока не уверен)
int NetFind::server_route_thread() {
	return 0;
}

//Открытие слушающего клиентского сокета
bool NetFind::bind_client_port() {
	sock_clt_rsp = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&clientaddr, 0, sizeof(sockaddr_in));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	_D(cout << "NetFind::client_thread info - поиск свободного порта..." << endl);
	int rc = -1;
	client_port = server_port;
	while (rc < 0) {
		usleep(1000);
		client_port++;
		if (client_port >= 65535) {
			cout << "NetFind::client_thread error - couldn`t find free port" << endl;
			return false;
		}
		clientaddr.sin_port = htons((unsigned short)client_port);
		rc = bind(sock_clt_rsp, (struct sockaddr *)&clientaddr, sizeof(sockaddr_in));
	}
	return true;
}

//Извлекает одно входящее udp сообщение и разбирает его
bool NetFind::receive_udp_message(int socket) {
	unsigned char rcv_buf[MAX_RSP_RCV_BUFLEN] = {0};
	socklen_t remote_addrlen = sizeof(sockaddr_in);
	sockaddr_in remote_addr;
	int rc = recvfrom(socket, rcv_buf, MAX_RSP_RCV_BUFLEN, 0, (sockaddr*)&remote_addr, &remote_addrlen);
	if(rc < 0) {
		cout << "NetFind::receive_udp_message error - закрылся слушающий сокет клиента поиска сети." << endl;
		close(sock_clt_rsp);
		return false;
	}
	msg_wrapper wrpr;
	if (!wrpr.ParseFromArray(rcv_buf, rc)) {
		cout << "NetFind::receive_udp_message error - couldn`t parse wrapper" << endl;
		cout << "Trying to parse it partially..." << endl;
		if (!wrpr.ParsePartialFromArray(rcv_buf, rc)) {
			cout << "completely failed" << endl;
			cout << "Dumping message:" << endl;
			cout << "Message size: " << rc << endl;
			cout << "Message data:" << endl;
			for (int i=0;i<rc;i++) {
				cout << hex << " 0x" << (int)rcv_buf[i];
			}
			cout << dec << endl;
			return true;
		}
		return true;
	}
	if (!wrpr.has_body()) {
		cout << "NetFind::receive_udp_message warning - request body field is empty" << endl;
		return true;
	}
	int nmsgid = wrpr.msg_type();
	std::map<int, INetFindMsgHandler*>::iterator it = msg_handlers.find(nmsgid);
	if (it == msg_handlers.end()) {
		cout << "NetFind::receive_udp_message warning - received message with unsupported id (" << nmsgid << ")" << endl;
		return true;
	}
	if (!it->second->HandleMessage(&wrpr, &remote_addr)) {
		cout << "NetFind::receive_udp_message warning - failed while processing message of type (" << nmsgid << ")" << endl;
		return true;
	}
	return true;
}

//Просмотр списка серверов, удаление серверов, прекративших свое существование
//заполнение списка серверов, которые необходимо опросить
void NetFind::review_remote_servers() {
	ping_list.clear();

	list<IRemoteNfServer*>::iterator it_rs;
	pthread_mutex_lock(&mtx);
	for (it_rs=remote_servers.begin();it_rs!=remote_servers.end();) {
		IRemoteNfServer* irnfs = *it_rs;
		if (NULL == irnfs) continue; //Этот сервер уже был удален
		irnfs->validate_alive(); //Проверяем, что сервер пингуется
		if (!irnfs->is_alive() && !irnfs->is_broadcast()) {
			it_rs = remote_servers.erase(it_rs);
			unlink_server(irnfs); //Сервер недоступен. Подготовить его к удалению
			continue;
		}
		if (irnfs->ping_allowed()) {
			ping_list.push_back(irnfs);
		}
		++it_rs;
	}
	pthread_mutex_unlock(&mtx);
}

void NetFind::invoke_requests() {
	list<IRemoteNfServer*>::iterator it_rs;
	//Пингуем сервера, для которых подошло время
	for (it_rs=ping_list.begin();it_rs!=ping_list.end();it_rs++) {
		link_handler->InvokeRequest(*it_rs);
	}
	//Собираем информацию с вновь обнаруженных серверов
	while (info_list.size()) {
		info_handler->InvokeRequest(info_list.pop_front());
	}

	//При необходимости собираем информацию о других серверах, работающих на этом хосте
	if (list_handler->requires_server_list()) {
		list_handler->RequestLocalhost();
	}
}

//Подготовить сервер к удалению. Сервер должен быть перемещен в список
//ожидающих удаления и удален из всех остальных списоков
void NetFind::unlink_server(IRemoteNfServer* irnfs) {
	if (NULL == irnfs) return;
	unlink_server_stream(irnfs);

	string uniq_id = irnfs->get_uniq_id();
	map<std::string, IRemoteNfServer*>::iterator it_uid = m_str_servers.find(uniq_id);
	if (it_uid != m_str_servers.end()) {
		m_str_servers.erase(it_uid);
	}

	//По возможности удаляем этот сервер из списка хостов. Только в том случае, если он не он не изменился
	sockaddr_in& sa = irnfs->get_remote_sockaddr();
	unsigned long long lsa = ((unsigned long long)sa.sin_port << 32) | ((unsigned int)sa.sin_addr.s_addr);
	map<unsigned long long, IRemoteNfServer*>::iterator it_sa = m_sa_servers.find(lsa);
	if (it_sa!=m_sa_servers.end() && irnfs==it_sa->second) {
		m_sa_servers.erase(it_sa);
	}

	if (0 == irnfs->decrease_ref_count()) {
		delete irnfs;
	}
}

//Поток клиента.
int NetFind::client_thread() {
	while (true) {
		if (!bind_client_port()) { //Поиск порта, на котором можно принимать ответные сообщения с точки зрения клиента
			return 1;
		}

		pollfd pfd;
		pfd.fd = sock_clt_rsp;
		pfd.events = POLLIN | POLLHUP;
		pfd.revents = 0;


		timeval tv_prev;
		gettimeofday(&tv_prev, NULL);
		while (true) {
			timeval tv_now;
			gettimeofday(&tv_now, NULL);
			unsigned  int delta = ((1000000-tv_prev.tv_usec)+(tv_now.tv_usec-1000000)+(tv_now.tv_sec-tv_prev.tv_sec)*1000000) / 1000;
			if (delta >= MIN_REVIEW_PERIOD) {
				review_remote_servers();
				invoke_requests();
				memcpy(&tv_prev, &tv_now, sizeof(timeval));
			}

			if (poll(&pfd, 1, 100) < 1) {
				continue;
			}

			if (!receive_udp_message(sock_clt_rsp)) {
				break;
			}
		}
	}
	return 0;
}


void* nf_server_rsp_thread_fcn (void* thread_arg) {
	NetFind* nf = (NetFind*)thread_arg;
	nf->server_response_thread();
	return NULL;
}

void* nf_client_thread_fcn (void* thread_arg) {
	NetFind* nf = (NetFind*)thread_arg;
	nf->client_thread();
	return NULL;
}

INetFind* net_find_create(net_find_config *nfc) {
	if (NULL == nfc) {
		cout << "net_find_create error - NULL pointer to net_find_config" << endl;
		return NULL;
	}

	return new NetFind(nfc);
}


