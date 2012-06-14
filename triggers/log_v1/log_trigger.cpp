/*
 * log_trigger.cpp
 *
 *  Created on: 19.06.2011
 *      Author: drozdov
 */

#include <iostream>
#include "log_trigger.h"
#include "mental_fsm.h"
#include "xml_support.h"

#include "mental_fsm_ext.h"
#include "trigger_tree.h"

using namespace std;

MentalFsm* fsm = NULL;
CTriggerTree* trigger_tree = NULL;

CLogTrigger::CLogTrigger(string file_name) {
	xmlConfigPtr xml = CreateXmlConfig((char*)file_name.c_str());
	if (NULL == xml) {
		cout << "CLogTrigger::CLogTrigger error: couldn`t load file <" << file_name << ">" << endl;
		return;
	}

	szTriggerName = xmlGetStringValue(xml,"/trigger/name");

	string latch_type = xmlGetStringValue(xml,"/trigger/latch");
	if ("combinational" == latch_type) {
		latch = combinational;
	} else if ("triggered" == latch_type) {
		latch = triggered;
	} else {
		cout << "CTimeTrigger::CTimeTrigger error: unknown latch type " << latch_type << endl;
		latch = combinational;
	}

	enabled = xmlGetBooleanValue(xml,"/trigger/enabled",false);

	//Загружаем пороги, по кторым должны срабатывать триггеры
	trig_up_threshold       = xmlGetDoubleValue(xml,"/trigger/messages/high_threshold/value",0);
	trig_down_threshold     = xmlGetDoubleValue(xml,"/trigger/messages/low_threshold/value",0);

	//Определяем, по каким порогам должны срабатывать триггеры
	max_threshold_overflow.enabled   = xmlGetBooleanValue(xml,"/trigger/messages/high_threshold/up/enabled",false);
	max_threshold_underflow.enabled  = xmlGetBooleanValue(xml,"/trigger/messages/high_threshold/down/enabled",false);
	min_threshold_overflow.enabled   = xmlGetBooleanValue(xml,"/trigger/messages/low_threshold/up/enabled",false);
	min_threshold_underflow.enabled  = xmlGetBooleanValue(xml,"/trigger/messages/low_threshold/down/enabled",false);

	max_threshold_overflow.log_enabled  = max_threshold_overflow.enabled && xmlGetBooleanValue(xml,"/trigger/messages/high_threshold/up/enable_log",false);
	max_threshold_underflow.log_enabled = max_threshold_underflow.enabled && xmlGetBooleanValue(xml,"/trigger/messages/high_threshold/down/enable_log",false);
	min_threshold_overflow.log_enabled  = min_threshold_overflow.enabled && xmlGetBooleanValue(xml,"/trigger/messages/low_threshold/up/enable_log",false);
	min_threshold_underflow.log_enabled = min_threshold_underflow.enabled && xmlGetBooleanValue(xml,"/trigger/messages/low_threshold/down/enable_log",false);

	max_threshold_overflow.feedback_enabled  = max_threshold_overflow.enabled && xmlGetBooleanValue(xml,"/trigger/messages/high_threshold/up/enable_feedback",false);
	max_threshold_underflow.feedback_enabled = max_threshold_underflow.enabled && xmlGetBooleanValue(xml,"/trigger/messages/high_threshold/down/enable_feedback",false);
	min_threshold_overflow.feedback_enabled  = min_threshold_overflow.enabled && xmlGetBooleanValue(xml,"/trigger/messages/low_threshold/up/enable_feedback",false);
	min_threshold_underflow.feedback_enabled = min_threshold_underflow.enabled && xmlGetBooleanValue(xml,"/trigger/messages/low_threshold/down/enable_feedback",false);

	//Загружаем сообщения, которые должны формироваться для каждого и порогов и направлений перехода
	max_threshold_overflow.log_text  = xmlGetStringValueSafe(xml,"/trigger/messages/high_threshold/up","");
	max_threshold_underflow.log_text = xmlGetStringValueSafe(xml,"/trigger/messages/high_threshold/down","");
	min_threshold_overflow.log_text  = xmlGetStringValueSafe(xml,"/trigger/messages/low_threshold/up","");
	min_threshold_underflow.log_text = xmlGetStringValueSafe(xml,"/trigger/messages/low_threshold/down","");

	max_threshold_overflow.feedback_text  = xmlGetStringValueSafe(xml,"/trigger/messages/high_threshold/up/feedback","");
	max_threshold_underflow.feedback_text = xmlGetStringValueSafe(xml,"/trigger/messages/high_threshold/down/feedback","");
	min_threshold_overflow.feedback_text  = xmlGetStringValueSafe(xml,"/trigger/messages/low_threshold/up/feedback","");
	min_threshold_underflow.feedback_text = xmlGetStringValueSafe(xml,"/trigger/messages/low_threshold/down/feedback","");

	//Задаем начальное состояние триггера. Чтобы избежать лишних сообщений
	//при загрузке помечаем его состояние как неопределенное.
	current_state = logtrigger_unknown;
	last_event = 0;

	anchs.resize(1);
	anchs[0] = NULL;

	//Инициализируем запись об этом триггере для дерева триггеров
	tree_leaf = new trigger_tree_leaf;
	tree_leaf->distance = 0;
	tree_leaf->trigger = this;

	trigger_tree->RegisterChildTrigger(tree_leaf);

	fsm->RegisterTrigger(this);

	touched = false;
	dump_enabled = false;
	dump_to_file = false;
}

