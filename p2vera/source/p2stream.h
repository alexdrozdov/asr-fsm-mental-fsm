/*
 * p2stream.h
 *
 *  Created on: 08.07.2012
 *      Author: drozdov
 */

#ifndef P2STREAM_H_
#define P2STREAM_H_

#include <string>
#include <vector>

#include "inet_find.h"

enum stream_direction {
	stream_direction_income   = 0,
	stream_direction_outcome  = 1,
	stream_direction_bidir    = 2,
	stream_direction_loopback = 3
};

enum stream_type {
	stream_type_dgram = 0, //Поток представлен отдельными сообщениями
	stream_type_flow  = 1  //Поток не разделен на сообщения
};

enum stream_msg_order {
	stream_msg_order_any = 0,    //Требования к порядку доставки сообщений не предъявляются
	stream_msg_order_chrono = 1, //Сообщения должны доставляться в хронологическом порядке, но могут пропадать
	stream_msg_order_strict = 2  //Сообщения должны доставляться полностью и в строго хронологическом порядке
};

class IP2VeraStreamQq;
class P2VeraBasicMessage;
class IP2VeraStreamHub;

typedef struct _stream_config {
	std::string name;           //Название потока
	stream_direction direction; //Направление потока со стороны внешней сети.
	stream_type type;           //Тип потока - UDP или TCP
	stream_msg_order order;     //Упорядочивание сообщений в потоке
} stream_config;

class IP2VeraMessage {
public:
	virtual ~IP2VeraMessage() = 0;
	virtual bool get_data(std::string& str) const = 0;
	virtual int get_data(void* data, int max_data_size) const = 0;
	virtual int get_data_size() const = 0;
	virtual bool set_data(void* data, int data_size) = 0;
	virtual bool set_data(std::string &str) = 0;
};

class P2VeraStream {
public:
	P2VeraStream();
	P2VeraStream(IP2VeraStreamQq* qq);
	P2VeraStream(const P2VeraStream& pvis);
	~P2VeraStream();
	virtual P2VeraStream& operator=(const P2VeraStream& pvis);
	virtual P2VeraStream& operator<<(IP2VeraMessage& p2m);
	virtual P2VeraStream& operator>>(IP2VeraMessage& p2m);
	virtual bool is_opened();
	virtual bool is_connected();
	virtual void close();
	virtual void flush();
	virtual void discard_incomming();
	virtual void discard_outcomming();
	virtual bool has_incomming();
	virtual bool has_outcomming();
	virtual stream_direction get_stream_direction();
	virtual int get_fd();
	virtual IP2VeraStreamQq* get_qq();
	virtual bool is_equal(const P2VeraStream& pvis);

	friend bool operator==(const P2VeraStream& ls, const P2VeraStream& rs);
private:
	IP2VeraStreamQq* qq;
};

bool operator==(const P2VeraStream& ls, const P2VeraStream& rs);

class IP2VeraStreamQq {
public:
	virtual ~IP2VeraStreamQq() = 0;

	virtual bool is_opened() = 0;
	virtual bool is_connected() = 0;
	virtual void close() = 0;
	virtual void flush() = 0;
	virtual void discard_incomming() = 0;
	virtual void discard_outcomming() = 0;
	virtual bool has_incomming() = 0;
	virtual bool has_outcomming() = 0;
	virtual stream_direction get_stream_direction() = 0;
	virtual int get_fd() = 0;
	virtual int pop_message(IP2VeraMessage& p2m) = 0;
	virtual int push_message(IP2VeraMessage& p2m) = 0;

	virtual bool increase_ref_count() = 0; //Увеличивает счетчик ссылок на экземпляр класса
	virtual int decrease_ref_count() = 0;  //Уменьшает счетчик ссылок на экземпляр класса
	virtual bool is_referenced() = 0;      //Позволяет проверить наличие ссылок на экземпляр и возможность его удаления

	virtual void _insert_message(P2VeraBasicMessage& p2m) = 0;
};

class IP2VeraStreamHub {
public:
	virtual ~IP2VeraStreamHub() = 0;
	virtual std::string get_name() = 0;                        //Возвращает название потока, для которого создан хаб
	virtual P2VeraStream create_stream() = 0;                  //Создает новый экземляр двунаправленного потока, подсоединенного к хабу.
	virtual P2VeraStream create_instream() = 0;                //Создает новый экземляр двунаправленного потока, подсоединенного к хабу.
	virtual P2VeraStream create_outstream() = 0;               //Создает новый экземляр двунаправленного потока, подсоединенного к хабу.
	virtual void unlink_stream(P2VeraStream p2s) = 0;          //Отсоединяет существующий экземпляр потока
	virtual bool send_message(IP2VeraMessage& p2m) = 0;        //Отправляет сообщение в сеть и другим подсоединенным хабам при необходимости
	virtual bool receive_message(IP2VeraMessage& p2m) = 0;     //Отправляет сообщение в сеть и другим подсоединенным хабам при необходимости
	virtual bool add_message_target(RemoteSrvUnit rsu, int port) = 0; //Добавляет удаленный сервер, который нуждается в получении сообщений от этого хаба
	virtual bool remove_message_target(RemoteSrvUnit rsu) = 0; //Удаляет сервер. Хаб должен прекратить передачу сообщений этому серверу
};

#endif /* P2STREAM_H_ */
