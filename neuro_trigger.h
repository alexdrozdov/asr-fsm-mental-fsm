/*
 * neuro_trigger.h
 *
 *  Created on: 16.07.2011
 *      Author: drozdov
 */

#ifndef NEURO_TRIGGER_H_
#define NEURO_TRIGGER_H_

#include "base_trigger.h"
#include "floatfann.h"
#include "fann_cpp.h"

class CNeuroTrigger; // Триггер с нейронной сетью
class CNeuroState;   // Состояние триггера (конечного автомата)
class CNeuroUnion;   // Объединение идентичных состояний

struct from_to_links {
	int from;
	int to;
	bool is_const;
	double value;
};

class CNeuroUnion {
public:
	CNeuroUnion(CNeuroTrigger *trigger, int nunion);

	bool IsMember(int nstate); //Проверяет принадлежность состояния к этому объединению
	void AddMemeber(int nstate); //Добавить состояние в эту группу

	void Enter();

	int TryState(int nstate);  //Выполняет попытку перейти в указанное состояние. Возвращает новое состояние.
	//Объединение может разрещить переход или отправить автомат в другое состояние в связи с невыполнением условия

	void Print();

	friend class CNeuroTrigger;
private:
	std::vector<bool> members; //Признаки членства в этой группе

	int min_time;    // Минимальное время, в течение которого триггер должен держаться в этом состоянии (в группе этого состояния)
	int time_udf_ns; // Состояние, в которое попадет триггер, если не продержится заданное время
	bool min_time_en;

	int max_time;    // Максимальное время, в течение которого триггер может держаться в этом состоянии (в группе состояний)
	int time_ovf_ns; // Состояние, в которое попадет триггер, если продержится дольше
	bool max_time_en;

	long long time_enter; //Время, когда триггер попал в эту группу состояний
	CNeuroTrigger* trigger;

	int id;

	std::string szCaption;
};

class CNeuroState {
public:
	CNeuroState(CNeuroTrigger *trigger, int nstate);
	void ProcessInputs();

	void Enter();

	friend class CNeuroTrigger;
private:

	void evalute_fann_output();
	void copy_outputs();

	FANN::neural_net *fann;
	std::vector<from_to_links> net_inputs;
	std::vector<from_to_links> net_outputs;
	std::vector<int> next_states;
	std::string fann_file_name;
	std::string tcl_file_name;
	int id;

	float *fann_input;
	float *fann_output;

	bool stable;

	int input_count;
	int output_count;
	int state_count;

	int min_time;    // Минимальное время, в течение которого триггер должен держаться в этом состоянии (в группе этого состояния)
	int time_udf_ns; // Состояние, в которое попадет триггер, если не продержится заданное время
	bool min_time_en;

	int max_time;    // Максимальное время, в течение которого триггер может держаться в этом состоянии (в группе состояний)
	int time_ovf_ns; // Состояние, в которое попадет триггер, если продержится дольше
	bool max_time_en;

	long long time_enter; //Время, когда триггер попал в это состояние
	CNeuroTrigger* trigger;

	std::vector<CNeuroUnion*> unions;

	std::string szCaption;
};


class CNeuroTrigger : CBaseTrigger {
public:
	CNeuroTrigger(std::string file_name);

	bool GetEnabled();
	void SetEnabled(bool b);

	double GetIntegralState();
	double GetExtendedState(int nExtender);
	int GetExtendedOutsCount();

	int BindTo(trig_inout_spec* self, trig_inout_spec* other);
	int BindFrom(trig_inout_spec* self, trig_inout_spec* other);

	void ProcessAnchestors();

	int GetState();
	//Установка состояния по его номеру
	int SetState(int state_num);
	//Установка состояния по его указателю
	int SetState(CNeuroState* st);

	friend class CNeuroState;
	friend class CNeuroUnion;
private:

	void handle_values_differences();

	CNeuroUnion* get_union_by_name(std::string union_name);
	CNeuroUnion* get_union_by_id(int nunion);

	CNeuroState* get_state_by_name(std::string state_name);
	CNeuroState* get_state_by_id(int nstate);


	std::vector<double> prev_values;
	int out_values_count;
	int in_values_count;

	int states_count;
	int state_num;
	std::vector<CNeuroState*> states;
	CNeuroState* state;

	int union_count;
	int union_num;
	std::vector<CNeuroUnion*> unions;
	CNeuroUnion* neuro_union;

	//Временный указатель на xml. Используется только при загрузке класса
	void* pxml_tmp;
};

CNeuroTrigger* load_neuro_trigger(std::string filename);


#endif /* NEURO_TRIGGER_H_ */
