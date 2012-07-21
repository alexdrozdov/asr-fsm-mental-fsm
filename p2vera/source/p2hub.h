/*
 * p2hub.h
 *
 *  Created on: 15.07.2012
 *      Author: drozdov
 */

#ifndef P2HUB_H_
#define P2HUB_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#include <list>

#include "p2stream.h"

class P2VeraStreamHub : public IP2VeraStreamHub {
public:
	P2VeraStreamHub(std::string name);
	virtual ~P2VeraStreamHub();

	virtual P2VeraStream create_stream();                  //Создает новый экземляр двунаправленного потока, подсоединенного к хабу.
	virtual P2VeraStream create_instream();                //Создает новый экземляр входящего потока, подсоединенного к хабу.
	virtual P2VeraStream create_outstream();               //Создает новый экземляр исходящего потока, подсоединенного к хабу.
	virtual void unlink_stream(P2VeraStream p2s);          //Отсоединяет существующий экземпляр потока. После того, как последний клиент отсоединится от его очереди поток будет автоматически удален
	virtual bool send_message(IP2VeraMessage& p2m);        //Отправляет сообщение в сеть и другим подсоединенным хабам при необходимости
	virtual bool add_message_target(RemoteSrvUnit rsu);    //Добавляет удаленный сервер, который нуждается в получении сообщений от этого хаба
	virtual bool remove_message_target(RemoteSrvUnit rsu); //Удаляет сервер. Хаб должен прекратить передачу сообщений этому серверу

private:
	pthread_mutex_t mtx;
	std::list<P2VeraStream> qqs;
};


#endif /* P2HUB_H_ */
