/*
 * nf_bkst_server.h
 *
 *  Created on: 02.06.2012
 *      Author: drozdov
 */

#ifndef NF_BKST_SERVER_H_
#define NF_BKST_SERVER_H_

#include "net_find.h"

//Представляет собой все машины в сети, способные откликаться на вещательные запросы
class BkstNfServer : public IRemoteNfServer {
public:
	BkstNfServer(int id, net_find_config* rnfc);
	virtual bool is_alive();      //Бессмысленная функция, т.к. не относится к конкретной машине. Возвращает true
	virtual void forbide_usage(); //Запрещает рассылку вещательных запросов
	virtual void enable_usage();  //Разрешает рассылку вещательных запросов
	virtual int get_id();         //Получение идентификатора сервера в пределах этого приложения
	virtual std::string get_uniq_id();   //Бессмысленная функция, т.к. не относится к конкретной машине
	virtual sockaddr_in& get_remote_sockaddr();

	virtual void add_ping_request(int ping_id);  //Регистрация отправленного запроса
	virtual void add_ping_response(int ping_id); //Регистрация принятого ответа
	virtual void validate_alive();               //Проверка на количество потеряных запросов-ответов за единицу времени
	virtual bool ping_allowed();
	virtual void add_alternate_addr(sockaddr_in& alt_addr);
	virtual bool validate_addr(sockaddr_in& alt_addr);
	virtual bool requires_info_request();
	virtual bool is_broadcast();
	virtual bool is_localhost();
	virtual void print_info();
private:
	bool enabled;
	pthread_mutex_t mtx;

	unsigned int min_ping_time_delta;             //Минимальный период опроса

	int local_id;
	sockaddr_in remote_addr;

	timeval tv_request;
};

#endif /* NF_BKST_SERVER_H_ */
