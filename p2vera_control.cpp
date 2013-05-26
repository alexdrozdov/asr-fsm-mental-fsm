/*
 * p2vera_control.cpp
 *
 *  Created on: 18.05.2013
 *      Author: drozdov
 */

#include <sys/poll.h>
#include <sys/time.h>

#include <pthread.h>

#include <iostream>

#include <p2vera.h>
#include <p2message.h>
#include <p2stream.h>

#include "p2vera_control.h"
#include "net_link.h"

using namespace std;

void* p2vctrl_thread (void* thread_arg) ;

P2VeraControl::P2VeraControl(P2Vera *p2v) {
	is_connected = false;
	is_started = false;
	this->p2v = p2v;
	p2ctrl = p2v->create_stream("control");
}

bool P2VeraControl::start() {
	if (is_started) return true;
	pthread_t thread_id;
	pthread_create (&thread_id, NULL, &p2vctrl_thread, this);
	return true;
}

void P2VeraControl::handle_network() {
	pollfd *pfd = new pollfd[1];
	pfd[0].fd = p2ctrl.get_fd();
	pfd[0].events = POLLIN | POLLHUP;
	pfd[0].revents = 0;

	while (true) {
		if (poll(pfd, 1, 100) < 1) {
			pfd[0].revents = 0;
			continue;
		}
		pfd[0].revents = 0;
		P2VeraTextMessage p2tm;
		p2ctrl >> p2tm;
		string request_str = p2tm;
		if (0 == request_str.compare(0,17,"connect request: ")) {
			if (!is_connected) {
				P2VeraStream p2s_c2s = p2v->create_instream("c2s-proc-data");
				P2VeraStream p2s_s2c = p2v->create_outstream("s2c-response");
				net_link->connect_client(p2s_c2s, p2s_s2c);

				string remote_id = request_str.substr(17, request_str.length()-17);
				P2VeraTextMessage p2tm_grant((string)"connect grant: " + remote_id);
				p2ctrl << p2tm_grant;
				is_connected = true;
			} else { //Соединение установлено и последующее соединение невозможно
				string remote_id = request_str.substr(17, request_str.length()-17);
				P2VeraTextMessage p2tm_deny((string)"connect deny: " + remote_id);
				p2ctrl << p2tm_deny;
			}
			continue;
		}
		if (0 == request_str.compare(0,20,"disconnect request: ")) {
			if (is_connected) { //FIXME Проверять возможность разрыва соединения
				string remote_id = request_str.substr(20, request_str.length()-20);
				P2VeraTextMessage p2tm_grant((string)"disconnect grant: " + remote_id);
				p2ctrl << p2tm_grant;
				is_connected = true;
			} else { //Соединение не установлено
				string remote_id = request_str.substr(20, request_str.length()-20);
				P2VeraTextMessage p2tm_deny((string)"disconnect grant: " + remote_id);
				p2ctrl << p2tm_deny;
			}
			continue;
		}
	}
	delete[] pfd;
}


bool P2VeraControl::alive() {
	return true;
}

void* p2vctrl_thread (void* thread_arg) {
	P2VeraControl* p2vctrl = (P2VeraControl*)thread_arg;
	p2vctrl->handle_network();

	return NULL;
}

P2VeraControl *p2vctrl = NULL;

