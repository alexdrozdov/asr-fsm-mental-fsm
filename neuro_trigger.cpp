/*
 * neuro_trigger.cpp
 *
 *  Created on: 17.07.2011
 *      Author: drozdov
 */

#include <iostream>
#include "base_trigger.h"
#include "neuro_trigger.h"
#include "mental_fsm.h"
#include "xml_config.h"

#include "trigger_tree.h"

using namespace std;


CNeuroTrigger::CNeuroTrigger(string file_name) {
	xmlConfigPtr xml = CreateXmlConfig((char*)file_name.c_str());
	if (NULL == xml) {
		cout << "CNeuroTrigger::CNeuroTrigger error: couldn`t load file <" << file_name << ">" << endl;
		return;
	}

	//Создаем выходы триггера
	out_values_count = xmlGetIntValue(xml,"/trigger/outputs/count",-1);
	if (out_values_count < 1) {
		//Ошибка! Недопустимое количество выходов у триггера
		cout << "CNeuroTrigger::CNeuroTrigger error: wrong output count" <<  endl;
		return;
	}
	values.resize(out_values_count);
	prev_values.resize(out_values_count);
	//Инициализируем предыдущие значения так, чтобы они вызвали срабатывание всех зависимых триггеров при первой же обработке
	for (int i=0;i<out_values_count;i++) {
		prev_values[i] = -10;
	}

	//Инициализируем входы триггера
	in_values_count = xmlGetIntValue(xml,"/trigger/inputs/count",-1);
	if (in_values_count < 1) {
		//Ошибка! Недопустимое количество входов у триггера
		cout << "CNeuroTrigger::CNeuroTrigger error: wrong input count" <<  endl;
		return;
	}
	anchs.resize(in_values_count);
	for (int i=0;i<in_values_count;i++) {
		anchs[i] = NULL;
	}

	szTriggerName = xmlGetStringValue(xml,"/trigger/name");
	enabled = xmlGetBooleanValue(xml,"/trigger/enabled",false);

	//Инициализируем группы состояний этого триггера
	union_count = xmlGetIntValue(xml,"/trigger/unions/count",-1);
	pxml_tmp = (void*)xml;

	if (union_count > 0) {
		unions.resize(union_count);
		for (int i=0;i<union_count;i++) {
			unions[i] = new CNeuroUnion(this,i);
		}
	}

	//Инициализируем внутренние состояния этого триггера
	states_count= xmlGetIntValue(xml,"/trigger/states/count",-1);
	if (in_values_count < 1) {
		//Ошибка! Недопустимое количество состояний у триггера
		cout << "CNeuroTrigger::CNeuroTrigger error: wrong states count" <<  endl;
		return;
	}

	states.resize(states_count);
	for (int i=0;i<states_count;i++) {
		states[i] = new CNeuroState(this,i);
	}

	//Переводим триггер в его начальное состояние
	state = NULL;
	int def_state = xmlGetIntValue(xml,"/trigger/states/default",0);
	SetState(def_state);

	//Инициализируем запись об этом триггере для дерева триггеров
	tree_leaf = new trigger_tree_leaf;
	tree_leaf->distance = 0;
	tree_leaf->trigger = this;
	trigger_tree->RegisterChildTrigger(tree_leaf);

	fsm->RegisterTrigger(this);
	fsm->RegisterTimeTrigger(this);

	dump_enabled = false;
	dump_to_file = false;
	touched = false;
}

bool CNeuroTrigger::GetEnabled() {
	return enabled;
}
void CNeuroTrigger::SetEnabled(bool b) {
	enabled = b;
}

double CNeuroTrigger::GetIntegralState() {
	return values[0];
}

double CNeuroTrigger::GetExtendedState(int nExtender) {
	if (nExtender<0 || nExtender >= out_values_count) {
		cout << "CNeuroTrigger::GetExtendedState error - index out of range in " << szTriggerName << endl;
		return -1;
	}

	return values[nExtender];
}

int CNeuroTrigger::GetExtendedOutsCount() {
	return out_values_count;
}

int CNeuroTrigger::BindTo(trig_inout_spec* self, trig_inout_spec* other) {
	if (NULL==self || NULL==other) {
		cout << "CNeuroTrigger::BindTo error - zero pointers received" << endl;
		return -1;
	}

	//Проверяем спецификацию выхода этого триггера
	if (self->trigger != this) {
		cout << "CNeuroTrigger::BindTo error - wrong pointer self. Differs from real pointer to this class" << endl;
		return 1;
	}
	if (inout_out != self->inout) {
		cout << "CNeuroTrigger::BindTo error - wrong output specification" << endl;
		return 1;
	}
	if (self->offs >= (int)values.size()) {
		cout << "CNeuroTrigger::BindTo error - output out of range" << endl;
		return 1;
	}

	//Проверяем спецификацию вязываемого триггера
	if (NULL == other->trigger) {
		cout << "CNeuroTrigger::BindTo error - zero pointer other." << endl;
		return 1;
	}
	if (inout_out == other->inout) {
		cout << "CNeuroTrigger::BindTo error - wrong input specification" << endl;
		return 1;
	}
	//if (other->offs > other->trigger->GetExtendedOutsCount()) {
	//	cout << "CTimeTrigger::BindFrom error - output out of range" << endl;
	//	return 1;
	//}

	//Проверяем, что этого триггера нет в списке зависимых  триггеров
	vector<trig_inout_spec*>::iterator it;
	for ( it=self->trigger->deps.begin() ; it < self->trigger->deps.end(); it++ ) {
		trig_inout_spec* tis = *it;
		if (tis->trigger == other->trigger) {
			cout << "CNeuroTrigger::BindTo warning - " << self->trigger->szTriggerName << " was already binded to " << other->trigger->szTriggerName << endl;
			return 1;
		}
	}
	self->trigger->deps.push_back(other);
	return 0;
}

