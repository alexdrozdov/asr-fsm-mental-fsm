/*
 * time_trigger.cpp
 *
 *  Created on: 11.06.2011
 *      Author: drozdov
 */

#include <stdlib.h>

#include <iostream>
#include "base_trigger.h"
#include "time_trigger.h"
#include "mental_fsm.h"
#include "xml_support.h"

#include "trigger_tree.h"
#include "mental_fsm_ext.h"

using namespace std;



CTimeTrigger::CTimeTrigger(string file_name) {
	xmlConfigPtr xml = CreateXmlConfig((char*)file_name.c_str());
	if (NULL == xml) {
		cout << "CTimeTrigger::CTimeTrigger error: couldn`t load file <" << file_name << ">" << endl;
		return;
	}

	values.resize(5);
	prev_values.resize(5);
	//Инициализируем предыдущие значения так, чтобы они вызвали срабатывание всех зависимых триггеров при первой же обработке
	for (int i=0;i<5;i++) {
		prev_values[i] = -10;
	}

	string latch_type = xmlGetStringValue(xml,"/trigger/latch");
	string direction  = xmlGetStringValue(xml,"/trigger/count/direction");
	string stop_type  = xmlGetStringValue(xml,"/trigger/stop/type");
	string sense      = xmlGetStringValue(xml,"/trigger/sense/event");

	szTriggerName = xmlGetStringValue(xml,"/trigger/name");

	trig_up_threshold   = xmlGetDoubleValue(xml,"/trigger/sense/max",0);
	trig_down_threshold = xmlGetDoubleValue(xml,"/trigger/sense/min",0);

	up_time   = (int)((long long)xmlGetDelayUsValue(xml,"/trigger/count/up", -1) * (long long)fsm->GetLocalSamplerate() / 1000000LL);
	down_time = (int)((long long)xmlGetDelayUsValue(xml,"/trigger/count/down", -1) * (long long)fsm->GetLocalSamplerate() / 1000000LL);


	if ("combinational" == latch_type) {
		latch = combinational;
	} else if ("triggered" == latch_type) {
		latch = triggered;
	} else {
		cout << "CTimeTrigger::CTimeTrigger error: unknown latch type " << latch_type << endl;
		latch = combinational;
	}

	if ("up" == direction) {
		mode = count_up;
	} else if ("down" == direction) {
		mode = count_down;
	} else if ("updown" == direction) {
		mode = count_up_down;
	} else {
		cout << "CTimeTrigger::CTimeTrigger error: unknown count direction " << direction << endl;
		mode = count_up;
	}

	if ("once" == stop_type) {
		circle = loop_once;
	} else if ("infinite" == stop_type) {
		circle = loop_infinite;
	} else if ("count" == stop_type) {
		circle = loop_count;
	} else {
		cout << "CTimeTrigger::CTimeTrigger error: unknown count loop " << stop_type << endl;
		circle = loop_once;
	}

	if ("trigup" == sense) {
		sense = sense_trigup;
	} else if ("trigdown" == sense) {
		sense = sense_trigdown;
	} else if ("trigrange" == sense) {
		sense = sense_trigrange;
	} else {
		cout << "CTimeTrigger::CTimeTrigger error: unknown trigger sense " << sense << endl;
		sense = sense_trigup;
	}

	anchs.resize(1);
	anchs[0] = NULL;

	enabled = xmlGetBooleanValue(xml,"/trigger/enabled",false);

	input_state = timetrigger_in_unknown;
	trigger_stopped = false;
	was_trigged     = false;

	//Инициализируем запись об этом триггере для дерева триггеров
	tree_leaf = new trigger_tree_leaf;
	tree_leaf->distance = 0;
	tree_leaf->trigger = this;
	trigger_tree->RegisterChildTrigger(tree_leaf);


	fsm->RegisterTrigger(this);
	fsm->RegisterTimeTrigger(this);

	touched = false;
	dump_enabled = false;
	dump_to_file = false;
}

