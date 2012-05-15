/*
 * net_find_handlers.h
 *
 *  Created on: 16.05.2012
 *      Author: drozdov
 */

#ifndef NET_FIND_HANDLERS_H_
#define NET_FIND_HANDLERS_H_

#include "net_find_ifaces.h"

//Класс - обработчик сообщений проверки связи
class NetFindLinkHandler : public INetFindMsgHandler {
public:
	NetFindLinkHandler(NetFind* nf, int msg_id);
	virtual ~NetFindLinkHandler();
	virtual bool HandleMessage(p2vera::msg_wrapper* wrpr);
private:
	NetFind* nf;
};

//Класс - обработчик сообщений сбора информации о доступных узлах
class NetFindInfoHandler : public INetFindMsgHandler {
public:
	NetFindInfoHandler(NetFind* nf, int msg_id);
	virtual ~NetFindInfoHandler();
	virtual bool HandleMessage(p2vera::msg_wrapper* wrpr);
private:
	NetFind* nf;
};

#endif /* NET_FIND_HANDLERS_H_ */
