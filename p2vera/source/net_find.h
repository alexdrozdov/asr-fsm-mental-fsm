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

#include <string>
#include <vector>
#include <map>

#include "net_find_ifaces.h"

#define MAX_FAILURES_COUNT 5
#define MAX_PING_RESP_TIMEOUT 1

class NetFind;
void* nf_server_rsp_thread_fcn (void* thread_arg);
void* nf_client_thread_fcn (void* thread_arg);

//Конфигурация модуля поиска приложений в сети
typedef struct _net_find_config {
	std::string nf_name;    // Кодовое название этой части приложения
	std::string nf_caption; // Полное  название этой части приложения
	std::string nf_address; // IP адрес, на котором обнаружен сервер
	std::string nf_port;    // Порт, на котором должен запускаться сервер
	std::string nf_hash;    // Идентификатор экземпляра приложения
	int usage;              // Степень использования этого приложения. Меньше - лучше.
	bool scanable;          // Необходимость пинга сервера. При ложном значении сервер считается
	                        // работоспособным, его пинг запрещен
} net_find_config;

typedef struct _rmt_ping {
	int ping_id;
	struct timeval ping_send_time;
} rmt_ping;

//Сервер, расположенный на другой машине
class RemoteNfServer {
public:
	RemoteNfServer(int id, net_find_config* rnfc);
	bool is_alive();      //Проверить возможность установки связи с этим приложением.
	                      //Не гарантирует жизнеспособность приложения в текущий момент,
	                      //опирается на наличие ответов в недавнем прошлом.
	void forbide_usage(); //Запретить проверку этого приложения на жизнеспособность,
	                      //принудительно считать приложение отсутствующим.
	                      //Т.к. на удаленной машине существует только один сервер с указаным
	                      //портом, машины различаются только ip-адресами. Поэтому блокировка
	                      //одного сервера означает полную блокировку удаленной машины
	void enable_usage();  //Разрешает работу с этим сервером, т.е. с любым сервером,
	                      //расположенным на этой удаленной машине

	friend class NetFind;
private:
	int failure_count;
	bool enabled;
	pthread_mutex_t mtx;
	std::map<int, rmt_ping> pings_sent; //Список отправленных запросов, ожидающих ответа

	void add_ping_request(int ping_id);  //Регистрация отправленного запроса
	void add_ping_response(int ping_id); //Регистрация принятого ответа
	void validate_alive();               //Проверка на количество потеряных запросов-ответов за единицу времени
};

//Класс обеспечивает поиск приложений, работающих в одной сети с ним на одном порту
//На локальной машине может выполнять функции клиента или сервера с переходом в режим
//сервера в случае падения предыдущего сервера.
class NetFind {
public:
	NetFind(net_find_config *nfc);
	bool is_server(); //Позволяет проверить текущий режим этой копии библиотеки
	int add_scanable_server(std::string address, std::string port);
	int add_unscanable_server(std::string address, std::string port);

	RemoteNfServer* by_id(int id);     //Поиск сервера по его локальному id
	void remove_remote_server(int id); //Удаление сервера из списка. Сервер может быть снова найден и получит новый id
	void print_servers();              //Вывод в консоль списка известрных серверов с их статусами

	friend void* nf_server_rsp_thread_fcn (void* thread_arg);
	friend void* nf_client_thread_fcn (void* thread_arg);
private:
	std::vector<RemoteNfServer*> remote_servers;
	pthread_mutex_t mtx;

	sockaddr_in ownaddr;
	int sock_srv_rsp;

	int server_response_thread(); //Поток сервера. Формирует ответы на запросы клиентов
	int server_route_thread();    //Поток сервера. Обеспечивает построение маршрутов (пока не уверен)
	int client_thread();          //Поток клиента.

	std::map<int, INetFindMsgHandler*> msg_handlers; //Обработчики сетевых сообщений, приходящих по протоколу UDP
};


#endif /* NET_FIND_H_ */