int CTimeTrigger::BindTo(trig_inout_spec* self, trig_inout_spec* other) {
	if (NULL==self || NULL==other) {
		cout << "CTimeTrigger::BindTo error - zero pointers received" << endl;
		return -1;
	}

	//Проверяем спецификацию выхода этого триггера
	if (self->trigger != this) {
		cout << "CTimeTrigger::BindTo error - wrong pointer self. Differs from real pointer to this class" << endl;
		return 1;
	}
	if (inout_out != self->inout) {
		cout << "CTimeTrigger::BindTo error - wrong output specification" << endl;
		return 1;
	}
	if (self->offs >= (int)values.size()) {
		cout << "CTimeTrigger::BindTo error - output out of range" << endl;
		return 1;
	}

	//Проверяем спецификацию вязываемого триггера
	if (NULL == other->trigger) {
		cout << "CTimeTrigger::BindTo error - zero pointer other." << endl;
		return 1;
	}
	if (inout_out == other->inout) {
		cout << "CTimeTrigger::BindTo error - wrong input specification" << endl;
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
			cout << "CTimeTrigger::BindTo warning - " << self->trigger->szTriggerName << " was already binded to " << other->trigger->szTriggerName << endl;
			return 1;
		}
	}
	self->trigger->deps.push_back(other);
	return 0;
}

int CTimeTrigger::BindFrom(trig_inout_spec* self, trig_inout_spec* other) {
	if (NULL==self || NULL==other) {
		cout << "CTimeTrigger::BindFrom error - zero pointers received" << endl;
		return -1;
	}

	//Проверяем спецификацию входа этого триггера
	if (self->trigger != this) {
		cout << "CTimeTrigger::BindFrom error - wrong pointer self. Differs from real pointer to this class" << endl;
		return 1;
	}
	if (inout_in != self->inout) {
		cout << "CTimeTrigger::BindFrom error - wrong input specification" << endl;
		return 1;
	}
	if (self->offs > 0) {
		cout << "CTimeTrigger::BindFrom error - input out of range" << endl;
		return 1;
	}

	//Проверяем спецификацию вязываемого триггера
	if (NULL == other->trigger) {
		cout << "CTimeTrigger::BindFrom error - zero pointer other." << endl;
		return 1;
	}
	if (inout_out != other->inout) {
		cout << "CTimeTrigger::BindFrom error - wrong output specification" << endl;
		return 1;
	}
	if (other->offs > other->trigger->GetExtendedOutsCount()) {
		cout << "CTimeTrigger::BindFrom error - output out of range" << endl;
		return 1;
	}

	//Проверки на правильность данных прошли успешно.
	if (anchs[0] != NULL) {
		cout << "CTimeTrigger::BindFrom warning - already was binded" << endl;
	}
	anchs[0] = other;
	return 0;
}

int CTimeTrigger::GetExtendedOutsCount() {
	return (int)values.size();
}

bool CTimeTrigger::GetEnabled() {
	return enabled;
}
void CTimeTrigger::SetEnabled(bool b) {
	enabled = b;

	if (!b) {
		//Триггер отключен. Соответственно сбрасываем его защелку
		was_trigged = false;
		trigger_stopped = false;
	}
}

double CTimeTrigger::GetIntegralState() {
	return value;
}

double CTimeTrigger::GetExtendedState(int nExtender) {
	if (nExtender<0 || nExtender >= (int)values.size()) {
		cout << "CTimeTrigger::GetExtendedState error - index out of range in " << szTriggerName << endl;
		return -1;
	}

	return values[nExtender];
}

