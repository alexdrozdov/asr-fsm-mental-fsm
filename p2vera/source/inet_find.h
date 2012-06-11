/*
 * inet_find.h
 *
 *  Created on: 10.06.2012
 *      Author: drozdov
 */

#ifndef INET_FIND_H_
#define INET_FIND_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <list>

//Конфигурация модуля поиска приложений в сети
typedef struct _net_find_config {
	std::string nf_name;    // Кодовое название этой части приложения
	std::string nf_caption; // Полное  название этой части приложения
	std::string nf_address; // IP адрес, на котором обнаружен сервер
	std::string nf_port;    // Порт, на котором должен запускаться сервер
	std::string nf_hash;    // Идентификатор экземпляра приложения
	std::string nf_cluster; // Идентификатор кластера, к которому относится сервер
	int usage;              // Степень использования этого приложения. Меньше - лучше.
	bool scanable;          // Необходимость пинга сервера. При ложном значении сервер считается
	                        // работоспособным, его пинг запрещен
} net_find_config;

class IRemoteNfServer {
public:
	virtual bool is_alive() = 0;      //Проверить возможность установки связи с этим приложением.
						              //Не гарантирует жизнеспособность приложения в текущий момент,
						              //опирается на наличие ответов в недавнем прошлом.
	virtual void forbide_usage() = 0; //Запретить проверку этого приложения на жизнеспособность,
						              //принудительно считать приложение отсутствующим.
	virtual void enable_usage() = 0;  //Разрешает работу с этим сервером, т.е. с любым сервером,
						              //расположенным на этой удаленной машине

	virtual int get_id() = 0;         //Получение идентификатора сервера в пределах этого приложения
	virtual std::string get_uniq_id() = 0;   //Получение уникального идентификатора
	virtual sockaddr_in& get_remote_sockaddr() = 0;
	virtual void add_alternate_addr(sockaddr_in& alt_addr) = 0; //Добавить альтернативный адрес, по которому можно найти этот сервер
	virtual bool validate_addr(sockaddr_in& alt_addr) = 0; //Проверить переданный адрес на принадлежность к этому серверу

	virtual void add_ping_request(int ping_id) = 0;  //Регистрация отправленного запроса
	virtual void add_ping_response(int ping_id) = 0; //Регистрация принятого ответа
	virtual void validate_alive() = 0;               //Проверка на количество потеряных запросов-ответов за единицу времени
	virtual bool ping_allowed() = 0;                 //Проверка возможности пинга. Пинг может быть запрещен, т.к. запрещено
										 //использование этого удаленного сервера или не прошел минимальный период ожидания
										 //с момента отправки последнего пинга.

	virtual bool requires_info_request() = 0; //Сервер нуждается в сборе дополнительной информации

	virtual bool is_broadcast() = 0; // Сервер на самом деле не существует, а используется для хранения информации о вещательных запросах
	virtual bool is_localhost() = 0; // Сервер рабоает на тойже машине, что и эта копия библиотеки
	virtual void print_info() = 0;   // Вывести параметры сервера в консоль
};

class INetFind {
public:

	virtual bool is_server() = 0; //Позволяет проверить текущий режим этой копии библиотеки
	virtual int add_scanable_server(std::string address, std::string port) = 0;
	virtual int add_broadcast_servers(std::string port) = 0; //Автоматическое обнаружение серверов, отвечающих на вещательные запросы

	virtual IRemoteNfServer* by_id(int id) = 0;                    //Поиск сервера по его локальному id
	virtual IRemoteNfServer* by_sockaddr(sockaddr_in& sa) = 0;     //Поиск сервера по его обратному адресу
	virtual IRemoteNfServer* by_uniq_id(std::string uniq_id) = 0; //Поиск сервера по его уникальному идентификатору

	virtual void get_alive_servers(std::list<IRemoteNfServer*>& srv_list) = 0; //Заполняет список действующих серверов.

	virtual void print_servers() = 0;

	virtual std::string get_uniq_id() = 0;
	virtual std::string get_name() = 0;
	virtual std::string get_caption() = 0;
	virtual std::string get_cluster() = 0;
};

extern INetFind* net_find_create(net_find_config *nfc);

#endif /* INET_FIND_H_ */
