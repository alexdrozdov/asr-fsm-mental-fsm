/*
 * time_trigger.h
 *
 *  Created on: 11.06.2011
 *      Author: drozdov
 */

#ifndef TIME_TRIGGER_H_
#define TIME_TRIGGER_H_

#include "base_trigger.h"

enum ETimeTriggerMode {
	count_up = 0,     //Счет вверх (в виде пилы)
	count_down = 1,   //Счет вниз (в виде пилы)
	count_up_down = 2 //Счет вверх-вниз (в виде домиков)
};

enum ETimeTriggerCircle {
	loop_once = 0,     //Однократное выполнение цикла счета
	loop_infinite = 1, //Бесконечное выполнение цикла счета
	loop_count = 2     //Выполнение цикла счета в течение заданного времени
};

enum ETimeTriggerInState {
	timetrigger_in_unknown   = 0,
	timetrigger_in_underflow = 1,
	timetrigger_in_inrange   = 2,
	timetrigger_in_overflow  = 3
};

#define TIMETRIGGER_DIRECT     0
#define TIMETRIGGER_DIRECT_PR  1
#define TIMETRIGGER_REVERSE_PR 2
#define TIMETRIGGER_DIRECT_FL  3
#define TIMETRIGGER_REVERSE_FL 4

class CTimeTrigger : CBaseTrigger {
public:

	CTimeTrigger(std::string file_name);

	bool GetEnabled();
	void SetEnabled(bool b);

	double GetIntegralState();
	double GetExtendedState(int nExtender);
	int GetExtendedOutsCount();

	int BindTo(trig_inout_spec* self, trig_inout_spec* other);
	int BindFrom(trig_inout_spec* self, trig_inout_spec* other);

	void ProcessAnchestors();

private:

	//Функция оюнаруживает изменение выходных значений и вызывает соответсвующие обработчики
	void handle_values_differences();

	int input_state;

	long long time_start;
	long long time_uptrig;
	long long time_downtrig;
	int up_time;
	int down_time;
	int direction;

	int mode;
	int circle;
	int latch;
	int sense;

	bool trigger_stopped;

	double trig_up_threshold;
	double trig_down_threshold;

	std::vector<double> prev_values;

	bool was_trigged;

};

CTimeTrigger* load_time_trigger(std::string filename);


#endif /* TIME_TRIGGER_H_ */
