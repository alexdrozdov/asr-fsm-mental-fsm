/*
 * test.cpp
 *
 *  Created on: 14.05.2012
 *      Author: drozdov
 */

#include <iostream>

#include <unistd.h>
#include "p2vera.h"
#include "inet_find.h"

using namespace std;

int main(int argc, char *argv[]) {
	P2Vera* p2v = new P2Vera();
	net_find_config nfc;
	nfc.nf_caption = "NetFind test";
	nfc.nf_name    = "nf_test";
	nfc.nf_address = "127.0.01"; //В тестовом режиме запускаемся на локалхосте
	nfc.nf_port    = "7300";
	nfc.nf_hash    = p2v->get_uniq_id();
	nfc.nf_cluster = "netfind-test-cluster";
	INetFind* nf = net_find_create(&nfc);
	nf->add_scanable_server("127.0.0.1", "7300");
	//nf->add_broadcast_servers("7300");
	while(true) {
		usleep(10000000);
		cout << endl;
		cout << "Действующие сервера..." << endl;
		nf->print_servers();
	}
	return 0;
}
