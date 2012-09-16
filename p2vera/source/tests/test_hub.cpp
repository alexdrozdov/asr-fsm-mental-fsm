/*
 * test_hub.cpp
 *
 *  Created on: 15.07.2012
 *      Author: drozdov
 */


#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <signal.h>

#include <iostream>
#include <list>

#include "p2vera.h"
#include "p2stream.h"
#include "p2hub.h"

#include "p2message.h"

using namespace std;

void* listener_thread_fcn (void* thread_arg);

typedef struct _listner_task {
	P2VeraStream p2vs;
	int thread_number;
} listner_task;

int main(int argc, char *argv[]) {
	signal(SIGPIPE, SIG_IGN);
	P2VeraStreamHub sh("sound");
	P2VeraStream vs = sh.create_outstream();
	for (int i=0;i<5;i++) {
		listner_task *lt = new listner_task;
		lt->p2vs = sh.create_stream();
		lt->thread_number = i;
		pthread_t thread_id;
		pthread_create(&thread_id, NULL, &listener_thread_fcn, lt);
	}
	while (true) {
		P2VeraStream vs2 = vs;
		P2VeraTextMessage p2m("hello");
		vs << p2m;
		usleep(10000);
	}
	return 0;
}

void* listener_thread_fcn (void* thread_arg) {
	listner_task *lt = (listner_task*)thread_arg;
	struct pollfd poll_arr[1];
	poll_arr[0].fd = lt->p2vs.get_fd();
	poll_arr[0].events = POLLIN | POLLHUP;
	poll_arr[0].revents = 0;
	while (true) {
		int retpoll = poll(poll_arr, 1, 100);
		if (retpoll>0 && poll_arr[0].revents&POLLIN) {
			P2VeraTextMessage p2m2;
			lt->p2vs >> p2m2;
			cout << lt->thread_number << " " << (std::string)p2m2 << endl;
		}
		poll_arr[0].revents = 0;
	}
	return NULL;
}
