/*
 * nf_remote_server.h
 *
 *  Created on: 26.05.2012
 *      Author: drozdov
 */

#ifndef NF_REMOTE_SERVER_H_
#define NF_REMOTE_SERVER_H_

#include "net_find.h"

//Сервер, расположенный на другой машине
class RemoteNfServer : public IRemoteNfServer {
public:
	RemoteNfServer(int id, net_find_config* rnfc);
	virtual bool is_alive();      //Проверить возможность установки связи с этим приложением.
	                      //Не гарантирует жизнеспособность приложения в текущий момент,
	                      //опирается на наличие ответов в недавнем прошлом.
	virtual void forbide_usage(); //Запретить проверку этого приложения на жизнеспособность,
	                      //принудительно считать приложение отсутствующим.
	                      //Т.к. на удаленной машине существует только один сервер с указаным
	                      //портом, машины различаются только ip-адресами. Поэтому блокировка
	                      //одного сервера означает полную блокировку удаленной машины
	virtual void enable_usage();  //Разрешает работу с этим сервером, т.е. с любым сервером,
	                      //расположенным на этой удаленной машине

	virtual int get_id(); //Получение идентификатора сервера в пределах этого приложения
	virtual std::string get_uniq_id();   //Получение уникального идентификатора
	virtual sockaddr_in& get_remote_sockaddr();

	virtual void add_ping_request(int ping_id);  //Регистрация отправленного запроса
	virtual void add_ping_response(int ping_id); //Регистрация принятого ответа
	virtual void validate_alive();               //Проверка на количество потеряных запросов-ответов за единицу времени
	virtual bool ping_allowed();                 //Проверка возможности пинга. Пинг может быть запрещен, т.к. запрещено
										 //использование этого удаленного сервера или не прошел минимальный период ожидания
										 //с момента отправки последнего пинга.

	//friend class NetFind;
	//friend class NetFindLinkHandler;
private:
	int failure_count;
	bool enabled;
	pthread_mutex_t mtx;
	std::map<int, rmt_ping> pings_sent; //Список отправленных запросов, ожидающих ответа

	unsigned int min_ping_time_delta;             //Минимальный период опроса

	int local_id;
	std::string name;
	std::string caption;
	std::string uniq_id;
	sockaddr_in remote_addr;

	timeval tv_request;
};

#endif /* NF_REMOTE_SERVER_H_ */