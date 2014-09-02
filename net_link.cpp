/*
 * net_link.cpp
 *
 *  Created on: 26.06.2011
 *      Author: drozdov
 */


#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <string.h>

#include <sys/poll.h>

#include <unistd.h>
#include <fcntl.h>

#include <pthread.h>

#include "crc.h"

#include "net_link.h"
#include "mental_fsm.h"
#include "back_messages.h"
#include "global_vars.h"
#include "p2vera_dsp.h"
#include "tclinterp_stream.h"

using namespace std;

P2Vera *p2v = NULL;

CNetLink::CNetLink() {
	dump_instream = false;
	dump_enabled = false;
	dump_to_file = false;
	enabled = false;
}

CNetLink::~CNetLink() {}

int CNetLink::handle_network() {
	pollfd *pfd = new pollfd[1];
	pfd[0].fd = p2s_c2s.get_fd();
	pfd[0].events = POLLIN | POLLHUP;
	pfd[0].revents = 0;

	DspMessageCmn dmc;
	while (enabled) {
		if (poll(pfd, 1, 100) < 1) {
			pfd[0].revents = 0;
			continue;
		}
		pfd[0].revents = 0;
		//cout << "CNetLink::handle_network info - msg received" << endl;
		p2s_c2s >> dmc;
		//cout << "CNetLink::handle_network info - msg poped" << endl;
		if (dmc.has_time()) {
			//cout << "CNetLink::handle_network info - has time" << endl;
			process_time_msg(dmc.dsp().time_inst(0));
		}
		if (dmc.has_samplerate()) {
			//cout << "CNetLink::handle_network info - has samplerate" << endl;
			process_samplerate_msg(dmc.dsp().samplerate_inst(0));
		}
		if (dmc.has_trig()) {
			//cout << "CNetLink::handle_network info - has trig" << endl;
			process_trig_msg(dmc.dsp().modified_triggers_inst(0));
		}
		dmc.Clear();
	}
	delete[] pfd;
	return 0;
}

int CNetLink::process_trig_msg(const ::dsp::modified_triggers& mt) {
	//cout << "CNetLink::process_trig_msg info - launched" << endl;
	::google::protobuf::RepeatedPtrField< ::dsp::modified_triger>::const_iterator ctit;
	for (ctit=mt.items().begin();ctit!=mt.items().end();ctit++) {
		const ::dsp::modified_triger& mmt = *ctit;
		if (ds_in.enabled()) {
			ds_in << "CNetLink::process_trig_msg info - trigger id: " << mmt.id() << "; outputs size: " << mmt.outputs_size();
			ds_in << "; outputs: ";
			::google::protobuf::RepeatedPtrField< ::dsp::trigger_output>::const_iterator coit;
			for (coit=mmt.outputs().begin();coit!=mmt.outputs().end();coit++)
				ds_in << " out[" << coit->out_id() << "]=" << coit->value();
			ds_in << endl;
		}

		if (0 == mmt.outputs_size()) {
			continue; //Триггер был передан, но не содержит ниодного изменившегося выхода
		}

		int trig_id = mmt.id();
		CVirtTrigger* vt = virtuals[trig_id];
		//Проверяем, что триггер с таким id зарегистрирован
		//FIXME Присвоение vt выполняется до проверки диапазонов
		if (0>trig_id || (unsigned)trig_id >= virtuals.size() || NULL == vt) {
			cout << "CNetLink::process_trig_msg atacked - trigger id is unregistered" << endl;
			exit(4);
		}

		::google::protobuf::RepeatedPtrField< ::dsp::trigger_output>::const_iterator coit;
		for (coit=mmt.outputs().begin();coit!=mmt.outputs().end();coit++) {
			vt->SetOutput(coit->out_id(), coit->value());
		}
	}
	//cout << "CNetLink::process_trig_msg info - ready" << endl;
	return 0;
}

int CNetLink::process_time_msg(const ::dsp::time_message& tm) {
	long long new_time = tm.current_time();
	ds_in << "CNetLink::process_time_msg info - remote time is " << new_time << endl;
	if (new_time < fsm->GetCurrentTime()) {
		cout << "CNetLink::process_time_msg atacked - time " << new_time << " is less than current time " << fsm->GetCurrentTime() << endl;
		exit(4);
	}
	fsm->RunToTime(fsm->ScaleRemoteTime(new_time));
	return 0;
}

int CNetLink::process_samplerate_msg(const ::dsp::samplerate_message& srtm) {
	ds_in << "CNetLink::process_samplerate_msg info - remote samplerate is " << srtm.samplerate() << endl;
	fsm->SetRemoteSamplerate(srtm.samplerate());
	return 0;
}

