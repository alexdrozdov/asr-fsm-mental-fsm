/*
 * test_p2vera.cpp
 *
 *  Created on: 26.07.2012
 *      Author: drozdov
 */


#include "p2vera.h"

int main(int argc, char* argv[]) {
	P2Vera* p2v = new P2Vera();

	stream_config stream_cfg;
	stream_cfg.name = "base";
	stream_cfg.direction = stream_direction_bidir;
	stream_cfg.order = stream_msg_order_any;
	stream_cfg.type = stream_type_dgram;
	p2v->register_stream(stream_cfg);

	while(true) {
		usleep(1000000);
	}
	return 0;
}

