/*
 * nf_remote_server.cpp
 *
 *  Created on: 26.05.2012
 *      Author: drozdov
 */

#include <iostream>
#include <string>
#include <vector>

#include "nf_remote_server.h"

using namespace std;

#define MIN_REMOTE_PING_DELTA 100
#define MAX_PING_QQ_SIZE 80
#define MAX_PING_TIMEOUT 2000
#define MAX_FAILURE_COUNT 10

RemoteNfServer::RemoteNfServer(int id, net_find_config* rnfc) {
	local_id = id;
	name = rnfc->nf_name;
	caption = rnfc->nf_caption;
	cluster = rnfc->nf_cluster;
	uniq_id = rnfc->nf_hash;
	if (rnfc->nf_hash.length()<1) {
		is_addr_placeholder = true;
	} else {
		is_addr_placeholder = false;
	}
	enabled = true;
	failure_count = 0;
	full_info_present = false;
	localhost = false;

	memset(&remote_addr, 0 , sizeof(sockaddr_in));
	remote_addr.sin_family = AF_INET;

	remote_addr.sin_port = htons(atoi(rnfc->nf_port.c_str()));
	if (0 == inet_aton(rnfc->nf_address.c_str(), &(remote_addr.sin_addr))) {
		cout << "RemoteNfServer::RemoteNfServer error: wrong server address" << endl;
		return;
	}

	gettimeofday(&tv_request, NULL);
	min_ping_time_delta = MIN_REMOTE_PING_DELTA;
}

RemoteNfServer::RemoteNfServer(int id, net_find_config* rnfc, sockaddr_in& addr) {
	local_id = id;
	name = rnfc->nf_name;
	caption = rnfc->nf_caption;
	uniq_id = rnfc->nf_hash;
	cluster = rnfc->nf_cluster;
	enabled = true;
	failure_count = 0;
	is_addr_placeholder = false;
	full_info_present = false;
	localhost = false;

	memcpy(&remote_addr, &addr, sizeof(sockaddr_in));
	alternate_addrs.push_back(addr);

	gettimeofday(&tv_request, NULL);
	min_ping_time_delta = MIN_REMOTE_PING_DELTA;
}

