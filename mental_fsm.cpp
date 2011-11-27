/*
 * mental_fsm.cpp
 *
 *  Created on: 11.06.2011
 *      Author: drozdov
 */

#include <iostream>
#include "mental_fsm.h"
#include "trigger_tree.h"

using namespace std;

MentalFsm::MentalFsm() {
	current_time = 0;

	local_samplerate = 1;
	remote_samplerate = 1;
}

int MentalFsm::RegisterTimeTrigger(CBaseTrigger* trigger) {
	if (NULL == trigger) {
		cout << "MentalFsm::RegisterTimeTrigger error. Null ptr to trigger" << endl;
		return -1;
	}

	//Проверяем, что такого триггера нет среди зарегистрированных триггеров
	vector<CBaseTrigger*>::iterator it;
	for ( it=time_triggers.begin() ; it < time_triggers.end(); it++ ) {
		if (trigger == *it) {
			cout << "MentalFsm::RegisterTimeTrigger error. Trigger already was registered" << endl;
			cout << "Trigger name: " << trigger->szTriggerName << endl;
			return -1;
		}
	}

	time_triggers.push_back(trigger);
	return time_triggers.size() - 1;
}

int MentalFsm::RegisterTrigger(CBaseTrigger* trigger) {
	if (NULL == trigger) {
		cout << "MentalFsm::RegisterTrigger error. Null ptr to trigger" << endl;
		return -1;
	}


	//Проверяем, что такого триггера нет среди зарегистрированных триггеров
	vector<CBaseTrigger*>::iterator it;
	for ( it=triggers.begin() ; it < triggers.end(); it++ ) {
		if (trigger == *it) {
			cout << "MentalFsm::RegisterTrigger error. Trigger already was registered" << endl;
			cout << "Trigger name: " << trigger->szTriggerName << endl;
			return -1;
		}
	}

	//Проверяем, что имя триггера уникально
	map<string,CBaseTrigger*>::iterator mit = trig_dict.find(trigger->szTriggerName);
	if (mit != trig_dict.end()) {
		//Найден
		cout << "MentalFsm::RegisterTrigger error. Trigger with this name already was registered" << endl;
		cout << "Trigger name: " << trigger->szTriggerName << endl;
		return -1;
	}


	triggers.push_back(trigger);
	trig_dict[trigger->szTriggerName] = trigger;
	return triggers.size() - 1;
}

CBaseTrigger* MentalFsm::FindTrigger(std::string name) {
	map<string,CBaseTrigger*>::iterator mit = trig_dict.find(name);
	if (mit != trig_dict.end())
		return mit->second;

	return NULL;
}

long long MentalFsm::GetCurrentTime() {
	return current_time;
}

int MentalFsm::RunToTime(long long new_time) {
	//Сначала обрабатываем все изменения, которые не связаны с временем:
	//обрабатывем цепочку триггеров, измененную сообщениями от клиента
	trigger_tree_leaf* ttl = NULL;
	//Обрабатываем триггеры с точки зрения функциональной работы
	while (NULL != (ttl = trigger_tree->ExtractCandidate()))
		ttl->trigger->ProcessAnchestors();

	//Пост процессинг (например, для вывода дампов)
	while (NULL != (ttl = trigger_tree->ExtractPostprocess()))
			ttl->trigger->PostprocessTasks();

	current_time = new_time;
	return 0;
}

void MentalFsm::SetLocalSamplerate(unsigned int samplerate) {
	local_samplerate = samplerate;
}

void MentalFsm::SetRemoteSamplerate(unsigned int samplerate) {
	remote_samplerate = samplerate;
}

unsigned int MentalFsm::GetLocalSamplerate() {
	return local_samplerate;
}

unsigned int MentalFsm::GetRemoteSamplerate() {
	return remote_samplerate;
}

long long MentalFsm::ScaleRemoteTime(long long remote_time) {
	return remote_time * local_samplerate / remote_samplerate;
}


MentalFsm* fsm = NULL;




