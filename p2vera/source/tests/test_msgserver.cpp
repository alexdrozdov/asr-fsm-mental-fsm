/*
 * test_msgclient.cpp
 *
 *  Created on: 02.05.2013
 *      Author: drozdov
 */


#include <sys/poll.h>
#include <signal.h>

#include <iostream>
#include <string>
#include <list>
#include <map>

#include "common.h"
#include "p2vera.h"
#include "p2message.h"
#include "p2stream.h"

#include "p2vera_link.h"


using namespace std;



int main(int argc, const char *argv[]) {
	signal(SIGPIPE, SIG_IGN);
	P2Vera* p2v = new P2Vera("./server_cfg.p2v");
	P2VeraStream p2s = p2v->create_instream("c2s-proc-data");
	P2VeraStream p2ctrl = p2v->create_stream("control");

	pollfd *pfd = new pollfd[2];
	pfd[0].fd = p2s.get_fd();
	pfd[0].events = POLLIN | POLLHUP;
	pfd[0].revents = 0;

	pfd[1].fd = p2ctrl.get_fd();
	pfd[1].events = POLLIN | POLLHUP;
	pfd[1].revents = 0;

	p2v->start_network();

	while(true) {
		if (poll(pfd, 2, 100) < 1) {
			continue;
		}
		if (0!=pfd[0].revents) {
			pfd[0].revents = 0;
			AsrMsgCmn amc;
			p2s >> amc;
			if (amc.has_m1()) {
				AsrMsg1 am1;
				amc >> am1;
				am1.print();
			}
			if (amc.has_m2()) {
				AsrMsg2 am2;
				amc >> am2;
				am2.print();
			}
		}
		if (0!=pfd[1].revents) {
			pfd[1].revents = 0;
			P2VeraTextMessage p2tm;
			p2ctrl >> p2tm;
			string request_str = p2tm;
			if (0 == request_str.compare(0,17,"connect request: ")) {
				string remote_id = request_str.substr(17, request_str.length()-17);
				P2VeraTextMessage p2tm_grant((string)"connect grant: " + remote_id);
				p2ctrl << p2tm_grant;
			}
		}
	}

	return 0;
}

