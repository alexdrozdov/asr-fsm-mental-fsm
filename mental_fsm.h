/*
 * mental_fsm.h
 *
 *  Created on: 11.06.2011
 *      Author: drozdov
 */

#ifndef MENTAL_FSM_H_
#define MENTAL_FSM_H_

#include <map>
#include "base_trigger.h"

class MentalFsm;

class MentalFsm {
public:
	MentalFsm();

	virtual int RegisterTrigger(CBaseTrigger* trigger);
	virtual int RegisterTimeTrigger(CBaseTrigger* trigger);
	virtual bool UnregisterTrigger(CBaseTrigger* trigger);
	virtual bool UnregisterTrigger(std::string name);

	virtual CBaseTrigger* FindTrigger(std::string name);

	virtual long long GetCurrentTime();
	virtual int RunToTime(long long new_time);

	virtual void SetLocalSamplerate(unsigned int samplerate);
	virtual void SetRemoteSamplerate(unsigned int samplerate);
	virtual void SetMinSamplerate(unsigned int samplerate);

	virtual unsigned int GetLocalSamplerate();
	virtual unsigned int GetRemoteSamplerate();
	virtual unsigned int GetMinSamplerate();

	virtual long long ScaleRemoteTime(long long remote_time);

	virtual void SendResponse(std::string response_text);
private:

	virtual int StepToTime(long long new_time);
	virtual void TouchTimeTriggers();

	long long current_time;
	unsigned int local_samplerate;
	unsigned int remote_samplerate;
	unsigned int min_samplerate;
	long long max_time_step;

	std::vector<CBaseTrigger*> triggers;    //Все триггеры, зарегистрированые в этом обработчике
	std::vector<CBaseTrigger*> unprocessed; //Триггеры, которые требуется обработать
	std::vector<CBaseTrigger*> time_triggers; //Триггеры времени

	std::map<std::string ,CBaseTrigger*> trig_dict; //Триггеры по их именам
};



#endif /* MENTAL_FSM_H_ */
