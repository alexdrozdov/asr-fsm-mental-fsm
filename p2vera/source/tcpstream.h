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
#include <string>

#include "p2stream.h"
#include "inet_find.h"

void* tcp_accept_thread_fcn (void* thread_arg);
void* tcp_poll_thread_fcn (void* thread_arg);

//Класс обеспечивает передачу данных по протоколу Tcp между двумя серверами.
//В один Tcp-канал мультиплексируются все потоки, которыми могут обмениваться сервера
//Класс создается при обнаружение удаленного сервера, имеющего общие каналы
class TcpStream {
public:
	TcpStream(INetFind* nf, RemoteSrvUnit& rsu, int remote_port);
	TcpStream(INetFind* nf, int socket);
	virtual ~TcpStream();
	virtual void add_hub(IP2VeraStreamHub* p2h); //Добавление потока по его идентификатору
	virtual void send_message(IP2VeraMessage& p2m, IP2VeraStreamHub* p2h); //Передача сообщения от лица указанного хаба. Сообщение будет смаршрутизировано в нужный поток
	virtual bool receive_message();
	virtual int get_fd();
private:
	int process_buffer(unsigned char* buf, int len);
	int process_message(unsigned char* buf, int len);
	bool send_data(std::string data);          //Упаковка данных в стартовый и стоповый байты и передача по сети
	RemoteSrvUnit rsu;
	INetFind* net_find;
	int remote_port;
	int fd;
	int last_flow_id;

	std::map<int, IP2VeraStreamHub*> flow_to_hub;
	std::map<IP2VeraStreamHub*, int> hub_to_flow;

	int buf_proc_state;
	unsigned char *cur_buf; //Текущий буфер для декодируемого сообщения
	unsigned char *pcur_buf;
	int cur_buf_usage; //Количество использованных ячеек буфера

	int coded_length[256];
};

struct rsu_fd_item {
	RemoteSrvUnit rsu;
	TcpStream *tcp_stream;
	int fd;
};

//Класс управляет tcp-соединениями. Соединения с вновь обнаруженными серверами устанавливаются средствами
//этого класса, также класс обрабатывает входящие запросы на соединение от других серверов
class TcpStreamManager {
public:
	TcpStreamManager(INetFind* nf, int start_port);
	void add_server(RemoteSrvUnit& rsu, int remote_port); //Добавить удаленный сервер и попытаться установить с ним соединение
	void remove_server(RemoteSrvUnit& rsu);
	int get_fd();
	int get_port();

	TcpStream* find_stream(RemoteSrvUnit rsu);

	friend void* tcp_accept_thread_fcn (void* thread_arg);
	friend void* tcp_poll_thread_fcn (void* thread_arg);

private:
	void rcv_thread();
	int accept_thread();
	INetFind* net_find;
	std::list<rsu_fd_item> rsu_fd_items; //Список удаленных серверов и их файловых дескрипторов, на которые необходимо получать данные
	int last_rsu_fd_count;
	bool rsu_fd_list_modified; //Признак того, что список серверов был изменен. Требуется перестроить список poll_fd
	pthread_mutex_t mtx;

	int accept_port;
	int sock_listen;

};

#endif /* TCPSTREAM_H_ */
