/*
 * p2vera.h
 *
 *  Created on: 14.05.2012
 *      Author: drozdov
 */

#ifndef P2VERA_H_
#define P2VERA_H_

#include <pthread.h>

#include <string>
#include <map>

#include "inet_find.h"
#include "p2stream.h"

#define MD5_DIGEST_LENGTH 16

class P2Vera {
public:
	P2Vera();
	virtual void register_stream(stream_config& stream_cfg);  //Регистрация потока. Все потоки должны быть зарегистрированы до начала сетевого обмена
	virtual void start_network();                             //Запуск сетевого взаимодействия. Любые настройки после запуска обмена становятся бесполезными
	virtual P2VeraStream create_stream(std::string name);     //Создание двунаправленного потока обмена
	virtual P2VeraStream create_instream(std::string name);   //Создание потока входящих сообщений
	virtual P2VeraStream create_outstream(std::string name);  //Создание потока исходящих сообщений

	virtual std::string get_uniq_id();
private:
	char md5_data[MD5_DIGEST_LENGTH];
	virtual void generate_uniq_id();
	virtual void create_netfind();
	std::string uniq_id;
	INetFind* nf;

	bool networking_active;
	pthread_mutex_t mtx;
	std::map<std::string, IP2VeraStreamHub*> hubs;
};

#endif /* P2VERA_H_ */
