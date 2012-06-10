/*
 * net_find_handlers.h
 *
 *  Created on: 16.05.2012
 *      Author: drozdov
 */

#ifndef NET_FIND_HANDLERS_H_
#define NET_FIND_HANDLERS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/time.h>

#include <pthread.h>
#include "net_find_ifaces.h"

class IRemoteNfServer;

//Класс - обработчик сообщений проверки связи
class NetFindLinkHandler : public INetFindMsgHandler {
public:
	NetFindLinkHandler(NetFind* nf, int msg_id);
	virtual ~NetFindLinkHandler();
	virtual bool HandleMessage(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr);
	virtual bool InvokeRequest(); //Отправка вещательного запроса для поиска лидеров в ЛВС
	virtual bool InvokeRequest(IRemoteNfServer *remote_server); //Отправка запроса на выбранный сервер

	virtual int get_msg_id();
private:
	bool handle_request(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr);
	bool handle_response(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr);

	virtual bool cmp_addr(sockaddr_in& sa_1, sockaddr_in& sa_2);
	virtual void load_ifinfo();
	virtual bool is_localhost(sockaddr_in& sa); //Проверят, относится ли ip адрес к этому компьютеру

	std::vector<sockaddr_in> local_ips; //Массив локальных ip адресов

	NetFind* nf;
	pthread_mutex_t mtx;
	int nmsg_index;

	int rq_socket;
	int rq_bkst_socket;
	sockaddr_in rq_bkst_addr;


	timeval tv_broadcast_request; //Время совершения последнего вещательного запроса
	unsigned int broadcast_time_delta; //Время между вещательными запросами. Может меняться в зависимости от текущего состояния системы
};

//Класс - обработчик сообщений сбора информации о доступных узлах
class NetFindInfoHandler : public INetFindMsgHandler {
public:
	NetFindInfoHandler(NetFind* nf, int msg_id);
	virtual ~NetFindInfoHandler();
	virtual bool HandleMessage(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr);
	virtual bool InvokeRequest(IRemoteNfServer *remote_server); //Отправка запроса на выбранный сервер
	virtual int get_msg_id();
private:
	bool handle_request(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr);
	bool handle_response(p2vera::msg_wrapper* wrpr, sockaddr_in* remote_addr);

	NetFind* nf;
	pthread_mutex_t mtx;
	int nmsg_index;

	int rq_socket;
};

#endif /* NET_FIND_HANDLERS_H_ */
