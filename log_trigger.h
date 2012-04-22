/*
 * log_trigger.h
 *
 *  Created on: 11.06.2011
 *      Author: drozdov
 */

#ifndef LOG_TRIGGER_H_
#define LOG_TRIGGER_H_


#include "base_trigger.h"

enum ELogTriggerState {
	logtrigger_unknown   = 0,
	logtrigger_underflow = 1,
	logtrigger_inrange   = 2,
	logtrigger_overflow  = 3
};

enum ELogTriggerLastEvent {
	logtrigger_low_underflow  = 0,
	logtrigger_low_overflow   = 1,
	logtrigger_high_underflow = 2,
	logtrigger_high_overflow  = 3
};


typedef struct _log_trig_event_response {
	bool enabled;
	bool log_enabled;
	bool feedback_enabled;
	std::string log_text;
	std::string feedback_text;
} log_trig_event_response;

class CLogTrigger : CBaseTrigger {
public:

	CLogTrigger(std::string file_name);

	bool GetEnabled();
	void SetEnabled(bool b);

	double GetIntegralState();
	double GetExtendedState(int nExtender);
	int GetExtendedOutsCount();

	int BindTo(trig_inout_spec* self, trig_inout_spec* other);
	int BindFrom(trig_inout_spec* self, trig_inout_spec* other);

	void ProcessAnchestors();

private:

	int latch;
	int sense;

	double trig_up_threshold;
	double trig_down_threshold;

	log_trig_event_response min_threshold_overflow;
	log_trig_event_response min_threshold_underflow;
	log_trig_event_response max_threshold_overflow;
	log_trig_event_response max_threshold_underflow;

	int current_state;
	int last_event;

	bool was_trigged;

};

CLogTrigger* load_log_trigger(std::string filename);


#endif /* LOG_TRIGGER_H_ */
