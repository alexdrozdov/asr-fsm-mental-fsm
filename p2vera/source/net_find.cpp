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

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "net_find.h"
#include "msg_wrapper.pb.h"
#include "msg_netfind.pb.h"

#define MAX_RSP_RCV_BUFLEN 1024

using namespace std;
using namespace p2vera;
using namespace netfind;


RemoteNfServer::RemoteNfServer(int id, net_find_config* rnfc) {
}
//Проверить возможность установки связи с этим приложением.
//Не гарантирует жизнеспособность приложения в текущий момент,
//опирается на наличие ответов в недавнем прошлом.
bool RemoteNfServer::is_alive() {
	return false;
}
//Запретить проверку этого приложения на жизнеспособность,
//принудительно считать приложение отсутствующим.
//Т.к. на удаленной машине существует только один сервер с указаным
//портом, машины различаются только ip-адресами. Поэтому блокировка
//одного сервера означает полную блокировку удаленной машины
void RemoteNfServer::forbide_usage() {
}
//Разрешает работу с этим сервером, т.е. с любым сервером,
//расположенным на этой удаленной машине
void RemoteNfServer::enable_usage() {
}

//Регистрация отправленного запроса
void RemoteNfServer::add_ping_request(int ping_id) {
}
//Регистрация принятого ответа
void RemoteNfServer::add_ping_response(int ping_id) {
}
//Проверка на количество потеряных запросов-ответов за единицу времени
void RemoteNfServer::validate_alive() {
}

NetFind::NetFind(net_find_config *nfc) {
	//Создаем мутексы
	pthread_mutex_init (&mtx, NULL);

	memset(&ownaddr, 0x00,sizeof(sockaddr_in));
	ownaddr.sin_family = AF_INET;
	ownaddr.sin_port = htons(atoi(nfc->nf_port.c_str()));
	ownaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//Создаем потоки
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, &nf_server_rsp_thread_fcn, this); //Поток, выполняющий роль сервера
	pthread_create(&thread_id, NULL, &nf_client_thread_fcn, this); //Поток, выполняющий роль сервера
}
//Позволяет проверить текущий режим этой копии библиотеки
bool NetFind::is_server() {
	return false;
}
int NetFind::add_scanable_server(std::string address, std::string port) {
	return 0;
}
int NetFind::add_unscanable_server(std::string address, std::string port) {
	return 0;
}

//Поиск сервера по его локальному id
RemoteNfServer* NetFind::by_id(int id) {
	return NULL;
}
//Удаление сервера из списка. Сервер может быть снова найден и получит новый id
void NetFind::remove_remote_server(int id) {
}
//Вывод в консоль списка известрных серверов с их статусами
void NetFind::print_servers() {
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
			socklen_t remote_addrlen = 0;
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
			switch (wrpr.dir()) {
				case msg_wrapper_msg_direction_request:
					break;
				case msg_wrapper_msg_direction_response:
					break;
				default:
					cout << "NetFind::server_response_thread warning - unknown message direction" << endl;
					break;
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


