/*
 * p2message.h
 *
 *  Created on: 15.07.2012
 *      Author: drozdov
 */

#ifndef P2MESSAGE_H_
#define P2MESSAGE_H_

#include <string>

#include "p2stream.h"

class P2VeraTextMessage : public IP2VeraMessage {
public:
	P2VeraTextMessage();
	P2VeraTextMessage(std::string str);
	P2VeraTextMessage(const P2VeraTextMessage& tm);
	virtual ~P2VeraTextMessage();

	virtual bool get_data(std::string& str) const;
	virtual int get_data(void* data, int max_data_size) const;
	virtual int get_data_size() const;
	virtual bool set_data(void* data, int data_size);
	virtual bool set_data(std::string &str);

	virtual operator std::string();

	virtual void print();
private:
	std::string str;
};

class P2VeraBasicMessage : public IP2VeraMessage {
public:
	P2VeraBasicMessage();
	P2VeraBasicMessage(std::string str);
	P2VeraBasicMessage(const IP2VeraMessage& tm);
	virtual ~P2VeraBasicMessage();

	virtual bool get_data(std::string& str) const;
	virtual int get_data(void* data, int max_data_size) const;
	virtual int get_data_size() const;
	virtual bool set_data(void* data, int data_size);
	virtual bool set_data(std::string &str);

	virtual void print();
private:
	std::string str;
};


#endif /* P2MESSAGE_H_ */
