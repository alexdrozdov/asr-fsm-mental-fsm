/*
 * p2stream.h
 *
 *  Created on: 08.07.2012
 *      Author: drozdov
 */

#ifndef P2STREAM_H_
#define P2STREAM_H_

#include <string>

enum stream_direction {
	stream_direction_income   = 0,
	stream_direction_outcome  = 1,
	stream_direction_bidir    = 2,
	stream_direction_loopback = 3
};

class IP2VeraMessage {
public:
	virtual ~IP2VeraMessage()
	virtual bool get_data(std::string& str) = 0;
	virtual int get_data(void* data, int max_data_size) = 0;
	virtual int get_data_size() = 0;
};

class IP2VeraStream {
public:
	//IP2VeraStream(const IP2VeraStream& p2s);
	//IP2VeraStream(std::string stream_name);
	virtual ~IP2VeraStream() = 0;
	virtual IP2VeraStream& operator<<(IP2VeraMessage& p2m) = 0;
	virtual IP2VeraStream& operator>>(IP2VeraMessage& p2m) = 0;
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
};

enum stream_type {
	stream_type_dgram = 0, //Поток представлен отдельными сообщениями
	stream_type_flow  = 1  //Поток не разделен на сообщения
};

enum stream_msg_order {
	stream_msg_order_any = 0,
	stream_msg_order_
};

class P2VeraStreamHub {
public:
private:
	std::string stream_name;
	bool is_opened();

};


#endif /* P2STREAM_H_ */