void CTimeTrigger::ProcessAnchestors() {
	if (!enabled || anchs.size()<1) {
		//Работа триггера запрещена или нет ни одного предка
		return;
	}

	if (trigger_stopped) {
		return;
	}

	//Определяем входное значение триггера
	trig_inout_spec *anch = anchs[0];
	double input_value = anch->trigger->GetExtendedState(anch->offs);

	//Определяем, в какое новое состояние перейдет триггер.
	int new_input_state = timetrigger_in_unknown;
	if (input_value < trig_down_threshold) {
		new_input_state = timetrigger_in_underflow;
	} else if (trig_down_threshold <= input_value && input_value < trig_up_threshold) {
		new_input_state = timetrigger_in_inrange;
	} else if (trig_up_threshold <= input_value) {
		new_input_state = timetrigger_in_overflow;
	} else {
		new_input_state = timetrigger_in_unknown;
	}

	if (timetrigger_in_unknown == input_state) {
		//Предыдущее состояние триггера было неопределено. Просто устанавливаем
		//первое определенное состояние и выходим из обработчика
		input_state = new_input_state;
		return;
	}

	//Решаем, что делать
	bool new_was_trigged = false;
	if (sense_trigup==sense && timetrigger_in_overflow==new_input_state) {
		//Триггер сработал
		new_was_trigged = true;
	} else if (sense_trigdown==sense && timetrigger_in_underflow==new_input_state) {
		//Триггер сработал
		new_was_trigged = true;
	} else if (sense_trigrange==sense && timetrigger_in_inrange==new_input_state) {
		//Триггер сработал
		new_was_trigged = true;
	}

	input_state = new_input_state;

	if (dump_enabled && dump_to_file) {
		dump_stream << "TIME " << fsm->GetCurrentTime() << " INPUTS " << input_value << " STATE ";
		switch (input_state) {
			case timetrigger_in_unknown:
				dump_stream << "timetrigger_in_unknown ";
				break;
			case timetrigger_in_underflow:
				dump_stream << "timetrigger_in_underflow ";
				break;
			case timetrigger_in_inrange:
				dump_stream << "timetrigger_in_inrange ";
				break;
			case timetrigger_in_overflow:
				dump_stream << "timetrigger_in_overflow ";
				break;
		}
	} else if (dump_enabled) {
		cout << szTriggerName << " TIME " << fsm->GetCurrentTime() << " INPUTS " << input_value << " STATE ";
		switch (input_state) {
			case timetrigger_in_unknown:
				cout << "timetrigger_in_unknown ";
				break;
			case timetrigger_in_underflow:
				cout << "timetrigger_in_underflow ";
				break;
			case timetrigger_in_inrange:
				cout << "timetrigger_in_inrange ";
				break;
			case timetrigger_in_overflow:
				cout << "timetrigger_in_overflow ";
				break;
		}
	}

	if (!was_trigged && new_was_trigged) {
		//Триггер только что сработал. Устанавливаем его парамтры в исходное состояние
		was_trigged = true;
		values[TIMETRIGGER_DIRECT]     = 0;
		values[TIMETRIGGER_DIRECT_PR]  = -1;
		values[TIMETRIGGER_REVERSE_PR] = -1;
		values[TIMETRIGGER_DIRECT_FL]  = -1;
		values[TIMETRIGGER_REVERSE_FL] = -1;

		time_start = fsm->GetCurrentTime();
		//Направление выбирается в соответсвии с режимом работы
		switch (mode) {
			case count_up:
				direction = count_up;
				time_uptrig = time_start + up_time;
				break;
			case count_down:
				direction = count_down;
				time_downtrig = time_start + down_time;
				break;
			case count_up_down:
				direction = count_up;
				time_uptrig = time_start + up_time;
				break;
			default:
				return;
		}
		handle_values_differences();
		return;
	} else 	if (was_trigged && !new_was_trigged) {
		//Триггер был в активном состоянии, но его вход перешел в неактивное.
		//Для триггера ничего не меняется, если на его входе стояла защелка
		//Если вход был комбинационным, то триггер должен сформировать сигнал о невыполнении счета
		if (combinational == latch) {
			//Комбинационный
			if (count_up == direction) {
				values[TIMETRIGGER_DIRECT]     = 0;
				values[TIMETRIGGER_DIRECT_PR]  = -1;
				values[TIMETRIGGER_REVERSE_PR] = -1;
				values[TIMETRIGGER_DIRECT_FL]  =  1; // Сработал
				values[TIMETRIGGER_REVERSE_FL] = -1;
			} else if (count_down == direction) {
				values[TIMETRIGGER_DIRECT]     = 0;
				values[TIMETRIGGER_DIRECT_PR]  = -1;
				values[TIMETRIGGER_REVERSE_PR] = -1;
				values[TIMETRIGGER_DIRECT_FL]  = -1;
				values[TIMETRIGGER_REVERSE_FL] =  1; // Сработал
			}

			handle_values_differences();
			was_trigged = false;
			return;
		}
	}

	//Работа триггера однозначно разрешена. Рассчитываем его выходные состояния
	long long cur_time = fsm->GetCurrentTime();
	if (count_up == direction) {
		//Выполняется счет вверх. Значит, используется время time_uptrig
		values[TIMETRIGGER_DIRECT_PR] = (double)(cur_time-time_start) / (double)up_time;
		values[TIMETRIGGER_REVERSE_PR] = -1;
		values[TIMETRIGGER_DIRECT_FL]  = -1;
		values[TIMETRIGGER_REVERSE_FL] = -1;
		if (cur_time >= time_uptrig) {
			//Досчитано.
			if (values[TIMETRIGGER_DIRECT] < 1.0) {
				values[TIMETRIGGER_DIRECT] = 1.0; //В соответствии со спецификацией устанавливаем в 1
			}

			if (count_up == mode) {
				//Режим счета вверх. Проверяем необходимость остановки работы триггера
				if (loop_once == circle) {
					//Действительно. Надо было один раз досчитать до максимального значения.
					trigger_stopped = true;
				} else if (loop_count == circle ) {
					cout << "CTimeTrigger::ProcessAnchestors error - count mode wasnt implemented" << endl;
					exit(5);
				}

				//Определяем момент следующего срабатывания триггера
				time_start = cur_time;
				time_uptrig = time_start + up_time;
			} else if (count_up_down == mode) {
				//Досчитали до верхней границы, меняем направление счета
				direction = count_down;

				//Определяем момент следующего срабатывания триггера
				time_start = cur_time;
				time_uptrig = time_start + down_time;
			}
		}
	} else if (count_down == direction) {
		//Выполняется счет вверх. Значит, используется время time_uptrig
		values[TIMETRIGGER_DIRECT_PR]  = -1;
		values[TIMETRIGGER_REVERSE_PR] = (double)(cur_time-time_start) / (double)down_time;
		values[TIMETRIGGER_DIRECT_FL]  = -1;
		values[TIMETRIGGER_REVERSE_FL] = -1;
		if (cur_time >= time_downtrig) {
			//Досчитано.

			if (count_down == mode) {
				//Режим счета вниз. Проверяем необходимость остановки работы триггера
				if (loop_once == circle) {
					//Действительно. Надо было один раз досчитать до минимального значения.
					trigger_stopped = true;
				} else if (loop_count == circle ) {
					cout << "CTimeTrigger::ProcessAnchestors error - count mode wasnt implemented" << endl;
					exit(5);
				}

				//Определяем момент следующего срабатывания триггера
				time_start = cur_time;
				time_uptrig = time_start + down_time;
			} else if (count_up_down == mode) {
				//Досчитали до верхней границы, меняем направление счета
				//Режим счета вверх-вниз. Проверяем необходимость остановки работы триггера
				if (loop_once == circle) {
					//Действительно. Надо было один раз досчитать до минимального значения.
					trigger_stopped = true;
				} else if (loop_count == circle) {
					cout << "CTimeTrigger::ProcessAnchestors error - count mode wasnt implemented" << endl;
					exit(5);
				} else {
					//Останавливать триггер не надо. Меняем направление счета
					direction = count_up;
				}

				//Определяем момент следующего срабатывания триггера
				time_start = cur_time;
				time_uptrig = time_start + up_time;
			}
		}
	}

	handle_values_differences();
}

void CTimeTrigger::handle_values_differences() {
	if (dump_enabled && dump_to_file) {
		dump_stream << " OUTS ";
		for (int i=0;i<5;i++) {
			dump_stream << values[i] << " ";
		}
		dump_stream << endl;
	} else if (dump_enabled) {
		cout << " OUTS ";
		for (int i=0;i<5;i++) {
			cout << values[i] << " ";
		}
		cout << endl;
	}
	for (int i=0;i<5;i++) {
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

CTimeTrigger* load_time_trigger(std::string filename) {
	return new CTimeTrigger(filename);
}



