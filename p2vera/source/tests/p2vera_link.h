/*
 * p2vera_link.h
 *
 *  Created on: 30.04.2013
 *      Author: drozdov
 */

#ifndef P2VERA_LINK_H_
#define P2VERA_LINK_H_

#include <p2vera.h>
#include <p2message.h>
#include <p2stream.h>

#include "msg_multiplex.pb.h"

class AsrMsgCmn;

class AsrMsg1 : public IP2VeraMessage {
public:
	AsrMsg1();
	AsrMsg1(std::string str);
	AsrMsg1(const IP2VeraMessage& tm);
	virtual ~AsrMsg1();

	virtual bool get_data(std::string& str) const;
	virtual int get_data(void* data, int max_data_size) const;
	virtual int get_data_size() const;
	virtual bool set_data(void* data, int data_size);
	virtual bool set_data(std::string &str);

	virtual void print();

	friend class AsrMsgCmn;
private:
	msg_mtpx::msg_multiplex_package mmp;
};

class AsrMsg2 : public IP2VeraMessage {
public:
	AsrMsg2();
	AsrMsg2(std::string str);
	AsrMsg2(const IP2VeraMessage& tm);
	virtual ~AsrMsg2();

	virtual bool get_data(std::string& str) const;
	virtual int get_data(void* data, int max_data_size) const;
	virtual int get_data_size() const;
	virtual bool set_data(void* data, int data_size);
	virtual bool set_data(std::string &str);

	virtual void print();

	friend class AsrMsgCmn;
private:
	msg_mtpx::msg_multiplex_package mmp;
};


class AsrMsgCmn : public IP2VeraMessage {
public:
	AsrMsgCmn();
	AsrMsgCmn(std::string str);
	AsrMsgCmn(const IP2VeraMessage& tm);
	virtual ~AsrMsgCmn();

	virtual bool get_data(std::string& str) const;
	virtual int get_data(void* data, int max_data_size) const;
	virtual int get_data_size() const;
	virtual bool set_data(void* data, int data_size);
	virtual bool set_data(std::string &str);

	virtual AsrMsgCmn& operator>>(AsrMsg1& p2m);
	virtual AsrMsgCmn& operator>>(AsrMsg2& p2m);
	virtual bool has_m1();
	virtual bool has_m2();

	virtual void print();
private:
	msg_mtpx::msg_multiplex_package mmp;
};


#endif /* P2VERA_LINK_H_ */
