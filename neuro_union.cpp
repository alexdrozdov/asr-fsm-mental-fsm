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

	szCaption = xmlGetStringValue(xml,(mypath + "/caption").c_str());

	min_time    = xmlGetIntValue(xml, (mypath + "/mintime/value").c_str(), -1);
	time_udf_ns = xmlGetIntValue(xml, (mypath + "/mintime/next_state").c_str(), 0);
	min_time_en = xmlGetBooleanValue(xml, (mypath + "/mintime/enabled").c_str(), 0);

	max_time    = xmlGetIntValue(xml, (mypath + "/maxtime/value").c_str(), -1);
	time_ovf_ns = xmlGetIntValue(xml, (mypath + "/maxtime/next_state").c_str(), 0);
	max_time_en = xmlGetBooleanValue(xml, (mypath + "/maxtime/enabled").c_str(), 0);

	time_enter = 0;
}


bool CNeuroUnion::IsMember(int nstate) {
	return false;
}

int CNeuroUnion::TryState(int nstate) {
	return 0;
}
