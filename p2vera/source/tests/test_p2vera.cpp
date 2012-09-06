/*
 * test_p2vera.cpp
 *
 *  Created on: 26.07.2012
 *      Author: drozdov
 */


#include <signal.h>
#include "p2vera.h"
#include "p2message.h"

int main(int argc, char* argv[]) {
	signal(SIGPIPE, SIG_IGN);
	P2Vera* p2v = new P2Vera();

	stream_config stream_cfg;
	stream_cfg.name = "base";
	stream_cfg.direction = stream_direction_bidir;
	stream_cfg.order = stream_msg_order_any;
	stream_cfg.type = stream_type_dgram;
	p2v->register_stream(stream_cfg);
	p2v->start_network();

	P2VeraStream p2s = p2v->create_outstream("base");
	while(true) {
		usleep(100000);
		P2VeraTextMessage p2tm("hello world!");
		p2s << p2tm;
	}
	return 0;
}

