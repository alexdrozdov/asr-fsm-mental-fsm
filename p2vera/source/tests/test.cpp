/*
 * test.cpp
 *
 *  Created on: 14.05.2012
 *      Author: drozdov
 */

#include <unistd.h>
#include "net_find.h"

int main(int argc, char *argv[]) {
	net_find_config nfc;
	nfc.nf_caption = "NetFind test";
	nfc.nf_name    = "nf_test";
	nfc.nf_address = "127.0.01"; //В тестовом режиме запускаемся на локалхосте
	nfc.nf_port    = "7200";
	NetFind* nf = new NetFind(&nfc);
	while(true) {
		usleep(100000);
	}
	return 0;
}
