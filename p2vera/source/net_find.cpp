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

int NetFind::add_scanable_server(std::string address, std::string port) {
	//FIXME Уникальный идентификатор должен добавляться к каждому вновь создаваемому удаленному хосту
	net_find_config nfc;
	nfc.nf_address = address;
	nfc.nf_port = port;
	nfc.nf_caption = "unknown";
	nfc.nf_name = "unknown";
	nfc.scanable = true;
	nfc.usage = 0;
	RemoteNfServer* rnfs = new RemoteNfServer(generate_remote_id(), &nfc);
	remote_servers.push_back(rnfs);
	return 0;
}
int NetFind::add_unscanable_server(std::string address, std::string port) {
	return 0;
}

//Поиск сервера по его локальному id
IRemoteNfServer* NetFind::by_id(int id) {
	return NULL;
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
	unsigned char rcv_buf[MAX_RSP_RCV_BUFLEN] = {0};
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
		while (true) {
			socklen_t remote_addrlen = sizeof(sockaddr_in);
			sockaddr_in remote_addr;
			rc = recvfrom(sock_srv_rsp, rcv_buf, MAX_RSP_RCV_BUFLEN, 0, (sockaddr*)&remote_addr, &remote_addrlen);
			if(rc < 0) {
				cout << "NetFind::server_response_thread error - закрылся слушающий сокет сервера поиска сети." << endl;
				close(sock_srv_rsp);
				break;
			}
			msg_wrapper wrpr;
			if (!wrpr.ParseFromArray(rcv_buf, rc)) {
				cout << "NetFind::server_response_thread error - couldn`t parse wrapper" << endl;
				continue;
			}
			if (!wrpr.has_body()) {
				cout << "NetFind::server_response_thread warning - request body field is empty" << endl;
				continue;
			}
			int nmsgid = wrpr.msg_type();
			std::map<int, INetFindMsgHandler*>::iterator it = msg_handlers.find(nmsgid);
			if (it == msg_handlers.end()) {
				cout << "NetFind::server_response_thread warning - received message with unsupported id (" << nmsgid << ")" << endl;
				continue;
			}
			if (!it->second->HandleMessage(&wrpr, &remote_addr)) {
				cout << "NetFind::server_response_thread warning - failed while processing message of type (" << nmsgid << ")" << endl;
				continue;
			}
		}
	}
	return 0;
}
//Поток сервера. Обеспечивает построение маршрутов (пока не уверен)
int NetFind::server_route_thread() {
	return 0;
}
//Поток клиента.
int NetFind::client_thread() {
	//Поиск порта, на котором можно принимать ответные сообщения с точки зрения клиента
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
			return 1;
		}
		clientaddr.sin_port = htons((unsigned short)client_port);
		rc = bind(sock_clt_rsp, (struct sockaddr *)&clientaddr, sizeof(sockaddr_in));
	}

	pollfd pfd;
	pfd.fd = sock_clt_rsp;
	pfd.events = POLLIN | POLLHUP;
	pfd.revents = 0;

	unsigned char rcv_buf[MAX_RSP_RCV_BUFLEN] = {0};

	while (true) {
		link_handler->InvokeRequest();
		vector<IRemoteNfServer*>::iterator it_rs;
		for (it_rs=remote_servers.begin();it_rs!=remote_servers.end();it_rs++) {
			link_handler->InvokeRequest(*it_rs);
		}

		if (poll(&pfd, 1, 100) < 1) {
			continue;
		}
		socklen_t remote_addrlen = sizeof(sockaddr_in);
		sockaddr_in remote_addr;
		rc = recvfrom(sock_clt_rsp, rcv_buf, MAX_RSP_RCV_BUFLEN, 0, (sockaddr*)&remote_addr, &remote_addrlen);
		if(rc < 0) {
			cout << "NetFind::client_thread error - закрылся слушающий сокет клиента поиска сети." << endl;
			close(sock_clt_rsp);
			break;
		}
		msg_wrapper wrpr;
		if (!wrpr.ParseFromArray(rcv_buf, rc)) {
			cout << "NetFind::client_thread error - couldn`t parse wrapper" << endl;
			continue;
		}
		if (!wrpr.has_body()) {
			cout << "NetFind::client_thread warning - request body field is empty" << endl;
			continue;
		}
		int nmsgid = wrpr.msg_type();
		std::map<int, INetFindMsgHandler*>::iterator it = msg_handlers.find(nmsgid);
		if (it == msg_handlers.end()) {
			cout << "NetFind::client_thread warning - received message with unsupported id (" << nmsgid << ")" << endl;
			continue;
		}
		if (!it->second->HandleMessage(&wrpr, &remote_addr)) {
			cout << "NetFind::client_thread warning - failed while processing message of type (" << nmsgid << ")" << endl;
			continue;
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


