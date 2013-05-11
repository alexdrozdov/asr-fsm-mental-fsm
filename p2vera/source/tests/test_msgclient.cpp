/*
 * test_msgclient.cpp
 *
 *  Created on: 02.05.2013
 *      Author: drozdov
 */


#include <sys/poll.h>
#include <signal.h>

#include <sys/time.h>

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
	P2VeraStream p2s = p2v->create_outstream("c2s-proc-data");
	P2VeraStream p2ctrl = p2v->create_stream("control");

	pollfd *pfd = new pollfd[2];
	pfd[0].fd = p2s.get_fd();
	pfd[0].events = POLLIN | POLLHUP;
	pfd[0].revents = 0;
	pfd[1].fd = p2ctrl.get_fd();
	pfd[1].events = POLLIN | POLLHUP;
	pfd[1].revents = 0;

	p2v->start_network();

	P2VeraTextMessage p2tm_connect((string)"connect request: " + p2v->get_uniq_id());
	P2VeraTextMessage p2tm_response;
	p2ctrl << p2tm_connect;

	timeval tv_now;
	timeval tv_request;
	gettimeofday(&tv_request, NULL);


	int connect_attemps = 0;
	const int max_connect_attemps = 20;
	bool connected = false;
	while(!connected) {
		if (poll(pfd, 2, 100) < 1) {
			gettimeofday(&tv_now, NULL);
			unsigned  int delta = ((1000000-tv_request.tv_usec)+(tv_now.tv_usec-1000000)+(tv_now.tv_sec-tv_request.tv_sec)*1000000) / 1000;
			if (delta < 500) {
				continue;
			}
			gettimeofday(&tv_request, NULL);
			p2ctrl << p2tm_connect;
			continue;
		}
		if (0==pfd[1].revents) continue;
		pfd[1].revents = 0;

		p2ctrl >> p2tm_response;
		string response_str = p2tm_response;
		string response_grant = "connect grant: " + p2v->get_uniq_id();
		if (response_str == response_grant) {
			cout << "remote connection accepted" << endl;
			connected = true;
			break;
		}

		connect_attemps++;
		if (connect_attemps >= max_connect_attemps) {
			cout << "failed to connect remote server, exiting" << endl;
			exit(0);
		}
	}

	AsrMsg1 m1("message_m1");
	AsrMsg2 m2("message_m2");
	p2s << m1 << m2;

	while (true) {
		usleep(100000);
	}


	return 0;
}

