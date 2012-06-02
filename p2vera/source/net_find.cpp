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

#define MAX_RSP_RCV_BUFLEN 1024

using namespace std;
using namespace p2vera;
using namespace netfind;


NetFind::NetFind(net_find_config *nfc) {
	name = nfc->nf_name;
	caption = nfc->nf_caption;
	hash = nfc->nf_hash;

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
	msg_handlers[0] = link_handler;

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
	remote_servers.push_back(rnfs);
	m_str_servers[uniq_id] = rnfs;
	cout << "NetFind::add_discovered_server info - server registered for " << uniq_id << endl;

	return remote_id;
}

int NetFind::add_scanable_server(std::string address, std::string port) {
	//FIXME Уникальный идентификатор должен добавляться к каждому вновь создаваемому удаленному хосту
	net_find_config nfc;
	nfc.nf_address = address;
	nfc.nf_port = port;
	nfc.nf_caption = "unknown";
	nfc.nf_name = "unknown";
	nfc.scanable = true;
	nfc.usage = 0;

	int remote_id = generate_remote_id();
	RemoteNfServer* rnfs = new RemoteNfServer(remote_id, &nfc);
	remote_servers.push_back(rnfs);
	cout << "NetFind::add_scanable_server info - server created" << endl;

	return remote_id;
}

int NetFind::add_unscanable_server(std::string address, std::string port) {
	return 0;
}

int NetFind::add_broadcast_servers(std::string port) {
	net_find_config nfc;
	nfc.nf_port = port;

	BkstNfServer* bnfs = new BkstNfServer(generate_remote_id(), &nfc);
	remote_servers.push_back(bnfs);
	cout << "NetFind::add_broadcast_servers info - server created" << endl;
	return 0;
}

//Поиск сервера по его локальному id
IRemoteNfServer* NetFind::by_id(int id) {
	if (id<0 || id>=(int)remote_servers.size()) {
		return NULL;
	}
	return remote_servers[id];
}

//Поиск сервера по его обратному адресу
IRemoteNfServer* NetFind::by_sockaddr(sockaddr_in& sa) {
	unsigned long long lsa = ((unsigned long long)sa.sin_port << 32) | ((unsigned int)sa.sin_addr.s_addr);
	map<unsigned long long, IRemoteNfServer*>::iterator it = m_sa_servers.find(lsa);
	if (it == m_sa_servers.end()) {
		return NULL;
	}

	return it->second;
}

//Поиск сервера по его уникальному идентификатору
IRemoteNfServer* NetFind::by_uniq_id(std::string uniq_id) {
	map<std::string, IRemoteNfServer*>::iterator it = m_str_servers.find(uniq_id);
	if (it == m_str_servers.end()) {
		return NULL;
	}
	return it->second;
}

//Удаление сервера из списка. Сервер может быть снова найден и получит новый id
void NetFind::remove_remote_server(int id) {
}
//Вывод в консоль списка известрных серверов с их статусами
void NetFind::print_servers() {
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

		while (true) {
			vector<IRemoteNfServer*>::iterator it_rs;
			for (it_rs=remote_servers.begin();it_rs!=remote_servers.end();it_rs++) {
				link_handler->InvokeRequest(*it_rs);
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


