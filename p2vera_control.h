/*
 * p2vera_control.h
 *
 *  Created on: 18.05.2013
 *      Author: drozdov
 */

#ifndef P2VERA_CONTROL_H_
#define P2VERA_CONTROL_H_

#include <p2vera.h>
#include <p2message.h>
#include <p2stream.h>

extern void* p2vctrl_thread (void* thread_arg) ;

class P2VeraControl {
public:
	P2VeraControl(P2Vera *p2v);
	bool start();
	bool connected();
	bool alive();

	friend void* p2vctrl_thread (void* thread_arg);
private:
	void handle_network();

	P2Vera *p2v;
	P2VeraStream p2ctrl;
	bool is_connected;
	bool is_started;
};

extern P2VeraControl *p2vctrl;

#endif /* P2VERA_CONTROL_H_ */
