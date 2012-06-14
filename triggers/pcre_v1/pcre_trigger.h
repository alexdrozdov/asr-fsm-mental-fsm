/*
 * pcre_trigger.h
 *
 *  Created on: 16.07.2011
 *      Author: drozdov
 */

#ifndef PCRE_TRIGGER_H_
#define PCRE_TRIGGER_H_

#include "mental_fsm_ext.h"

typedef struct _pcre_input_coding {
	std::string name;
	char code;
} pcre_input_coding;

class CPcreTrigger;

typedef struct _out_change {
	unsigned int out;
	double new_value;
} out_change;

class CPcreStringQueue {
public:
	CPcreStringQueue(int max_size);
	void push_back(char c);
	void pop_front(int count);
	std::string str();
private:
	int max_size;
	int cur_size;
	char *q;
};

class CPcrePattern {
public:
	CPcrePattern(CPcreTrigger *trigger, int ntemplate);
	int ProcessString(CPcreStringQueue* strq);
private:
	std::string pattern;
	CPcreTrigger *trigger;
	std::vector<out_change> output_changes;

	void *ppcre;
};

class CPcreTrigger : public CBaseTrigger {
public:
	CPcreTrigger(std::string file_name);

	bool GetEnabled();
	void SetEnabled(bool b);

	double GetIntegralState();
	double GetExtendedState(int nExtender);
	int GetExtendedOutsCount();

	int BindTo(trig_inout_spec* self, trig_inout_spec* other);
	int BindFrom(trig_inout_spec* self, trig_inout_spec* other);

	void ProcessAnchestors();

	friend class CPcrePattern;
private:

	CPcreStringQueue* pcreq;

	void push_input_to_q();
	void handle_values_differences();

	std::vector<double> prev_values;
	int out_values_count;
	int in_values_count;

	std::vector<pcre_input_coding> input_coding;
	std::vector<CPcrePattern*> patterns;
	//Временный указатель на xml. Используется только при загрузке класса
	void* pxml_tmp;
};


extern MentalFsm* fsm;
extern CTriggerTree* trigger_tree;

CPcreTrigger* load_pcre_trigger(std::string filename);

#ifdef __cplusplus
extern "C" {
#endif

extern int fsm_trigger_init(dynamictrig_init* dt_init);

extern int   pcre_v1_init_tcl(Tcl_Interp* interp);
extern int   pcre_v1_init_core(trig_init_environ* env);
extern void* pcre_v1_create(std::string filename);

#ifdef __cplusplus
}
#endif

#endif /* NEURO_TRIGGER_H_ */
