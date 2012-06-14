/*
 * mental_fsm.cpp
 *
 *  Created on: 11.06.2011
 *      Author: drozdov
 */

#include <iostream>
#include "mental_fsm.h"
#include "trigger_tree.h"
#include "net_link.h"
#include "global_vars.h"

using namespace std;

MentalFsm::MentalFsm() {
	current_time = 0;

	local_samplerate = 1;
	remote_samplerate = 1;

	min_samplerate = 10;
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

bool MentalFsm::UnregisterTrigger(CBaseTrigger* trigger) {
	if (NULL == trigger) {
		cout << "MentalFsm::UnregisterTrigger error. Null ptr to trigger" << endl;
		return false;
	}

	//Находим триггер среди зарегистрированных
	vector<CBaseTrigger*>::iterator it;
	for ( it=triggers.begin() ; it < triggers.end(); it++ ) {
		if (trigger != *it) continue;
		triggers.erase(it);
		break;
	}

	//Удаляем его имя
	trig_dict.erase(trigger->szTriggerName);

	//При необходимости удаляем из списка триггеров времени
	for ( it=time_triggers.begin() ; it < time_triggers.end(); it++ ) {
		if (trigger != *it) continue;
		time_triggers.erase(it);
		break;
	}

	//Удаляем его самого
	delete trigger;
	return true;
}

bool MentalFsm::UnregisterTrigger(std::string name) {
	CBaseTrigger* tr = FindTrigger(name);
	if (NULL == tr)
		return false;

	return UnregisterTrigger(tr);
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

void MentalFsm::TouchTimeTriggers() {
	std::vector<CBaseTrigger*>::iterator it;
	for (it=time_triggers.begin();it!=time_triggers.end();it++)
		(*it)->AddToDynamicTree();
}

int MentalFsm::StepToTime(long long new_time) {
	trigger_tree_leaf* ttl = NULL;
	//Обрабатываем триггеры с точки зрения функциональной работы
	while (NULL != (ttl = trigger_tree->ExtractCandidate()))
		ttl->trigger->ProcessAnchestors();

	//Пост процессинг (например, для вывода дампов)
	while (NULL != (ttl = trigger_tree->ExtractPostprocess()))
		ttl->trigger->PostprocessTasks();

	return 0;
}

int MentalFsm::RunToTime(long long new_time) {
	max_time_step = GetLocalSamplerate() / min_samplerate;
	while(new_time - current_time > max_time_step) {
		//Устанавливаем все триггеры времени
		TouchTimeTriggers();

		StepToTime(current_time+max_time_step);
		current_time += max_time_step;
	}
	TouchTimeTriggers();
	StepToTime(new_time);
	current_time = new_time;
	return 0;
}

void MentalFsm::SetLocalSamplerate(unsigned int samplerate) {
	local_samplerate = samplerate;
}

void MentalFsm::SetRemoteSamplerate(unsigned int samplerate) {
	remote_samplerate = samplerate;
}

void MentalFsm::SetMinSamplerate(unsigned int samplerate) {
	min_samplerate = samplerate;
}

unsigned int MentalFsm::GetLocalSamplerate() {
	return local_samplerate;
}

unsigned int MentalFsm::GetRemoteSamplerate() {
	return remote_samplerate;
}

unsigned int MentalFsm::GetMinSamplerate() {
	return min_samplerate;
}

long long MentalFsm::ScaleRemoteTime(long long remote_time) {
	return remote_time * local_samplerate / remote_samplerate;
}

void MentalFsm::SendResponse(std::string response_text) {
	net_link->SendTextResponse(response_text);
}


MentalFsm* fsm = NULL;




