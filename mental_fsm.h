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

	int RegisterTrigger(CBaseTrigger* trigger);
	int RegisterTimeTrigger(CBaseTrigger* trigger);
	CBaseTrigger* FindTrigger(std::string name);

	long long GetCurrentTime();
	int RunToTime(long long new_time);
private:

	long long current_time;

	std::vector<CBaseTrigger*> triggers;    //Все триггеры, зарегистрированые в этом обработчике
	std::vector<CBaseTrigger*> unprocessed; //Триггеры, которые требуется обработать
	std::vector<CBaseTrigger*> time_triggers; //Триггеры времени

	std::map<std::string ,CBaseTrigger*> trig_dict; //Триггеры по их именам
};

extern MentalFsm* fsm;

extern std::string executable_path;
extern std::string project_path;


#endif /* MENTAL_FSM_H_ */
