/*
 * tcpstream.h
 *
 *  Created on: 06.08.2012
 *      Author: drozdov
 */

#ifndef TCPSTREAM_H_
#define TCPSTREAM_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <map>
#include <list>

#include "p2stream.h"
#include "inet_find.h"

//Класс обеспечивает передачу данных по протоколу Tcp между двумя серверами.
//В один Tcp-канал мультиплексируются все потоки, которыми могут обмениваться сервера
//Класс создается при обнаружение удаленного сервера, имеющего общие каналы
class TcpStream {
public:
	TcpStream(RemoteSrvUnit& rsu, int remote_port);
	virtual ~TcpStream();
	virtual void add_hub(IP2VeraStreamHub* p2h, int flow_id); //Добавление потока по его идентификатору
	virtual void start_network();
private:
	RemoteSrvUnit rsu;
	int remote_port;
	int fd;

	std::map<int, IP2VeraStreamHub*> flow_to_hub;
	std::map<IP2VeraStreamHub*, int> hub_to_flow;
};


#endif /* TCPSTREAM_H_ */
