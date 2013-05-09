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
	P2Vera* p2v = new P2Vera("./client_cfg.p2v");
	P2VeraStream p2s = p2v->create_instream("c2s-proc-data");

	pollfd *pfd = new pollfd[1];
	pfd[0].fd = p2s.get_fd();
	pfd[0].events = POLLIN | POLLHUP;
	pfd[0].revents = 0;

	p2v->start_network();

	AsrP2Stream ap2s(p2s);
	AsrMsg1 m1;
	AsrMsg2 m2;
	ap2s << m1 << m2 << asr_fl;


	while(true) {
		if (poll(pfd, 1, 100) < 1) {
			continue;
		}
		if (0==pfd[0].revents) continue;
		pfd[0].revents = 0;
		//P2VeraTextMessage p2tm;
		//stream_fd[pfd[i].fd] >> p2tm;
		//p2tm.print();
	}

	return 0;
}

