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

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "p2vera.h"
#include "inet_find.h"
#include "net_find.h"
#include "p2stream.h"
#include "p2hub.h"

using namespace std;

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
		pthread_mutex_unlock(&mtx);
		return;
	}
	IP2VeraStreamHub* sh = new P2VeraStreamHub(stream_cfg.name);
	hubs[stream_cfg.name] = sh;
	nf->register_stream(stream_cfg, sh);
	pthread_mutex_unlock(&mtx);
}

void P2Vera::start_network() {
	networking_active = true;
}

P2VeraStream P2Vera::create_stream(std::string name) {
	pthread_mutex_lock(&mtx);
	map<std::string, IP2VeraStreamHub*>::iterator it = hubs.find(name);
	if (hubs.end() != it) {
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
	if (hubs.end() != it) {
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
	if (hubs.end() != it) {
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
