/*
 * neuro_union.cpp
 *
 *  Created on: 03.09.2011
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

CNeuroUnion::CNeuroUnion(CNeuroTrigger *trigger, int nunion) {
	this->trigger = trigger;
	xmlConfigPtr xml = (xmlConfigPtr)trigger->pxml_tmp;
	std::ostringstream stream;
	stream << nunion;
	string mypath = "/trigger/unions/i" + stream.str();
	cout << "\tCNeuroUnion::CNeuroUnion info - loading union from " << mypath << endl;


	//Идентификатор объединения. По идее, должен совпадать с nstate, но это уже на совести программиста ASR
	id = xmlGetIntValue(xml,(mypath + "/id").c_str(),-1);
	if (id != nunion) {
		cout << "\tCNeuroUnion::CNeuroUnion error - id value differs from element number" << endl;
	}

	szCaption = xmlGetStringValue(xml,(mypath + "/caption").c_str());

	min_time    = xmlGetIntValue(xml, (mypath + "/mintime/value").c_str(), -1);
	time_udf_ns = xmlGetIntValue(xml, (mypath + "/mintime/next_state").c_str(), 0);
	min_time_en = xmlGetBooleanValue(xml, (mypath + "/mintime/enabled").c_str(), 0);

	max_time    = xmlGetIntValue(xml, (mypath + "/maxtime/value").c_str(), -1);
	time_ovf_ns = xmlGetIntValue(xml, (mypath + "/maxtime/next_state").c_str(), 0);
	max_time_en = xmlGetBooleanValue(xml, (mypath + "/maxtime/enabled").c_str(), 0);

	int nmembers = xmlGetIntValue(xml, "/trigger/states/count", -1);
	members.resize(nmembers,false);
	time_enter = 0;
}


bool CNeuroUnion::IsMember(int nstate) {
	//FIXME Добавить проверку номера состояния, в которое выполняется переход. Проверка должна выполняться только при включении специального режима
	return members[nstate];
}

void CNeuroUnion::AddMemeber(int nstate) {
	if (nstate >= (int)members.size()) {
		cout << "CNeuroUnion::AddMemeber error - попытка добавить член объединения с номером " << nstate << ", превышающим количество членов" << endl;
		return;
	}
	members[nstate] = true;

}

int CNeuroUnion::TryState(int nstate) {
	long long union_time = fsm->GetCurrentTime() - time_enter; //Время, в течение которого триггер находился в этом кластере
	cout << "Current time " << fsm->GetCurrentTime() << endl;
	cout << "Union time " << fsm->GetCurrentTime() - time_enter << endl;
	cout << "Max time " << max_time << endl;
	bool stay_in_union = members[nstate]; //Совершается попытка перехода в состояние, которое выведет триггер из кластера
	cout << "Stay in union " << stay_in_union << endl;

	bool time_underflow = !stay_in_union && min_time_en && (union_time < min_time); //Триггер пытается покинуть состояние до истечения мимнимального времени
	bool time_overflow = stay_in_union && max_time_en && (union_time > max_time); //Триггер пытается остаться в состоянии по истечении максимального времени

	if (time_underflow) {
		//Предлагаем переход в альтернативное состояние из-за невыполнения требований по минимальной длительности
		return time_udf_ns;
	}
	if (time_overflow) {
		//Предлагаем переход в альтернативное состояние из-за превышения времни находждения в кластере
		return time_ovf_ns;
	}

	return nstate;
}

void CNeuroUnion::Enter() {
	time_enter = fsm->GetCurrentTime();
}
