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

#include "mtx_containers.h"
#include "mtx_containers.hpp"

#define MAX_RSP_RCV_BUFLEN 1024
#define MIN_REVIEW_PERIOD 330

using namespace std;
using namespace p2vera;
using namespace netfind;


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
	msg_handlers[0] = link_handler;
	msg_handlers[1] = info_handler;

	//Создаем потоки
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, &nf_server_rsp_thread_fcn, this); //Поток, выполняющий роль сервера
	pthread_create(&thread_id, NULL, &nf_client_thread_fcn, this); //Поток, выполняющий роль сервера
}
//Позволяет проверить текущий режим этой копии библиотеки
bool NetFind::is_server() {
	return false;
}

int NetFind::generate_remote_id() {
	pthread_mutex_lock(&mtx);
	int n = last_remote_id++;
	pthread_mutex_unlock(&mtx);
	return n;
}

int NetFind::add_discovered_server(sockaddr_in& addr, std::string uniq_id) {
	net_find_config nfc;
	nfc.nf_caption = "unknown";
	nfc.nf_name = "unknown";
	nfc.nf_hash = uniq_id;
	nfc.scanable = true;
	nfc.usage = 0;

	int remote_id = generate_remote_id();
	RemoteNfServer* rnfs = new RemoteNfServer(remote_id, &nfc, addr);

	pthread_mutex_lock(&mtx);
	remote_servers[remote_id] = rnfs;
	m_str_servers[uniq_id] = rnfs;
	reg_to_sockaddr(addr, rnfs);
	pthread_mutex_unlock(&mtx);

	if (rnfs->requires_info_request()) {
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
	remote_servers[remote_id] = rnfs;
	pthread_mutex_unlock(&mtx);

	return remote_id;
}

int NetFind::add_broadcast_servers(std::string port) {
	net_find_config nfc;
	nfc.nf_port = port;

	int remote_id = generate_remote_id();
	BkstNfServer* bnfs = new BkstNfServer(remote_id, &nfc);
	pthread_mutex_lock(&mtx);
	remote_servers[remote_id] = bnfs;
	pthread_mutex_unlock(&mtx);
	return 0;
}

//Поиск сервера по его локальному id
IRemoteNfServer* NetFind::by_id(int id) {
	IRemoteNfServer* irnfs = NULL;
	pthread_mutex_lock(&mtx);
	map<int, IRemoteNfServer*>::iterator it = remote_servers.find(id);
	if (it != remote_servers.end()) {
		irnfs = it->second;
	}
	pthread_mutex_unlock(&mtx);
	return irnfs;
}

//Поиск сервера по его обратному адресу
IRemoteNfServer* NetFind::by_sockaddr(sockaddr_in& sa) {
	pthread_mutex_lock(&mtx);
	unsigned long long lsa = ((unsigned long long)sa.sin_port << 32) | ((unsigned int)sa.sin_addr.s_addr);
	map<unsigned long long, IRemoteNfServer*>::iterator it = m_sa_servers.find(lsa);
	if (it == m_sa_servers.end()) {
		pthread_mutex_unlock(&mtx);
		return NULL;
	}

	pthread_mutex_unlock(&mtx);
	return it->second;
}

//Поиск сервера по его уникальному идентификатору
IRemoteNfServer* NetFind::by_uniq_id(std::string uniq_id) {
	pthread_mutex_lock(&mtx);
	map<std::string, IRemoteNfServer*>::iterator it = m_str_servers.find(uniq_id);
	if (it == m_str_servers.end()) {
		pthread_mutex_unlock(&mtx);
		return NULL;
	}
	pthread_mutex_unlock(&mtx);
	return it->second;
}

void NetFind::reg_to_sockaddr(sockaddr_in& sa, IRemoteNfServer* irnfs) {
	unsigned long long lsa = ((unsigned long long)sa.sin_port << 32) | ((unsigned int)sa.sin_addr.s_addr);
	map<unsigned long long, IRemoteNfServer*>::iterator it = m_sa_servers.find(lsa);
	if (it!=m_sa_servers.end()) {
		//Сервер с таким сочетанием ip--слушающий-порт уже был зарегистрирован ранее
		//это означает, что он завершил свою работу. Теперь его необходимо пометить, как
		//неактивный
		IRemoteNfServer* sa_irnfs = it->second;
		sa_irnfs->forbide_usage();
		cout << "NetFind::reg_to_sockaddr info - server " << sa_irnfs->get_uniq_id() << " was replaced by " << irnfs->get_uniq_id() << endl;
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
	map<int,IRemoteNfServer*>::iterator it;
	int count = 0;
	for (it=remote_servers.begin();it!=remote_servers.end();it++) {
		IRemoteNfServer* irnfs = it->second;
		if (NULL == irnfs) continue;
		irnfs->print_info();
		cout << endl;
		count++;
	}
	cout << "total: " << count << endl;
	pthread_mutex_unlock(&mtx);
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
		while (receive_udp_message(sock_srv_rsp));
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

	cout << "NetFind::client_thread info - поиск свободного порта..." << endl;
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

	map<int,IRemoteNfServer*>::iterator it_rs;
	pthread_mutex_lock(&mtx);
	for (it_rs=remote_servers.begin();it_rs!=remote_servers.end();it_rs++) {
		IRemoteNfServer* irnfs = it_rs->second;
		if (NULL == irnfs) continue; //Этот сервер уже был удален
		irnfs->validate_alive(); //Проверяем, что сервер пингуется
		if (!irnfs->is_alive() && !irnfs->is_broadcast()) {
			unlink_server(irnfs); //Сервер недоступен. Подготовить его к удалению
			continue;
		}
		if (irnfs->ping_allowed()) {
			ping_list.push_back(irnfs);
		}
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
}

//Подготовить сервер к удалению. Сервер должен быть перемещен в список
//ожидающих удаления и удален из всех остальных списоков
void NetFind::unlink_server(IRemoteNfServer* irnfs) {
	if (NULL == irnfs) return;
	remove_list.push_back(irnfs);

	int nid = irnfs->get_id();

	string uniq_id = irnfs->get_uniq_id();
	map<std::string, IRemoteNfServer*>::iterator it_uid = m_str_servers.find(uniq_id);
	if (it_uid != m_str_servers.end()) {
		m_str_servers.erase(it_uid);
	}

	map<int, IRemoteNfServer*>::iterator it_id = remote_servers.find(nid);
	if (it_id != remote_servers.end()) {
		remote_servers.erase(it_id);
	}

	//FIXED Удалять запись из списка адресов нельзя, т.к. она уже может быть занята другим приложением,
	//запустившимся на той же машине и занявшим тот же порт.
	//unsigned long long lsa = ((unsigned long long)sa.sin_port << 32) | ((unsigned int)sa.sin_addr.s_addr);
	//map<unsigned long long, IRemoteNfServer*>::iterator it_sa = m_sa_servers.find(lsa);
	//if (it_sa!=m_sa_servers.end()) {
	//	m_sa_servers.erase(it_sa);
	//}
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


