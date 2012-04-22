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


using namespace std;

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
	unsigned char* pnt = space;

	netlink_string_msg *pmsg_ptr = (netlink_string_msg*)pnt;
	pmsg_ptr->msg_type   = nlmt_text;
	pmsg_ptr->msg_length = RequiredSize();
	pmsg_ptr->start_time = start_time;
	pmsg_ptr->duration   = duration;

	char *text_ptr = (char*)(((char*)pmsg_ptr)+sizeof(netlink_string_msg));
	strncpy(text_ptr, msg_text.c_str(), msg_text.length());
	text_ptr[msg_text.length()] = 0;
}

void NetlinkMessageString::Clear() {
	msg_text = "";
}