int CNeuroTrigger::BindFrom(trig_inout_spec* self, trig_inout_spec* other) {
	if (NULL==self || NULL==other) {
		cout << "CNeuroTrigger::BindFrom error - zero pointers received" << endl;
		return -1;
	}

	//Проверяем спецификацию входа этого триггера
	if (self->trigger != this) {
		cout << "CNeuroTrigger::BindFrom error - wrong pointer self. Differs from real pointer to this class" << endl;
		return 1;
	}
	if (inout_in != self->inout) {
		cout << "CNeuroTrigger::BindFrom error - wrong input specification" << endl;
		return 1;
	}
	if (self->offs < 0 || self->offs >= in_values_count) {
		cout << "CNeuroTrigger::BindFrom error - input out of range" << endl;
		return 1;
	}

	//Проверяем спецификацию вязываемого триггера
	if (NULL == other->trigger) {
		cout << "CNeuroTrigger::BindFrom error - zero pointer other." << endl;
		return 1;
	}
	if (inout_out != other->inout) {
		cout << "CNeuroTrigger::BindFrom error - wrong output specification" << endl;
		return 1;
	}
	if (other->offs > other->trigger->GetExtendedOutsCount()) {
		cout << "CNeuroTrigger::BindFrom error - output out of range" << endl;
		return 1;
	}

	//Проверки на правильность данных прошли успешно.
	if (anchs[self->offs] != NULL) {
		cout << "CNeuroTrigger::BindFrom warning - already was binded" << endl;
	}
	anchs[self->offs] = other;
	return 0;
}

void CNeuroTrigger::ProcessAnchestors() {
	state->ProcessInputs();
	handle_values_differences();
}

void CNeuroTrigger::handle_values_differences() {
	if (dump_enabled && dump_to_file) {
		dump_stream << " OUTS ";
		for (int i=0;i<out_values_count;i++) {
			dump_stream << values[i] << " ";
		}
		dump_stream << "STATE " << state->szCaption;
		dump_stream << endl;
	} else if (dump_enabled) {
		cout << " OUTS ";
		for (int i=0;i<out_values_count;i++) {
			cout << values[i] << " ";
		}
		cout << "STATE " << state->szCaption;
		cout << endl;
	}
	for (int i=0;i<out_values_count;i++) {
		if (values[i] != prev_values[i]) {
			prev_values[i] = values[i];
			vector<trig_inout_spec*>::iterator it;

			for (it=deps.begin();it<deps.end();it++) {
				trig_inout_spec* tis = *it;
				if (tis->offs != 0) {
					continue;
				}
				//Помечаем триггер, зависящий от этого входа, как требующий обработки
				tis->trigger->AddToDynamicTree();
			}
		}
	}
}

CNeuroTrigger* load_neuro_trigger(std::string filename) {
	return new CNeuroTrigger(filename);
}

int CNeuroTrigger::SetState(int state_num) {
	CNeuroState* next_state = states[state_num];

	if (state != next_state) {
		// Состояние меняется. Проверяем изменение кластера сотсояний
		if (next_state->unions.size() > 0) {
			neuro_union = next_state->unions[0];
		} else {
			neuro_union = NULL;
		}
	}

	state = next_state;
	return state_num;
}


CNeuroUnion* CNeuroTrigger::get_union_by_name(std::string union_name) {
	std::vector<CNeuroUnion*>::iterator it;
	for (it=unions.begin();it!=unions.end();it++) {
		CNeuroUnion* cur = *it;
		if (union_name == cur->szCaption) {
			return cur;
		}
	}

	return NULL;
}

CNeuroUnion* CNeuroTrigger::get_union_by_id(int nunion) {
	return unions[nunion];
}

CNeuroState* CNeuroTrigger::get_state_by_name(std::string state_name) {
	std::vector<CNeuroState*>::iterator it;
	for (it=states.begin();it!=states.end();it++) {
		CNeuroState* cur = *it;
		if (state_name == cur->szCaption) {
			return cur;
		}
	}

	return NULL;
}

CNeuroState* CNeuroTrigger::get_state_by_id(int nstate) {
	return states[nstate];
}


