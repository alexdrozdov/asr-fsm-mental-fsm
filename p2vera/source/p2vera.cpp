/*
 * p2vera.cpp
 *
 *  Created on: 14.05.2012
 *      Author: drozdov
 */

#include <string.h>

#include <unistd.h>
#include <sys/time.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

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
	nfc.nf_address = "127.0.01"; //В тестовом режиме запускаемся на локалхосте
	nfc.nf_port    = "7300";
	nfc.nf_hash    = this->get_uniq_id();
	nfc.nf_cluster = "netfind-test-cluster";
	nf = net_find_create(&nfc);
	nf->add_scanable_server("127.0.0.1", "7300");
	nf->add_broadcast_servers("7300");
}

void P2Vera::generate_uniq_id() {
	app_uniq_info aui;
	gettimeofday(&aui.tv_now, NULL);
	aui.proc_id = getpid();
	aui.parent_proc_id = getppid();
	aui.host_id = gethostid();
	gethostname(aui.hostname, MAX_HOSTNAME_LEN);

	MD5((const unsigned char*)(&aui), sizeof(app_uniq_info), (unsigned char*)md5_data);

	//Перекодируем в Base-64
	BIO *bmem, *b64;
	BUF_MEM *bptr;

	b64 = BIO_new(BIO_f_base64());
	bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, md5_data, MD5_DIGEST_LENGTH);
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);
	char *buff = new char[bptr->length+1];
	memcpy(buff, bptr->data, bptr->length-1);
	buff[bptr->length-1] = 0;
	uniq_id = buff;
	delete[] buff;
	BIO_free_all(b64);
}

std::string P2Vera::get_uniq_id() {
	return uniq_id;
}

void* rcv_thread_fcn(void* thread_arg) {
	P2Vera* p2v = reinterpret_cast<P2Vera*>(thread_arg);
	p2v->rcv_thread();
	return NULL;
}


