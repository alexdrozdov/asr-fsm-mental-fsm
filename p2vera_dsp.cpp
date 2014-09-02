/*
 * netlink_pack.cpp
 *
 *  Created on: 04.07.2011
 *      Author: drozdov
 */


#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/poll.h>
#include <sys/time.h>

#include <pthread.h>

#include <iostream>

#include "p2vera_dsp.h"

using namespace std;

DspMessage::~DspMessage() {
}


bool DspMessage::get_data(std::string& str) const {
	return pkg.SerializeToString(&str);
}

int DspMessage::get_data(void* data, int max_data_size) const {
	int required_size = pkg.ByteSize();
	if (required_size > max_data_size) return 0;
	pkg.SerializeToArray(data, required_size);
	return required_size;
}

int DspMessage::get_data_size() const {
	return pkg.ByteSize();
}

bool DspMessage::set_data(void* data, int data_size) {
	if (NULL == data) return false;
	return pkg.ParseFromArray(data, data_size);
}

bool DspMessage::set_data(std::string &str) {
	return pkg.ParseFromString(str);
}


/////////////////////////////////////////////////////////////////////////////////////////////



DspMessageTrig::DspMessageTrig() {
	pkg.add_modified_triggers_inst();
}

DspMessageTrig::~DspMessageTrig() {
}

void DspMessageTrig::Add(int trigger_id, int out_id, double value) {
	//Ищем триггер с указанным id
	dsp::modified_triggers *mt = pkg.mutable_modified_triggers_inst(0);
	dsp::modified_triger *mmt = NULL;
	::google::protobuf::RepeatedPtrField< ::dsp::modified_triger >::iterator it;
	for (it=mt->mutable_items()->begin();it!=mt->mutable_items()->end();it++){
		if (it->id() == trigger_id) {
			mmt = &(*it);
			break;
		}
	}
	if (NULL == mmt) {
		mmt = mt->add_items();
	}
	mmt->set_id(trigger_id);
	dsp::trigger_output* tro = mmt->add_outputs();
	tro->set_out_id(out_id);
	tro->set_value(value);
}

void DspMessageTrig::DspMessageTrig::Clear() {
	pkg.mutable_modified_triggers_inst(0)->Clear();
}

////////////////////////////////////////////////////////////////////


DspMessageTime::DspMessageTime(long long time) {
	pkg.add_time_inst()->set_current_time(time);
}

DspMessageTime::~DspMessageTime() {
}

void DspMessageTime::SetTime(long long time) {
	pkg.mutable_time_inst(0)->set_current_time(time);
}

void DspMessageTime::Clear() {
	pkg.mutable_time_inst(0)->Clear();
}
////////////////////////////////////////////////////////////////////

DspMessageSamplerate::DspMessageSamplerate(unsigned int samplerate) {
	pkg.add_samplerate_inst()->set_samplerate(samplerate);
}

DspMessageSamplerate::~DspMessageSamplerate() {
}

void DspMessageSamplerate::SetSamplerate(unsigned int samplerate) {
	pkg.mutable_samplerate_inst(0)->set_samplerate(samplerate);
}

void DspMessageSamplerate::Clear() {
	pkg.mutable_samplerate_inst(0)->Clear();
}

///////////////////////////////////////////////////////////////////////

DspMessageCmn::DspMessageCmn() {}

DspMessageCmn::~DspMessageCmn() {}

bool DspMessageCmn::has_trig() {
	return pkg.modified_triggers_inst_size() > 0;
}

bool DspMessageCmn::has_time() {
	return pkg.time_inst_size() > 0;
}

bool DspMessageCmn::has_samplerate() {
	return pkg.samplerate_inst_size() > 0;
}

void DspMessageCmn::Clear() {
	pkg.Clear();
}

DspMessageCmn& DspMessageCmn::operator>>(DspMessageTrig& dsp_trig) {
	dsp_trig.pkg.CopyFrom(pkg);
	return *this;
}

DspMessageCmn& DspMessageCmn::operator>>(DspMessageTime& dsp_time) {
	dsp_time.pkg.CopyFrom(pkg);
	return *this;
}

DspMessageCmn& DspMessageCmn::operator>>(DspMessageSamplerate& dsp_sr) {
	dsp_sr.pkg.CopyFrom(pkg);
	return *this;
}

const dsp::dsp_package& DspMessageCmn::dsp() const {
	return pkg;
}

