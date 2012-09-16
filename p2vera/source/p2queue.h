/*
 * p2queue.h
 *
 *  Created on: 15.07.2012
 *      Author: drozdov
 */

#ifndef P2QUEUE_H_
#define P2QUEUE_H_

#include <pthread.h>

#include <queue>

#include "p2stream.h"
#include "p2message.h"

class P2VeraStreamQq : public IP2VeraStreamQq {
public:
	P2VeraStreamQq(IP2VeraStreamHub *hub);
	virtual ~P2VeraStreamQq();

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
	virtual int pop_message(IP2VeraMessage& p2m);
	virtual int push_message(IP2VeraMessage& p2m);

	virtual bool increase_ref_count();
	virtual int decrease_ref_count();
	virtual bool is_referenced();
	virtual void _insert_message(P2VeraBasicMessage& p2m);
private:
	void create_poll_pipe();
	IP2VeraStreamHub *hub;
	pthread_mutex_t mtx;
	int ref_count;

	std::queue<P2VeraBasicMessage> in_qq;

	int fd;
	int pipe_fd[2];
	bool opened;
};

class P2VeraInStreamQq : public P2VeraStreamQq {
public:
	P2VeraInStreamQq(IP2VeraStreamHub *hub);
	virtual ~P2VeraInStreamQq();
	virtual stream_direction get_stream_direction();
	virtual int push_message(IP2VeraMessage& p2m);
};

class P2VeraOutStreamQq : public P2VeraStreamQq {
public:
	P2VeraOutStreamQq(IP2VeraStreamHub *hub);
	virtual ~P2VeraOutStreamQq();
	virtual stream_direction get_stream_direction();

	virtual int pop_message(IP2VeraMessage& p2m);
	virtual void _insert_message(P2VeraBasicMessage& p2m);
};

#endif /* P2QUEUE_H_ */
