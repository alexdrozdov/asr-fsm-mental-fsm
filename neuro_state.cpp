/*
 * neuro_state.cpp
 *
 *  Created on: 17.07.2011
 *      Author: drozdov
 */

#include <iostream>
#include <sstream>
#include "base_trigger.h"
#include "neuro_trigger.h"
#include "mental_fsm.h"
#include "xml_config.h"

#include "trigger_tree.h"

using namespace std;
using namespace FANN;



CNeuroState::CNeuroState(CNeuroTrigger *trigger, int nstate) {
	this->trigger = trigger;
	xmlConfigPtr xml = (xmlConfigPtr)trigger->pxml_tmp;
	std::ostringstream stream;
	stream << nstate;
	string mypath = "/trigger/states/i" + stream.str();
	cout << "\tCNeuroState::CNeuroState info - loading state from " << mypath << endl;


	//Идентификатор состояния. По идее, должен совпадать с nstate, но это уже на совести программиста ASR
	id = xmlGetIntValue(xml,(mypath + "/id").c_str(),-1);

	//Путь к нейронной сети
	char* tmp_str = xmlGetStringValue(xml,(mypath + "/fann").c_str());
	if (NULL == tmp_str) {
		cout << "CNeuroState::CNeuroState error - couldn`t extract fann file name" << endl;
		return;
	}
	fann_file_name = tmp_str;

	//Путь к файлу, обеспечивающему четкую логику кластера в этом состоянии
	tmp_str = xmlGetStringValue(xml,(mypath + "/logic").c_str());
	if (NULL == tmp_str) {
		cout << "CNeuroState::CNeuroState error - couldn`t extract tcl file name" << endl;
		return;
	}
	tcl_file_name  = tmp_str;

	//Загружаем нейронную сеть из файла
	fann = new neural_net();
	if (!fann->create_from_file(fann_file_name)) {
		cout << "CNeuroState::CNeuroState error - couldn`t create fann from file " << fann_file_name << endl;
		return;
	}


	stable = xmlGetBooleanValue(xml,(mypath+"/stable").c_str(),false);

	input_count  = xmlGetIntValue(xml,(mypath + "/input_links/count").c_str(),-1);
	output_count = xmlGetIntValue(xml,(mypath + "/output_links/count").c_str(),-1);
	state_count  = xmlGetIntValue(xml,(mypath + "/state_coding/count").c_str(),-1);

	if (input_count < 1) {
		cout << "CNeuroState::CNeuroState error - state " << id \
				<< " of " << trigger->szTriggerName \
				<< " has wrong inputs count"<< endl;
		return;
	}
	if (input_count < (int)fann->get_num_input()) {
		cout << "CNeuroState::CNeuroState error - state " << id \
				<< " of " << trigger->szTriggerName \
				<< " input count is less than inputs of fann"<< endl;
		return;
	}
	if (output_count < 1) {
		cout << "CNeuroState::CNeuroState error - state " << id \
				<< " of " << trigger->szTriggerName \
				<< " has wrong outputs count"<< endl;
		return;
	}
	if (state_count < 1) {
		cout << "CNeuroState::CNeuroState error - state " << id \
				<< " of " << trigger->szTriggerName \
				<< " has wrong state coding count"<< endl;
		return;
	}

	net_inputs.resize(input_count);
	net_outputs.resize(output_count);
	next_states.resize(state_count);

	//Загружаем таблицу соединений входов триггера с входами нейронной сети
	string input_common_path = mypath + "/input_links/i";
	for (int i=0;i<input_count;i++) {
		std::ostringstream ostr;
		ostr << i;
		string input_item_path = input_common_path + ostr.str();

		int from = xmlGetIntValue(xml,(input_item_path + "/in").c_str(),-1);
		int to   = xmlGetIntValue(xml,(input_item_path + "/fann").c_str(),-1);
		if (from<0 || to<0) {
			cout << "CNeuroState::CNeuroState error - state " << id \
				<< " of " << trigger->szTriggerName \
				<< " has wrong input-to-fann connections"<< endl;
			return;
		}

		if (from >= (int)trigger->anchs.size()) {
			//Указанный выход за пределами доступных входов триггера
			cout << "CNeuroState::CNeuroState error - state " << id \
				<< " of " << trigger->szTriggerName \
				<< " has connection ot of trigger inputs range"<< endl;
			return;
		}

		if (to >= input_count) {
			//Указанный выход за пределами доступных входов неронной сети
			cout << "CNeuroState::CNeuroState error - state " << id \
				<< " of " << trigger->szTriggerName \
				<< " has connection ot of fann inputs range"<< endl;
			return;
		}

		net_inputs[i].from = from;
		net_inputs[i].to   = to;
	}

	//Загружаем таблицу соединений нейронной сети с выходами триггера
	string out_common_path = mypath + "/output_links/i";
	for (int i=0;i<output_count;i++) {
		std::ostringstream ostr;
		ostr << i;
		string out_item_path = out_common_path + ostr.str();

		int from = xmlGetIntValue(xml,(out_item_path + "/fann").c_str(),-1);
		int to   = xmlGetIntValue(xml,(out_item_path + "/out").c_str(),-1);
		double const_value = xmlGetDoubleValue(xml,(out_item_path + "/value").c_str(),-1);
		bool is_const = xmlGetBooleanValue(xml,(out_item_path + "/const").c_str(),false);

		if (is_const) {
			//Выходное значение триггера фиксировано
			net_outputs[i].from = from;
			net_outputs[i].to   = to;
			net_outputs[i].is_const = true;
			net_outputs[i].value = const_value;

		} else {
			//Выходное значение триггера определяется выходом нейронной сети
			if (from<0 || to<0) {
				cout << "CNeuroState::CNeuroState error - state " << id \
					<< " of " << trigger->szTriggerName \
					<< " has wrong fann-to-out connections"<< endl;
				return;
			}

			if (from >= (int)fann->get_num_output()) {
				//Указанный выход за пределами доступных выходов сети
				cout << "CNeuroState::CNeuroState error - state " << id \
					<< " of " << trigger->szTriggerName \
					<< " has connection ot of fann outputs range"<< endl;
				return;
			}

			if (to >= (int)trigger->out_values_count) {
				//Указанный выход за пределами доступных выходов триггера
				cout << "CNeuroState::CNeuroState error - state " << id \
					<< " of " << trigger->szTriggerName \
					<< " has connection ot of trigger outputs range"<< endl;
				return;
			}

			net_outputs[i].from = from;
			net_outputs[i].to   = to;
			net_outputs[i].is_const = false;
			net_outputs[i].value = 0.0;
		}
	}

	//Загружаем таблицу кодирования следующих состояний триггера
	string state_coding_path = mypath + "/state_coding/i";
	for (int i=0;i<state_count;i++) {
		std::ostringstream ostr;
		ostr << i;
		string state_item_path = state_coding_path + ostr.str();

		int fann_out   = xmlGetIntValue(xml,(state_item_path + "/fann").c_str(),-1);
		int next_state = xmlGetIntValue(xml,(state_item_path + "/state").c_str(),-1);
		if (fann_out<0 || next_state<0) {
			cout << "CNeuroState::CNeuroState error - state " << id \
				<< " of " << trigger->szTriggerName \
				<< " has wrong next-state coding"<< endl;
			return;
		}

		if (fann_out >= (int)fann->get_num_output()) {
			//Указанный выход за пределами доступных выходов сети
			cout << "CNeuroState::CNeuroState error - state " << id \
				<< " of " << trigger->szTriggerName \
				<< " has next state of fann outputs range"<< endl;
			return;
		}

		if (fann_out >= state_count) {
			//Указанный выход за пределами анализируемых выходов нейронной сети
			cout << "CNeuroState::CNeuroState error - state " << id \
				<< " of " << trigger->szTriggerName \
				<< " has next state of analized fann outputs range"<< endl;
			return;
		}

		if (next_state >= (int)trigger->states_count) {
			//Номер состояния выходит за пределы доступных состяний триггера
			cout << "CNeuroState::CNeuroState error - state " << id \
				<< " of " << trigger->szTriggerName \
				<< " has connection ot of trigger outputs range"<< endl;
			return;
		}

		next_states[fann_out] = next_state;
	}
}

void CNeuroState::ProcessInputs() {
}




