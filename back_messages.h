/*
 * back_messages.h
 *
 *  Created on: 22.04.2012
 *      Author: drozdov
 */

#ifndef BACK_MESSAGES_H_
#define BACK_MESSAGES_H_

#include <string>

#include "net_link.h"

/*typedef struct _netlink_string_msg {
	int msg_type;
	int msg_length;
	long long start_time;
	long long duration;
} netlink_string_msg, *pnetlink_string_msg;

class NetlinkMessageString : public NetlinkMessage {
public:
	NetlinkMessageString(std::string text, long long start_time = -1, long long duration = -1);
	~NetlinkMessageString();

	int RequiredSize();
	void Dump(unsigned char* space);
	void Clear();
private:
	std::string msg_text;
	long long start_time;
	long long duration;
};

*/

#endif /* BACK_MESSAGES_H_ */