int CLogTrigger::BindTo(trig_inout_spec* self, trig_inout_spec* other) {
	//Просто для совместимости. У этого триггера нет выходов
	return 0;
}

int CLogTrigger::BindFrom(trig_inout_spec* self, trig_inout_spec* other) {
	if (NULL==self || NULL==other) {
		cout << "CLogTrigger::BindFrom error - zero pointers received" << endl;
		return -1;
	}

	//Проверяем спецификацию входа этого триггера
	if (self->trigger != this) {
		cout << "CLogTrigger::BindFrom error - wrong pointer self. Differs from real pointer to this class" << endl;
		return 1;
	}
	if (inout_in != self->inout) {
		cout << "CLogTrigger::BindFrom error - wrong input specification" << endl;
		return 1;
	}
	if (self->offs > 0) {
		cout << "CLogTrigger::BindFrom error - input out of range" << endl;
		return 1;
	}

	//Проверяем спецификацию вязываемого триггера
	if (NULL == other->trigger) {
		cout << "CLogTrigger::BindFrom error - zero pointer other." << endl;
		return 1;
	}
	if (inout_out != other->inout) {
		cout << "CLogTrigger::BindFrom error - wrong output specification" << endl;
		return 1;
	}
	if (other->offs > other->trigger->GetExtendedOutsCount()) {
		cout << "CLogTrigger::BindFrom error - output out of range" << endl;
		return 1;
	}

	//Проверки на правильность данных прошли успешно.
	if (anchs[0] != NULL) {
		cout << "CLogTrigger::BindFrom warning - already was binded" << endl;
	}
	anchs[0] = other;
	return 0;
}

int CLogTrigger::GetExtendedOutsCount() {
	return 0;
}

bool CLogTrigger::GetEnabled() {
	return enabled;
}
void CLogTrigger::SetEnabled(bool b) {
	enabled = b;

	if (!b) {
		//Триггер отключен. Соответственно сбрасываем его защелку
		was_trigged = false;
	}
}

double CLogTrigger::GetIntegralState() {
	return 0;
}
double CLogTrigger::GetExtendedState(int nExtender) {
	return 1.0;
}

