/*
 * net_link_handlers.cpp
 *
 *  Created on: 16.05.2012
 *      Author: drozdov
 */

#include "net_find_handlers.h"


NetFindLinkHandler::NetFindLinkHandler(NetFind* nf, int msg_id) {
}

NetFindLinkHandler::~NetFindLinkHandler() {
}

bool NetFindLinkHandler::HandleMessage(p2vera::msg_wrapper* wrpr) {
	return false;
}


NetFindInfoHandler::NetFindInfoHandler(NetFind* nf, int msg_id) {
}

NetFindInfoHandler::~NetFindInfoHandler() {
}

bool NetFindInfoHandler::HandleMessage(p2vera::msg_wrapper* wrpr) {
	return false;
}


