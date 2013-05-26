/*
 * netlink_pack.h
 *
 *  Created on: 04.07.2011
 *      Author: drozdov
 */

#ifndef P2VERA_DSP_H_
#define P2VERA_DSP_H_

#include <pthread.h>

#include <string>
#include <vector>
#include <queue>
#include <fstream>
#include <stdint.h>

#include <p2vera.h>
#include <p2message.h>
#include <p2stream.h>

#include "dsp_stream.pb.h"

typedef struct _trig_msg_item {
	int32_t out_id;
	double value;
} trig_msg_item, *ptrig_msg_item;

class DspMessage : public IP2VeraMessage {
public:
	virtual ~DspMessage();
	virtual void Clear() = 0;

	virtual bool get_data(std::string& str) const;
	virtual int get_data(void* data, int max_data_size) const;
	virtual int get_data_size() const;
	virtual bool set_data(void* data, int data_size);
	virtual bool set_data(std::string &str);
protected:
	dsp::dsp_package pkg;
};

class DspMessageCmn;

class DspMessageTrig : public DspMessage {
public:
	DspMessageTrig();
	virtual ~DspMessageTrig();
	void Add(int triger_id, int out_id, double value);
	virtual void Clear();

	friend class DspMessageCmn;
};

class DspMessageTime : public DspMessage {
public:
	DspMessageTime(long long time);
	virtual ~DspMessageTime();

	void SetTime(long long time);
	virtual void Clear();

	friend class DspMessageCmn;
};

class DspMessageSamplerate : public DspMessage {
public:
	DspMessageSamplerate(unsigned int samplerate);
	virtual ~DspMessageSamplerate();

	void SetSamplerate(unsigned int samplerate);
	virtual void Clear();

	friend class DspMessageCmn;
};

class DspMessageCmn : public DspMessage {
public:
	DspMessageCmn();
	virtual ~DspMessageCmn();

	virtual void Clear();

	virtual bool has_trig();
	virtual bool has_time();
	virtual bool has_samplerate();

	virtual DspMessageCmn& operator>>(DspMessageTrig&);
	virtual DspMessageCmn& operator>>(DspMessageTime&);
	virtual DspMessageCmn& operator>>(DspMessageSamplerate&);

	const dsp::dsp_package& dsp() const;
};

extern P2Vera *p2v;

#endif /* NETLINK_PACK_H_ */