void CLogTrigger::ProcessAnchestors() {
	if (!enabled || anchs.size()<1) {
		//Работа триггера запрещена или нет ни одного предка
		return;
	}

	//Определяем входное значение триггера
	trig_inout_spec *anch = anchs[0];
	double input_value = anch->trigger->GetExtendedState(anch->offs);

	if (dump_enabled && dump_to_file) {
		dump_stream << "TIME " << fsm->GetCurrentTime() << " INPUTS " << input_value << endl;
	} else if (dump_enabled) {
		cout << szTriggerName << " TIME " << fsm->GetCurrentTime() << " INPUTS " << input_value << endl;
	}

	//cout << "CLogTrigger::ProcessAnchestors info - " << szTriggerName << " " << input_value << endl;

	//Определяем, в какое новое состояние перейдет триггер.
	int new_trigger_state = logtrigger_unknown;
	if (input_value < trig_down_threshold) {
		new_trigger_state = logtrigger_underflow;
	} else if (trig_down_threshold <= input_value && input_value < trig_up_threshold) {
		new_trigger_state = logtrigger_inrange;
	} else if (trig_up_threshold <= input_value) {
		new_trigger_state = logtrigger_overflow;
	} else {
		new_trigger_state = logtrigger_unknown;
	}

	if (logtrigger_unknown == current_state) {
		//Предыдущее состояние триггера было неопределено. Просто утсанвливаем
		//первое определенное состояние и выходим из обработчика
		current_state = new_trigger_state;
		return;
	}

	if (current_state==new_trigger_state) {
		//Состояние не изменилось. Обработка только в том случае, если триггер потроен
		//комбинационной схеме
		if (combinational != latch) {
			return;
		}
		switch (last_event) {
			case logtrigger_low_underflow:
				if (min_threshold_underflow.log_enabled)
					cout << min_threshold_underflow.log_text << endl;
				if (min_threshold_underflow.feedback_enabled)
					fsm->SendResponse(min_threshold_underflow.feedback_text);
				break;
			case logtrigger_low_overflow:
				if (min_threshold_overflow.log_enabled)
					cout << min_threshold_overflow.log_text << endl;
				if (min_threshold_overflow.feedback_enabled)
					fsm->SendResponse(min_threshold_overflow.feedback_text);
				break;
			case logtrigger_high_underflow:
				if (max_threshold_underflow.log_enabled)
					cout << max_threshold_underflow.log_text << endl;
				if (max_threshold_underflow.feedback_enabled)
					fsm->SendResponse(max_threshold_underflow.feedback_text);
				break;
			case logtrigger_high_overflow:
				if (max_threshold_overflow.log_enabled)
					cout << max_threshold_overflow.log_text << endl;
				if (max_threshold_overflow.feedback_enabled)
					fsm->SendResponse(max_threshold_overflow.feedback_text);
				break;
			default:
				break;
		}
		return;
	}

	//Состояние изменилось.
	switch (new_trigger_state) {
		case logtrigger_underflow:
			//Триггер перешел в низшее
			if (min_threshold_underflow.log_enabled)
				cout << min_threshold_underflow.log_text << endl;
			if (min_threshold_underflow.feedback_enabled)
				fsm->SendResponse(min_threshold_underflow.feedback_text);
			last_event = logtrigger_low_underflow;
			break;
		case logtrigger_inrange:
			if (current_state<new_trigger_state) {
				//Текущее состояние триггера было ниже нового. Значит, триггер перешел нижнюю границу
				if (min_threshold_overflow.log_enabled)
					cout << min_threshold_overflow.log_text << endl;
				if (min_threshold_overflow.feedback_enabled)
					fsm->SendResponse(min_threshold_overflow.feedback_text);
				last_event = logtrigger_low_overflow;
			} else {
				//Текущее состояние триггера было выше нового. Значит, триггер перешел верхнюю границу
				if (max_threshold_underflow.log_enabled)
					cout << max_threshold_underflow.log_text << endl;
				if (max_threshold_underflow.feedback_enabled)
					fsm->SendResponse(max_threshold_underflow.feedback_text);
				last_event = logtrigger_high_underflow;
			}
			break;
		case logtrigger_overflow:
			if (max_threshold_overflow.log_enabled)
				cout << max_threshold_overflow.log_text << endl;
			if (max_threshold_overflow.feedback_enabled)
				fsm->SendResponse(max_threshold_overflow.feedback_text);
			last_event = logtrigger_high_overflow;
			break;
		default:
			break;
	}
	current_state = new_trigger_state;
}

CLogTrigger* load_log_trigger(std::string filename) {
	return new CLogTrigger(filename);
}





