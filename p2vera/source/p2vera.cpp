/*
 * p2vera.cpp
 *
 *  Created on: 14.05.2012
 *      Author: drozdov
 */

#include <string.h>

#include <unistd.h>
#include <sys/time.h>

#include <sys/poll.h>
#include <pthread.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "p2vera.h"
#include "inet_find.h"
#include "net_find.h"
#include "p2stream.h"
#include "p2hub.h"
#include "p2message.h"
#include "cfg_reader.h"
#include "base64.h"

using namespace std;

#define MAX_RSP_RCV_BUFLEN 65536
#define MAX_HOSTNAME_LEN 100
typedef struct _app_uniq_info {
	timeval tv_now;
	int proc_id;
	int parent_proc_id;
	int host_id;
	char hostname[MAX_HOSTNAME_LEN];
} app_uniq_info;


P2Vera::P2Vera() {
	networking_active = false;
	generate_uniq_id();
	create_netfind();
	if (0 != pthread_mutex_init(&mtx, NULL)) {
		cout << "P2Vera::P2Vera error - couldn`t create mutex. Fatal." << endl;
		exit(1);
	}
}

P2Vera::P2Vera(std::string cfg_file) {
	CfgReader cr(cfg_file);
	networking_active = false;
	generate_uniq_id();

	net_find_config nfc; //Создаем и настраиваем модуль обнаружения с настройками из переданного файла
	nfc.nf_caption = cr.get_item_as_string("local_server", "caption", "p2v default caption");
	nfc.nf_name    = cr.get_item_as_string("local_server", "name", "p2v default caption");
	nfc.nf_address = cr.get_item_as_string("local_server", "address", "127.0.0.1");
	nfc.nf_port    = cr.get_item_as_string("local_server", "server_port", "7300");
	nfc.nf_hash    = this->get_uniq_id();
	nfc.nf_cluster = cr.get_item_as_string("local_server", "cluster", "p2v test cluster");
	nf = net_find_create(&nfc);

	//Добавляем удаленные сервера и параметры вещательных запросов
	for (CfgReaderIterator it=cr.get_cfg_items("discover", "remote_server");it!=cr.end();it++) {
		std::string srv = it->to_string();
		std::string port = nfc.nf_port;
		CfgReaderIterator it_port = it->get_cfg_items("port");
		if (it->end() != it_port)
			port = it_port->to_string();
		nf->add_scanable_server(srv, port);
	}
	for (CfgReaderIterator it=cr.get_cfg_items("discover", "broadcast_server");it!=cr.end();it++) {
		//std::string srv = it->to_string();
		std::string port = nfc.nf_port;
		CfgReaderIterator it_port = it->get_cfg_items("port");
		if (it->end() != it_port)
			port = it_port->to_string();
		nf->add_broadcast_servers(port); //FIXME Вещательные запросы должны рассылаться по маске с учетом сетевого интерфейса и его алиасов
	}

	//Добавляем потоки
	for (CfgReaderIterator it=cr.get_cfg_items("streams", "stream");it!=cr.end();it++) {
		stream_config stream_cfg;
		stream_cfg.name = it->to_string();
		stream_cfg.direction = stream_direction_bidir;
		stream_cfg.order = stream_msg_order_any;
		stream_cfg.type = stream_type_dgram;
		CfgReaderIterator it_dir = it->get_cfg_items("direction");
		if (it->end() != it_dir) {
			string dir = it_dir->to_string();
			if ("input" == dir) {
				stream_cfg.direction = stream_direction_income;
			} else if ("output" == dir) {
				stream_cfg.direction = stream_direction_outcome;
			} else if ("bidir" == dir) {
				stream_cfg.direction = stream_direction_bidir;
			} else {
				stream_cfg.direction = stream_direction_loopback;
			}
		}
		CfgReaderIterator it_order = it->get_cfg_items("integrity");
		if (it->end() != it_order) {
			string order = it_order->to_string();
			if ("complete" == order) {
				stream_cfg.order = stream_msg_order_strict;
			} else if ("chrono" == order) {
				stream_cfg.order = stream_msg_order_chrono;
			} else {
				stream_cfg.order = stream_msg_order_any;
			}
		}
		CfgReaderIterator it_proto = it->get_cfg_items("protocol");
		if (it->end() != it_proto) {
			string proto = it_proto->to_string();
			if ("flow" == proto) {
				stream_cfg.type = stream_type_flow;
			} else if ("dgram" == proto){
				stream_cfg.type = stream_type_dgram;
			} else {
				cout << "P2Vera::P2Vera warning - unknown protocol " << proto << ", assuming dgram" << endl;
			}
		}
		register_stream(stream_cfg);
	}

	if (0 != pthread_mutex_init(&mtx, NULL)) {
		cout << "P2Vera::P2Vera error - couldn`t create mutex. Fatal." << endl;
		exit(1);
	}
}

P2Vera::~P2Vera() {
}

