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

#include "net_find_ifaces.h"
#include "net_find_handlers.h"

#define MAX_FAILURES_COUNT 5
#define MAX_PING_RESP_TIMEOUT 1

class NetFind;
class NetFindLinkHandler;

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

class IRemoteNfServer {
public:
	virtual bool is_alive() = 0;      //Проверить возможность установки связи с этим приложением.
						  //Не гарантирует жизнеспособность приложения в текущий момент,
						  //опирается на наличие ответов в недавнем прошлом.
	virtual void forbide_usage() = 0; //Запретить проверку этого приложения на жизнеспособность,
						  //принудительно считать приложение отсутствующим.
						  //Т.к. на удаленной машине существует только один сервер с указаным
						  //портом, машины различаются только ip-адресами. Поэтому блокировка
						  //одного сервера означает полную блокировку удаленной машины
	virtual void enable_usage() = 0;  //Разрешает работу с этим сервером, т.е. с любым сервером,
						  //расположенным на этой удаленной машине

	virtual int get_id() = 0; //Получение идентификатора сервера в пределах этого приложения
	virtual std::string get_uniq_id() = 0;   //Получение уникального идентификатора
	virtual sockaddr_in& get_remote_sockaddr() = 0;

	virtual void add_ping_request(int ping_id) = 0;  //Регистрация отправленного запроса
	virtual void add_ping_response(int ping_id) = 0; //Регистрация принятого ответа
	virtual void validate_alive() = 0;               //Проверка на количество потеряных запросов-ответов за единицу времени
	virtual bool ping_allowed() = 0;                 //Проверка возможности пинга. Пинг может быть запрещен, т.к. запрещено
										 //использование этого удаленного сервера или не прошел минимальный период ожидания
										 //с момента отправки последнего пинга.
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

	IRemoteNfServer* by_id(int id);     //Поиск сервера по его локальному id
	void remove_remote_server(int id); //Удаление сервера из списка. Сервер может быть снова найден и получит новый id
	void print_servers();              //Вывод в консоль списка известрных серверов с их статусами

	unsigned int get_server_port();
	virtual unsigned int get_client_port();
	std::string get_uniq_id();
	friend void* nf_server_rsp_thread_fcn (void* thread_arg);
	friend void* nf_client_thread_fcn (void* thread_arg);
private:
	std::vector<IRemoteNfServer*> remote_servers;
	int last_remote_id;
	pthread_mutex_t mtx;

	sockaddr_in ownaddr;
	int sock_srv_rsp;

	sockaddr_in clientaddr;
	int sock_clt_rsp;

	unsigned int server_port;
	unsigned int client_port;

	int generate_remote_id();     //Формирование уникальных идентификаторв для найденных серверов
	int server_response_thread(); //Поток сервера. Формирует ответы на запросы клиентов
	int server_route_thread();    //Поток сервера. Обеспечивает построение маршрутов (пока не уверен)
	int client_thread();          //Поток клиента.

	std::map<int, INetFindMsgHandler*> msg_handlers; //Обработчики сетевых сообщений, приходящих по протоколу UDP

	NetFindLinkHandler *link_handler;
	NetFindInfoHandler *info_handler;

	std::string name;
	std::string caption;
	std::string hash;
};


#endif /* NET_FIND_H_ */
