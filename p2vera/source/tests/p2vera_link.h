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
private:
	std::string str;
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
private:
	std::string str;
};

class AsrP2vFlush : public IP2VeraMessage  {
public:
	AsrP2vFlush();
	virtual ~AsrP2vFlush();

	virtual bool get_data(std::string& str) const;
	virtual int get_data(void* data, int max_data_size) const;
	virtual int get_data_size() const;
	virtual bool set_data(void* data, int data_size);
	virtual bool set_data(std::string &str);
};

class AsrP2Stream : public P2VeraStream {
public:
	AsrP2Stream();
	AsrP2Stream(IP2VeraStreamQq* qq);
	AsrP2Stream(const P2VeraStream& pvis);
	virtual ~AsrP2Stream();
	virtual AsrP2Stream& operator=(const AsrP2Stream& pvis);

	virtual AsrP2Stream& operator<<(AsrMsg1& p2m);
	virtual AsrP2Stream& operator>>(AsrMsg1& p2m);

	virtual AsrP2Stream& operator<<(AsrMsg2& p2m);
	virtual AsrP2Stream& operator>>(AsrMsg2& p2m);

	virtual AsrP2Stream& operator<<(AsrP2vFlush& flush);
};

extern AsrP2vFlush asr_fl;

#endif /* P2VERA_LINK_H_ */
