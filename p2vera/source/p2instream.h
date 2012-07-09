/*
 * p2instream.h
 *
 *  Created on: 09.07.2012
 *      Author: drozdov
 */

#ifndef P2INSTREAM_H_
#define P2INSTREAM_H_

#include <queue>

#include "p2stream.h"

class P2VeraInStreamQq; //Класс, реализующий очередь потока. Является уникальным для потока, возвращаемого
                        //p2vera. При дальнейших копированиях использующих его потоколв не размножается

class P2VeraInStream : public IP2VeraStream {
public:
	P2VeraInStream();
	P2VeraInStream(const P2VeraInStream& pvis);
	virtual ~P2VeraInStream();
	virtual IP2VeraStream& operator<<(IP2VeraMessage& p2m);
	virtual IP2VeraStream& operator>>(IP2VeraMessage& p2m);
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
private:
	P2VeraInStreamQq* pqq; //Указатель на "настоящий" поток.
};

class P2VeraInStreamQq {
public:
	bool is_opened();
	bool is_connected();
	void close();
	void flush();
	void discard_incomming();
	void discard_outcomming();
	bool has_incomming();
	bool has_outcomming();
	stream_direction get_stream_direction();
	int get_fd();
	int pop_message(IP2VeraMessage& p2m);
private:
	P2VeraInStreamQq();
	P2VeraInStreamQq(const P2VeraInStreamQq& tmplt);
	virtual ~P2VeraInStreamQq();
};

#endif /* P2INSTREAM_H_ */
