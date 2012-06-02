/*
 * test.cpp
 *
 *  Created on: 14.05.2012
 *      Author: drozdov
 */

#include <unistd.h>
#include "p2vera.h"
#include "net_find.h"

int main(int argc, char *argv[]) {
	P2Vera* p2v = new P2Vera();
	net_find_config nfc;
	nfc.nf_caption = "NetFind test";
	nfc.nf_name    = "nf_test";
	nfc.nf_address = "127.0.01"; //В тестовом режиме запускаемся на локалхосте
	nfc.nf_port    = "7300";
	nfc.nf_hash    = p2v->get_uniq_id();
	NetFind* nf = new NetFind(&nfc);
	nf->add_scanable_server("127.0.0.1", "7300");
	nf->add_broadcast_servers("7300");
	while(true) {
		usleep(100000);
	}
	return 0;
}
