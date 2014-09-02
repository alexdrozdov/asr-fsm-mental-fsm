/*
 * virt_trigger.cpp
 *
 *  Created on: 26.06.2011
 *      Author: drozdov
 */

#include <iostream>
#include <stdlib.h>
#include "virt_trigger.h"
#include "mental_fsm.h"
#include "xml_config.h"
#include "net_link.h"

#include "trigger_tree.h"
#include "global_vars.h"

using namespace std;


CVirtTrigger::CVirtTrigger(string file_name) : CBaseTrigger() {
	xmlConfigPtr xml = CreateXmlConfig((char*)file_name.c_str());
	if (NULL == xml) {
		cout << "CVirtTrigger::CVirtTrigger error: couldn`t load file <" << file_name << ">" << endl;
		return;
	}

	szTriggerName = xmlGetStringValue(xml,"/trigger/name");
	szRemoteTriggerName = xmlGetStringValue(xml,"/trigger/remote_name");

	trigger_id = xmlGetIntValue(xml,"/trigger/id",-1);
	if (-1 == trigger_id) {
		cout << "CVirtTrigger::CVirtTrigger error: trigger_id isn`t specified for <" << szTriggerName << ">" << endl;
		return;
	}
	remote_trigger_id = xmlGetIntValue(xml,"/trigger/remote_id",-1);
	if (-1 == remote_trigger_id) {
		cout << "CVirtTrigger::CVirtTrigger error: remote_trigger_id isn`t specified for <" << szTriggerName << ">" << endl;
		return;
	}

	outputs_count = xmlGetIntValue(xml,"/trigger/output_count",-1);
	if (-1 == outputs_count) {
		cout << "CVirtTrigger::CVirtTrigger error: outputs_count isn`t specified for <" << szTriggerName << ">" << endl;
		return;
	}
	values.resize(outputs_count);

	anchs.resize(1);
	anchs[0] = NULL;

	//Инициализируем запись об этом триггере для дерева триггеров
	tree_leaf = new trigger_tree_leaf;
	tree_leaf->distance = 0;
	tree_leaf->trigger = this;
	trigger_tree->RegisterRootTrigger(tree_leaf);
	net_link->RegisterVirtualTrigger(this);
	fsm->RegisterTrigger(this);

	touched = false;
	dump_enabled = false;
	dump_to_file = false;
}

int CVirtTrigger::BindTo(trig_inout_spec* self, trig_inout_spec* other) {
	if (NULL==self || NULL==other) {
		cout << "CVirtTrigger::BindTo error - zero pointers received" << endl;
		return -1;
	}

	//Проверяем спецификацию выхода этого триггера
	if (self->trigger != this) {
		cout << "CVirtTrigger::BindTo error - wrong pointer self. Differs from real pointer to this class" << endl;
		return 1;
	}
	if (inout_out != self->inout) {
		cout << "CVirtTrigger::BindTo error - wrong output specification" << endl;
		return 1;
	}
	if (self->offs >= (int)values.size()) {
		cout << "CVirtTrigger::BindTo error - output out of range" << endl;
		return 1;
	}

	//Проверяем спецификацию вязываемого триггера
	if (NULL == other->trigger) {
		cout << "CVirtTrigger::BindTo error - zero pointer other." << endl;
		return 1;
	}
	if (inout_out == other->inout) {
		cout << "CVirtTrigger::BindTo error - wrong input specification" << endl;
		return 1;
	}
	//if (other->offs > other->trigger->GetExtendedOutsCount()) {
	//      cout << "CTimeTrigger::BindFrom error - output out of range" << endl;
	//      return 1;
	//}

	//Проверяем, что этого триггера нет в списке зависимых  триггеров
	vector<trig_inout_spec*>::iterator it;
	for ( it=self->trigger->deps.begin() ; it < self->trigger->deps.end(); it++ ) {
		trig_inout_spec* tis = *it;
		if (tis->trigger == other->trigger) {
			cout << "CVirtTrigger::BindTo warning - " << self->trigger->szTriggerName << " was already binded to " << other->trigger->szTriggerName << endl;
			return 1;
		}
	}
	self->trigger->deps.push_back(other);
	return 0;
}

int CVirtTrigger::BindFrom(trig_inout_spec* self, trig_inout_spec* other) {
	//Просто для совместимости. У этого триггера нет входов
	return 0;
}

void CVirtTrigger::SetOutput(int number,double value) {
	if (0>number || number >= outputs_count) {
		cout << "CVirtTrigger::SetOutput atacked - output number out of range" << endl;
		cout << "Will now exit" << endl;
		exit(4);
	}

	//Устанавливаем новое значение триггера
	values[number] = value;

	if (dump_enabled) {
		AddToPostprocess();
	}

	//cout << "CVirtTrigger::SetOutput info - searching output dependencies for " << number << endl;
	vector<trig_inout_spec*>::iterator it;
	for (it=deps.begin();it<deps.end();it++) {
		trig_inout_spec* tis = *it;
		if (tis->offs != number) {
			continue;
		}

		//Помечаем триггер, зависящий от этого входа, как требующий обработки
		//cout << "CVirtTrigger::SetOutput info - dependency found" << endl;
		tis->trigger->AddToDynamicTree();
	}
}

int CVirtTrigger::GetExtendedOutsCount() {
	return outputs_count;
}

bool CVirtTrigger::GetEnabled() {
	return enabled;
}

void CVirtTrigger::SetEnabled(bool b) {
	enabled = b;
}

double CVirtTrigger::GetIntegralState() {
	return 0;
}

double CVirtTrigger::GetExtendedState(int nExtender) {
	if (nExtender<0 || nExtender >= (int)values.size()) {
		cout << "CVirtTrigger::GetExtendedState error - index out of range in " << szTriggerName << endl;
		return -1;
	}

	return values[nExtender];
}

void CVirtTrigger::ProcessAnchestors() {
}

void CVirtTrigger::ReceiveFromRemoteTrigger(remote_trigger_msg *msg) {
}

CVirtTrigger* load_virt_trigger(std::string filename) {
	return new CVirtTrigger(filename);
}