void CNetLink::SendTextResponse(std::string response_text) {
	cout << response_text << endl;
	//NetlinkMessageString* nms = new NetlinkMessageString(response_text);
	//FIXME Оранизовать отправку через p2vera и определиться, в каких случаях должна применяться эта функция
	//Send(nms);
	//delete nms;
}

bool CNetLink::connect_client(P2VeraStream p2s_c2s, P2VeraStream p2s_s2c) {
	if (enabled) return false;
	enabled = true;
	this->p2s_c2s = p2s_c2s;
	this->p2s_s2c = p2s_s2c;
	pthread_t thread_id;
	pthread_create (&thread_id, NULL, &handle_network_thread, this);
	return true;
}

bool CNetLink::disconnect_client() {
	enabled = false;
	return true;
}

int CNetLink::RegisterVirtualTrigger(CVirtTrigger* trigger) {
	if (NULL == trigger) {
		cout << "CNetLink::RegisterVirtualTrigger error - null pointer to trigger" << endl;
		return 1;
	}

	unsigned int prev_size = virtuals.size();
	if (trigger->trigger_id >= (int)virtuals.size()) {
		for (unsigned int i=prev_size;i<virtuals.size();i++) {
			virtuals[i] = NULL; //Помечаем все указатели нулями, чобы потом знать что они не использовались
 		}
		virtuals.resize(trigger->trigger_id + 1);
	} else {
		//Триггер попадает в диапазон зарегистрированных триггеров
		//Проверяем, что он не был зарегистрирован раньше
		if (NULL != virtuals[trigger->trigger_id]) {
			cout << "CNetLink::RegisterVirtualTrigger warning - trigger " << trigger->szTriggerName << " was already registered" << endl;
			return 0;
		}
	}

	//Проверяем, что триггера с таким именем не было зарегистрировано раньше
	if (by_name.find(trigger->szTriggerName) != by_name.end()) {
		cout << "CNetLink::RegisterVirtualTrigger warning - trigger " << trigger->szTriggerName << " was already registered" << endl;
		return 0;
	}

	virtuals[trigger->trigger_id] = trigger;
	by_name[trigger->szTriggerName] = trigger;

	return 0;
}

int CNetLink::MkDump(bool enable) {
	if (dump_enabled) {
		//Дамп и так выводится. Закрываем существующий дескрптор
		if (dump_stream != cout) {
			dump_stream.close();
		}
	}
	dump_enabled = enable;
	dump_to_file = false;
	return 0;
}

int CNetLink::MkDump(bool enable, string file_name) {
	if (dump_enabled) {
		//Дамп и так выводится. Закрываем существующий дескриптор
		if (dump_stream != cout) {
			dump_stream.close();
		}
	}

	dump_enabled = enable;
	if (dump_enabled) {
		if ("cout" == file_name) {
			dump_to_file = false;
		} else {
			dump_to_file = true;
			dump_stream.open(file_name.c_str());
		}
	}
	return 0;
}


int CNetLink::TclHandler(Tcl_Interp* interp, int argc, CONST char *argv[]) {
	InterpResultStream irs(interp);
	if (3<=argc) {
		string cmd_op = argv[1];
		string cmd_pr = argv[2];
		if ("dump_instream" == cmd_op) {
			if ("enable" == cmd_pr) {
				ds_in.set_stdout();
				irs << irs_clear << "ok" << irs_apply;
				return TCL_OK;
			} else if ("disable" == cmd_pr) {
				ds_in.disable();
				irs << irs_clear << "ok" << irs_apply;
				return TCL_OK;
			} else {
				irs << irs_clear << "netlink dump_instream error - options on/off" << irs_apply;
				return TCL_ERROR;
			}
		}
		if ("dump_ostream" == cmd_op) {
			bool bdump_enable = false;
			if (cmd_pr == "enable") {
				bdump_enable = true;
			}

			bool bfile_name = false;
			string file_name = "";
			if (argc > 3 ) {
				bfile_name = true;
				file_name = argv[3];
			}

			if (!bfile_name) {
				ds_out.set_stdout();
			} else {
				ds_out.set_file(file_name);
			}
			irs << irs_clear << "ok" << irs_apply;
			return TCL_OK;
		}
	}
	irs << irs_clear << "unknown command operands" << irs_apply;
	return TCL_ERROR;
}


CNetLink *net_link = NULL;


void* handle_network_thread(void* thread_arg) {
	CNetLink* nl = (CNetLink*) thread_arg;
	nl->handle_network();
	return NULL;
}