//Проверить возможность установки связи с этим приложением.
//Не гарантирует жизнеспособность приложения в текущий момент,
//опирается на наличие ответов в недавнем прошлом.
bool RemoteNfServer::is_alive() {
	if (failure_count <= MAX_FAILURE_COUNT) return true;
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

int RemoteNfServer::get_id() {
	return local_id;
}

string RemoteNfServer::get_uniq_id() {
	return uniq_id;
}

sockaddr_in&  RemoteNfServer::get_remote_sockaddr() {
	return remote_addr;
}

//Регистрация отправленного запроса
void RemoteNfServer::add_ping_request(int ping_id) {
	if (is_addr_placeholder) return; //Регистрировать что либо на несущесвующий хост не стоит.
	gettimeofday(&tv_request, NULL);
	if (pings_sent.size() >= MAX_PING_QQ_SIZE) {
		//Просмотреть пинги в словаре и удалить все самое старое
		map<int, rmt_ping>::iterator it;
		for (it=pings_sent.begin();it!=pings_sent.end();it++) {
			rmt_ping& rmtp = it->second;
			unsigned  int delta = ((1000000-rmtp.ping_send_time.tv_usec)+(tv_request.tv_usec-1000000)+(tv_request.tv_sec-rmtp.ping_send_time.tv_sec)*1000000) / 1000;
			if (delta>MAX_PING_TIMEOUT) {
				pings_sent.erase(it,it);
				failure_count++;
			}
		}
	}

	rmt_ping rmtp;
	rmtp.ping_id = ping_id;
	memcpy(&rmtp.ping_send_time, &tv_request, sizeof(timeval));

	pings_sent[ping_id] = rmtp;
}
//Регистрация принятого ответа
void RemoteNfServer::add_ping_response(int ping_id) {
	//timeval tv_now;
	//gettimeofday(&tv_now, NULL);

	map<int, rmt_ping>::iterator it = pings_sent.find(ping_id);
	if (it == pings_sent.end()) {
		return; //Такой пинг не найден
	}
	//rmt_ping& rmtp = it->second;
	//unsigned  int delta = ((1000000-rmtp.ping_send_time.tv_usec)+(tv_now.tv_usec-1000000)+(tv_now.tv_sec-rmtp.ping_send_time.tv_sec)*1000000) / 1000;
	//cout << "RemoteNfServer::add_ping_response info - ping to " << uniq_id << " = " << delta << "ms" << endl;
	pings_sent.erase(it);
	failure_count = 0;
}
//Проверка на количество потеряных запросов-ответов за единицу времени
void RemoteNfServer::validate_alive() {
	if (is_addr_placeholder) return; //Эта запись только хранит адрес и не может быть проверена

	timeval tv_now;
	gettimeofday(&tv_now, NULL);
	map<int, rmt_ping>::iterator it;
	for (it=pings_sent.begin();it!=pings_sent.end();it++) {
		rmt_ping& rmtp = it->second;
		unsigned  int delta = ((1000000-rmtp.ping_send_time.tv_usec)+(tv_now.tv_usec-1000000)+(tv_now.tv_sec-rmtp.ping_send_time.tv_sec)*1000000) / 1000;
		if (delta>MAX_PING_TIMEOUT) {
			pings_sent.erase(it,it);
			failure_count++;
		}
	}

	if (failure_count > MAX_FAILURE_COUNT)
		cout << "RemoteNfServer::validate_alive info - server " << uniq_id << " is dead" << endl;
}

void RemoteNfServer::add_alternate_addr(sockaddr_in& alt_addr) {
	sockaddr_in full_alt_addr;
	memcpy(&full_alt_addr, &alt_addr, sizeof(sockaddr_in));
	if (0 == full_alt_addr.sin_port) {
		full_alt_addr.sin_port = remote_addr.sin_port;
	}
	if (validate_addr(full_alt_addr)) {
		return; //Такой адрес уже присутсвует в списке. Удаляем его
	}
	alternate_addrs.push_back(full_alt_addr);
}

bool RemoteNfServer::validate_addr(sockaddr_in& alt_addr) {
	vector<sockaddr_in>::iterator it;
	for (it=alternate_addrs.begin();it!=alternate_addrs.end();it++) {
		if (it->sin_port==alt_addr.sin_port && it->sin_addr.s_addr==alt_addr.sin_addr.s_addr) {
			return true;
		}
	}
	return false;
}

bool RemoteNfServer::update_info(net_find_config* rnfc) {
	if (NULL == rnfc) return false;

	bool complete = true;
	if (rnfc->nf_caption.length() > 0) {
		caption = rnfc->nf_caption;
	} else {
		complete = false;
	}
	if (rnfc->nf_name.length() > 0) {
		name = rnfc->nf_name;
	} else {
		complete = false;
	}
	if (rnfc->nf_cluster.length() > 0) {
		cluster = rnfc->nf_cluster;
	} else {
		complete = false;
	}

	full_info_present = complete;
	return complete;
}

bool RemoteNfServer::ping_allowed() {
	if (!enabled) return false;

	timeval tv_now;
	gettimeofday(&tv_now, NULL);
	unsigned  int delta = ((1000000-tv_request.tv_usec)+(tv_now.tv_usec-1000000)+(tv_now.tv_sec-tv_request.tv_sec)*1000000) / 1000;
	if (delta < min_ping_time_delta) {
		return false; //Не прошло минимальное время с последнего опроса этого сервера
	}
	return true; //Можно пиговать
}

bool RemoteNfServer::requires_info_request() {
	if (!is_addr_placeholder && !full_info_present) {
		return true; //Сервер был обнаружен в сети, при этом полная информация о нем недоступна. Ее необходимо получить.
	}
	return false; //Сервер либо просто хранит адрес (в этом случае ничего запрашивать не надо), либо о нем уже все известно
}

//Сервер является физическим, а не виртуальным вещательным
bool RemoteNfServer::is_broadcast() {
	return false;
}

bool RemoteNfServer::is_localhost() {
	return localhost;
}

void RemoteNfServer::is_localhost(bool b) {
	localhost = b;
}

void RemoteNfServer::print_info() {
	cout << "server type: " << (is_addr_placeholder?(string)"virtual":(string)"physical") << endl;
	cout << "uniq id: " << uniq_id << endl;
	cout << "address: " << inet_ntoa(remote_addr.sin_addr) << ":" << htons(remote_addr.sin_port) << endl;
	if (full_info_present) {
		cout << "name:    " << name << endl;
		cout << "caption: " << caption << endl;
		cout << "cluster: " << cluster << endl;
	} else {
		cout << "warning: aux info unavaible" << endl;
	}
}

