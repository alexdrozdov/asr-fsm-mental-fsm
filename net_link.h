/*
 * net_link.h
 *
 *  Created on: 26.06.2011
 *      Author: drozdov
 */

#ifndef NET_LINK_H_
#define NET_LINK_H_

#include <sys/types.h>
#include <sys/socket.h>

#ifdef AUXPACKAGES
#include <tcl.h>
#else
#include <tcl8.5/tcl.h>
#endif


#include <vector>
#include <map>
#include <string>

#include <p2vera.h>
#include <p2message.h>
#include <p2stream.h>

#include "dsp_stream.pb.h"

#include "base_trigger.h"
#include "virt_trigger.h"

extern void* handle_network_thread (void* thread_arg);

class CNetLink {
public:
	CNetLink();
	virtual ~CNetLink();

	int RegisterVirtualTrigger(CVirtTrigger* trigger);
	int TclHandler(Tcl_Interp* interp, int argc, CONST char *argv[]);

	virtual bool connect_client(P2VeraStream p2s_c2s, P2VeraStream p2s_s2c);
	virtual bool disconnect_client();

	int MkDump(bool enable);
	int MkDump(bool enable, std::string file_name);

	virtual void SendTextResponse(std::string response_text);
private:
	bool enabled;
	P2Vera *p2v;
	P2VeraStream p2s_c2s;
	P2VeraStream p2s_s2c;

	int handle_network();

	int process_message(unsigned char* buf, int len);
	int process_trig_msg(const ::dsp::modified_triggers& mt);
	int process_samplerate_msg(const ::dsp::samplerate_message& srtm);
	int process_time_msg(const ::dsp::time_message& tm);
	friend void* netlink_accept_thread (void* thread_arg);

	std::vector<CVirtTrigger*> virtuals;
	std::map<std::string,CVirtTrigger*> by_name;

	bool dump_instream;


	bool dump_enabled;
	bool dump_to_file;
	std::ofstream dump_stream; //Поток, в который выводится дамп

	friend void* handle_network_thread(void* thread_arg);
};


extern P2Vera *p2v;
extern CNetLink *net_link;


#endif /* NET_LINK_H_ */
