/*
 * back_messages.cpp
 *
 *  Created on: 22.04.2012
 *      Author: drozdov
 */


#include <string>
#include <iostream>

#include <string.h>

#include "back_messages.h"
#include "mental_fsm.h"
#include "global_vars.h"


using namespace std;
/*
NetlinkMessage::~NetlinkMessage() {
}

NetlinkMessageString::NetlinkMessageString(string text, long long start_time, long long duration) {
	msg_text = text;
	if (-1 == start_time) {
		this->start_time = fsm->GetCurrentTime();
	}
	if (duration < 0 && duration!=-1) {
		cout << "NetlinkMessageString::NetlinkMessageString error - negative duration for message \"" << text << "\"" << endl;
	}
	this->duration = duration;
}

NetlinkMessageString::~NetlinkMessageString() {
}

int NetlinkMessageString::RequiredSize() {
	int nsize = sizeof(netlink_string_msg);
	nsize += msg_text.length() + 1;
	return nsize;
}

void NetlinkMessageString::Dump(unsigned char* space) {
}

void NetlinkMessageString::Clear() {
	msg_text = "";
}
*/
