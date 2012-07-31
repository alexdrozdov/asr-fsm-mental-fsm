/*
 * net_find.h
 *
 *  Created on: 14.05.2012
 *      Author: drozdov
 */

#ifndef NET_FIND_H_
#define NET_FIND_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#include <sys/time.h>

#include <string>
#include <vector>
#include <map>
#include <list>

#include "net_find_ifaces.h"
#include "net_find_handlers.h"

#include "inet_find.h"
#include "mtx_containers.hpp"

#include "p2stream.h"

#define MAX_FAILURES_COUNT 5
#define MAX_PING_RESP_TIMEOUT 1

class NetFind;
class NetFindLinkHandler;

void* nf_server_rsp_thread_fcn (void* thread_arg);
void* nf_client_thread_fcn (void* thread_arg);

typedef struct _rmt_ping {
	int ping_id;
	struct timeval ping_send_time;

	_rmt_ping();
	_rmt_ping(const _rmt_ping& rmtp);
	_rmt_ping& operator=(const _rmt_ping& rmtp);
} rmt_ping;

typedef struct _remote_endpoint {
	RemoteSrvUnit rsu;
	int remote_port;
} remote_endpoint;

typedef struct _stream_full_cfg {
	stream_config stream_cfg; //Параметры соединения
	sockaddr_in local_sa;     //Локальный адрес порта, на котором это соединение прослушивается
	int port;                 //Порт (частично повторяет данные из local_sa)
	int sock_fd;              //Сокет этого соединения
	std::list<remote_endpoint> remote_endpoints; //Удаленные сервера, с которыми может быть установлено соединение этим потоком
	IP2VeraStreamHub* sh;
} stream_full_cfg;


//Класс обеспечивает поиск приложений, работающих в одной сети с ним на одном порту
//На локальной машине может выполнять функции клиента или сервера с переходом в режим
//сервера в случае падения предыдущего сервера.
class NetFind : public INetFind {
public:
	NetFind(net_find_config *nfc);
	virtual bool is_server(); //Позволяет проверить текущий режим этой копии библиотеки
	virtual int add_scanable_server(std::string address, std::string port);
	virtual int add_broadcast_servers(std::string port); //Автоматическое обнаружение серверов, отвечающих на вещательные запросы
	virtual int add_discovered_server(sockaddr_in& addr, std::string uniq_id);

	virtual void remove_remote_server(int id); //Удаление сервера из списка. Сервер может быть снова найден и получит новый id
	virtual void print_servers();              //Вывод в консоль списка известрных серверов с их статусами

	virtual void get_alive_servers(std::list<RemoteSrvUnit>& srv_list);
	virtual RemoteSrvUnit get_by_sockaddr(sockaddr_in& sa);
	virtual RemoteSrvUnit get_by_uniq_id(std::string uniq_id);

	virtual std::string get_uniq_id();
	virtual std::string get_name();
	virtual std::string get_caption();
	virtual std::string get_cluster();

	virtual unsigned int get_server_port();
	virtual unsigned int get_client_port();

	virtual bool is_localhost(sockaddr_in& sa); //Проверят, относится ли ip адрес к этому компьютеру

	virtual int register_stream(_stream_config& stream_cfg, IP2VeraStreamHub* sh);   //Зарегистрировать новый поток сообщений, по которому возможно взаимодействие

	virtual std::list<stream_full_cfg>::const_iterator streams_begin();
	virtual std::list<stream_full_cfg>::const_iterator streams_end();

	friend void* nf_server_rsp_thread_fcn (void* thread_arg);
	friend void* nf_client_thread_fcn (void* thread_arg);
private:
	std::list<IRemoteNfServer*> remote_servers;           //Массив удаленных серверов
	std::map<unsigned long long, IRemoteNfServer*> m_sa_servers;   //Массив серверов, проиндексированный по sockaddr
	std::map<std::string, IRemoteNfServer*> m_str_servers;  //Массив серверов, проиндексированный по уникальным идентификаторам

	std::list<IRemoteNfServer*> ping_list;   //Список серверов, которые должны опрошены в этой итерации
	std::list<IRemoteNfServer*> remove_list; //Список серверов, ожидающих удаления
	mtx_queue<IRemoteNfServer*> info_list;   //Список серверов, о которых необходимо получить информацию

	int last_remote_id;
	pthread_mutex_t mtx;

	sockaddr_in ownaddr;
	int sock_srv_rsp;

	sockaddr_in clientaddr;
	int sock_clt_rsp;

	unsigned int server_port;
	unsigned int client_port;

	bool is_local_server; //Признак того, что эта копия библиотеки захватила основной порт и выполняет роль сервера
	std::vector<sockaddr_in> local_ips; //Массив локальных ip адресов

	int generate_remote_id();     //Формирование уникальных идентификаторв для найденных серверов
	int server_response_thread(); //Поток сервера. Формирует ответы на запросы клиентов
	int server_route_thread();    //Поток сервера. Обеспечивает построение маршрутов (пока не уверен)
	int client_thread();          //Поток клиента.

	bool bind_client_port();
	bool receive_udp_message(int socket);

	void invoke_requests();
	void review_remote_servers();
	void unlink_server(IRemoteNfServer* irnfs); //Начать удаление сервера

	virtual void load_ifinfo();
	void reg_to_sockaddr(sockaddr_in& sa, IRemoteNfServer* rnfs);   //Привязка сервера к его адресу


	std::map<int, INetFindMsgHandler*> msg_handlers; //Обработчики сетевых сообщений, приходящих по протоколу UDP

	NetFindLinkHandler *link_handler;
	NetFindInfoHandler *info_handler;
	NetFindListHandler *list_handler;

	std::list<stream_full_cfg> streams;
	std::list<stream_full_cfg>::iterator find_stream(std::string name);

	std::string name;
	std::string caption;
	std::string cluster;
	std::string hash;
};


#endif /* NET_FIND_H_ */
