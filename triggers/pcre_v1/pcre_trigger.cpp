/*
 * neuro_trigger.cpp
 *
 *  Created on: 17.07.2011
 *      Author: drozdov
 */

#include <iostream>
#include <sstream>
#include "pcre_trigger.h"
#include "mental_fsm.h"
#include "xml_support.h"

#include "trigger_tree.h"
#include "mental_fsm_ext.h"

using namespace std;

MentalFsm* fsm = NULL;
CTriggerTree* trigger_tree = NULL;


CPcreTrigger::CPcreTrigger(string file_name) : CBaseTrigger()  {
	xmlConfigPtr xml = CreateXmlConfig((char*)file_name.c_str());
	pxml_tmp = (void*)xml;
	if (NULL == xml) {
		cout << "CPcreTrigger::CPcreTrigger error: couldn`t load file <" << file_name << ">" << endl;
		return;
	}

	szTriggerName = xmlGetStringValue(xml,"/trigger/name");
	enabled = xmlGetBooleanValue(xml,"/trigger/enabled",false);

	//Создаем выходы триггера
	out_values_count = xmlGetIntValue(xml,"/trigger/outputs/count",-1);
	if (out_values_count < 1) {
		//Ошибка! Недопустимое количество выходов у триггера
		cout << "CPcreTrigger::CPcreTrigger error: wrong output count" <<  endl;
		return;
	}
	values.resize(out_values_count);
	prev_values.resize(out_values_count,0);
	//Инициализируем предыдущие значения так, чтобы они вызвали срабатывание всех зависимых триггеров при первой же обработке
	for (int i=0;i<out_values_count;i++) {
		ostringstream ostr;
		ostr << "/trigger/outputs/i" << i << "/value";
		values[i] = xmlGetDoubleValue(xml,ostr.str().c_str(),0);
		prev_values[i] = -100;
	}

	//Инициализируем входы триггера
	in_values_count = xmlGetIntValue(xml,"/trigger/inputs/count",-1);
	if (in_values_count < 1) {
		//Ошибка! Недопустимое количество входов у триггера
		cout << "CPcreTrigger::CPcreTrigger error - wrong input count" <<  endl;
		return;
	}
	anchs.resize(in_values_count);
	for (int i=0;i<in_values_count;i++) {
		anchs[i] = NULL;
	}

	//Создаем очередь символов
	int max_q_size = xmlGetIntValue(xml,"/trigger/patterns/max_queue_length", 1);
	if (max_q_size < 1) {
		cout << "CPcreTrigger::CPcreTrigger error - не задан размер входной очереди" << endl;
		return;
	}
	pcreq = new CPcreStringQueue(max_q_size);

	//Загружаем кодирование входов
	input_coding.resize(in_values_count);
	for (int i=0;i<in_values_count;i++) {
		ostringstream ostr;
		ostr << "/trigger/inputs/i" << i;
		input_coding[i].name = xmlGetStringValue(xml, (ostr.str()+"/caption").c_str());
		input_coding[i].code = xmlGetStringValue(xml, (ostr.str()+"/code").c_str())[0];
	}

	//Загружаем шаблоны поиска
	int npatterns = xmlGetIntValue(xml, "/trigger/patterns/count",-1);
	if (npatterns < 1) {
		cout << "CPcreTrigger::CPcreTrigger error: wrong pattern count" <<  endl;
		return;
	}
	for (int i=0;i<npatterns;i++)
		patterns.push_back(new CPcrePattern(this, i));

	//Инициализируем запись об этом триггере для дерева триггеров
	tree_leaf = new trigger_tree_leaf;
	tree_leaf->distance = 0;
	tree_leaf->trigger = this;
	trigger_tree->RegisterChildTrigger(tree_leaf);

	fsm->RegisterTrigger(this);

	dump_enabled = false;
	dump_to_file = false;
	touched = false;
}

bool CPcreTrigger::GetEnabled() {
	return enabled;
}
void CPcreTrigger::SetEnabled(bool b) {
	enabled = b;
}

double CPcreTrigger::GetIntegralState() {
	return values[0];
}

double CPcreTrigger::GetExtendedState(int nExtender) {
	if (nExtender<0 || nExtender >= out_values_count) {
		cout << "CPcreTrigger::GetExtendedState error - index out of range in " << szTriggerName << endl;
		return -1;
	}

	return values[nExtender];
}

int CPcreTrigger::GetExtendedOutsCount() {
	return out_values_count;
}

