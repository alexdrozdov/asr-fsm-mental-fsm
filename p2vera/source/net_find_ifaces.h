/*
 * net_find_ifaces.h
 *
 *  Created on: 16.05.2012
 *      Author: drozdov
 */

#ifndef NET_FIND_IFACES_H_
#define NET_FIND_IFACES_H_

#include "msg_wrapper.pb.h"

class NetFind;

class INetFindMsgHandler {
public:
	virtual ~INetFindMsgHandler() = 0;
	virtual bool HandleMessage(p2vera::msg_wrapper* wrpr) = 0;
};

#endif /* NET_FIND_IFACES_H_ */