void P2Vera::register_stream(stream_config& stream_cfg) {
	pthread_mutex_lock(&mtx);
	if (networking_active) {
		pthread_mutex_unlock(&mtx);
		cout << "P2Vera::register_stream warning - tried to register stream " << stream_cfg.name << " while networking is active" << endl;
		return;
	}
	map<std::string, IP2VeraStreamHub*>::iterator it = hubs.find(stream_cfg.name);
	if (hubs.end() != it) {
		cout << "P2Vera::register_stream warning - stream " << stream_cfg.name << " was already registered" << endl;
		pthread_mutex_unlock(&mtx);
		return;
	}
	IP2VeraStreamHub* sh = new P2VeraStreamHub(stream_cfg.name);
	hubs[stream_cfg.name] = sh;
	if (stream_cfg.type == stream_type_dgram) { //Управление файловыми дескрипторами выполняется в объекте
		//NetFind. Поэтому рвозвращенный файловый дескриптор добавляется в массив
		int fd = nf->register_stream(stream_cfg, sh);
		hub_fds[fd] = sh;
	} else { //Управление файловыми дескрипторами выполняется в специализированном объекте внутри NetFind. В этом классе файловый дескриптор не добавляется
		nf->register_stream(stream_cfg, sh);
	}
	pthread_mutex_unlock(&mtx);
}

void P2Vera::start_network() {
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, &rcv_thread_fcn, this); //Поток, выполняющий роль сервера
	networking_active = true;
}

void P2Vera::rcv_thread() {
	pthread_mutex_lock(&mtx);
	int fd_count = hub_fds.size();
	pollfd *pfd = new pollfd[fd_count];
	int fd_cnt = 0;
	for (map<int, IP2VeraStreamHub*>::iterator it=hub_fds.begin();it!=hub_fds.end();it++) {
		pfd[fd_cnt].fd = it->first;
		pfd[fd_cnt].events = POLLIN | POLLHUP;
		pfd[fd_cnt].revents = 0;
		fd_cnt++;
	}
	pthread_mutex_unlock(&mtx);
	unsigned char rcv_buf[MAX_RSP_RCV_BUFLEN] = {0};
	sockaddr_in remote_addr;
	socklen_t remote_addrlen = sizeof(sockaddr_in);

	while (true) {
		if (poll(pfd, fd_count, 100) < 1) {
			continue;
		}
		for (int i=0;i<fd_count;i++) {
			if (0==pfd[i].revents) continue;
			pfd[i].revents = 0;
			int rc = recvfrom(pfd[i].fd, rcv_buf, MAX_RSP_RCV_BUFLEN, 0, (sockaddr*)&remote_addr, &remote_addrlen);
			if(rc < 0) {
				continue;
			}
			IP2VeraStreamHub* sh = hub_fds[pfd[i].fd];
			P2VeraTextMessage p2tm;
			p2tm.set_data(rcv_buf, rc);
			sh->receive_message(p2tm);
		}
	}
	delete[] pfd;
}

P2VeraStream P2Vera::create_stream(std::string name) {
	pthread_mutex_lock(&mtx);
	map<std::string, IP2VeraStreamHub*>::iterator it = hubs.find(name);
	if (hubs.end() == it) {
		pthread_mutex_unlock(&mtx);
		throw;
	}
	IP2VeraStreamHub* sh = it->second;
	P2VeraStream p2s = sh->create_stream();
	pthread_mutex_unlock(&mtx);
	return p2s;
}

P2VeraStream P2Vera::create_instream(std::string name) {
	pthread_mutex_lock(&mtx);
	map<std::string, IP2VeraStreamHub*>::iterator it = hubs.find(name);
	if (hubs.end() == it) {
		pthread_mutex_unlock(&mtx);
		throw;
	}
	IP2VeraStreamHub* sh = it->second;
	P2VeraStream p2s = sh->create_instream();
	pthread_mutex_unlock(&mtx);
	return p2s;
}

P2VeraStream P2Vera::create_outstream(std::string name) {
	pthread_mutex_lock(&mtx);
	map<std::string, IP2VeraStreamHub*>::iterator it = hubs.find(name);
	if (hubs.end() == it) {
		pthread_mutex_unlock(&mtx);
		throw;
	}
	IP2VeraStreamHub* sh = it->second;
	P2VeraStream p2s = sh->create_outstream();
	pthread_mutex_unlock(&mtx);
	return p2s;
}


void P2Vera::create_netfind() {
	net_find_config nfc;
	nfc.nf_caption = "NetFind test";
	nfc.nf_name    = "nf_test";
	nfc.nf_address = "127.0.0.1"; //В тестовом режиме запускаемся на локалхосте
	nfc.nf_port    = "7300";
	nfc.nf_hash    = this->get_uniq_id();
	nfc.nf_cluster = "netfind-test-cluster";
	nf = net_find_create(&nfc);
	nf->add_scanable_server("127.0.0.1", "7300");
	//nf->add_broadcast_servers("7300");
}

unsigned int crc32(const unsigned char *buf, size_t len) {
    unsigned int crc_table[256];
    unsigned int crc;

    for (int i = 0; i < 256; i++) {
        crc = i;
        for (int j = 0; j < 8; j++) {
            crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;
        }
        crc_table[i] = crc;
    }

    crc = 0xFFFFFFFFUL;
    while (len--) {
        crc = crc_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFFUL;
}

void P2Vera::generate_uniq_id() {
	app_uniq_info aui;
	gettimeofday(&aui.tv_now, NULL);
	aui.proc_id = getpid();
	aui.parent_proc_id = getppid();
	aui.host_id = gethostid();
	gethostname(aui.hostname, MAX_HOSTNAME_LEN);

	unsigned int crc32_hash = crc32((const unsigned char*)(&aui), sizeof(app_uniq_info));
	uniq_id = base64_encode((unsigned char const*)&crc32_hash, sizeof(unsigned int));
}

std::string P2Vera::get_uniq_id() {
	return uniq_id;
}

void* rcv_thread_fcn(void* thread_arg) {
	P2Vera* p2v = reinterpret_cast<P2Vera*>(thread_arg);
	p2v->rcv_thread();
	return NULL;
}