int CPcreTrigger::BindTo(trig_inout_spec* self, trig_inout_spec* other) {
	if (NULL==self || NULL==other) {
		cout << "CPcreTrigger::BindTo error - zero pointers received" << endl;
		return -1;
	}

	//Проверяем спецификацию выхода этого триггера
	if (self->trigger != this) {
		cout << "CPcreTrigger::BindTo error - wrong pointer self. Differs from real pointer to this class" << endl;
		return 1;
	}
	if (inout_out != self->inout) {
		cout << "CPcreTrigger::BindTo error - wrong output specification" << endl;
		return 1;
	}
	if (self->offs >= (int)values.size()) {
		cout << "CPcreTrigger::BindTo error - output out of range" << endl;
		return 1;
	}

	//Проверяем спецификацию вязываемого триггера
	if (NULL == other->trigger) {
		cout << "CPcreTrigger::BindTo error - zero pointer other." << endl;
		return 1;
	}
	if (inout_out == other->inout) {
		cout << "CPcreTrigger::BindTo error - wrong input specification" << endl;
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
			cout << "CPcreTrigger::BindTo warning - " << self->trigger->szTriggerName << " was already binded to " << other->trigger->szTriggerName << endl;
			return 1;
		}
	}
	self->trigger->deps.push_back(other);
	return 0;
}

int CPcreTrigger::BindFrom(trig_inout_spec* self, trig_inout_spec* other) {
	if (NULL==self || NULL==other) {
		cout << "CNeuroTrigger::BindFrom error - zero pointers received" << endl;
		return -1;
	}

	//Проверяем спецификацию входа этого триггера
	if (self->trigger != this) {
		cout << "CPcreTrigger::BindFrom error - wrong pointer self. Differs from real pointer to this class" << endl;
		return 1;
	}
	if (inout_in != self->inout) {
		cout << "CPcreTrigger::BindFrom error - wrong input specification" << endl;
		return 1;
	}
	if (self->offs < 0 || self->offs >= in_values_count) {
		cout << "CPcreTrigger::BindFrom error - input out of range" << endl;
		return 1;
	}

	//Проверяем спецификацию вязываемого триггера
	if (NULL == other->trigger) {
		cout << "CPcreTrigger::BindFrom error - zero pointer other." << endl;
		return 1;
	}
	if (inout_out != other->inout) {
		cout << "CPcreTrigger::BindFrom error - wrong output specification" << endl;
		return 1;
	}
	if (other->offs > other->trigger->GetExtendedOutsCount()) {
		cout << "CPcreTrigger::BindFrom error - output out of range" << endl;
		return 1;
	}

	//Проверки на правильность данных прошли успешно.
	if (anchs[self->offs] != NULL) {
		cout << "CPcreTrigger::BindFrom warning - already was binded" << endl;
	}
	anchs[self->offs] = other;
	return 0;
}

void CPcreTrigger::push_input_to_q() {
	//Ищем вход с максимальным значением
	double max_val = anchs[0]->trigger->GetExtendedState(anchs[0]->offs);
	char c = input_coding[0].code;
	for (int i=1;i<(int)anchs.size();i++) {
		double val = anchs[i]->trigger->GetExtendedState(anchs[i]->offs);
		if (val > max_val) {
			max_val = val;
			c = input_coding[i].code;
		}
	}

	pcreq->push_back(c);
}

void CPcreTrigger::ProcessAnchestors() {
	push_input_to_q(); //Записываем входы в очередь обработки
	//cout << "queue " << pcreq->str() << endl;

	vector<CPcrePattern*>::iterator it; //Просматриваем входную последовательность на соответствие шаблонам
	int max_trim = 0;
	for (it=patterns.begin();it!=patterns.end();it++) {
		int cur_trim = (*it)->ProcessString(pcreq);
		max_trim = (cur_trim>max_trim)?cur_trim:max_trim;
	}
	pcreq->pop_front(max_trim); //Удаляем из входного буфера все найденные последовательности

	handle_values_differences(); //Обрабатываем изменившиеся значения
}

void CPcreTrigger::handle_values_differences() {
	if (dump_enabled && dump_to_file) {
		dump_stream << " OUTS ";
		for (int i=0;i<out_values_count;i++) {
			dump_stream << values[i] << " ";
		}
		//dump_stream << "STATE " << state->szCaption;
		dump_stream << endl;
	} else if (dump_enabled) {
		cout << " OUTS ";
		for (int i=0;i<out_values_count;i++) {
			cout << values[i] << " ";
		}
		//cout << "STATE " << state->szCaption;
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

CPcreTrigger* load_pcre_trigger(std::string filename) {
	return new CPcreTrigger(filename);
}



