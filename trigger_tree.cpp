/*
 * trigger_tree.cpp
 *
 *  Created on: 23.06.2011
 *      Author: drozdov
 */

#include "trigger_tree.h"
#include "base_trigger.h"
#include "mental_fsm.h"

using namespace std;

CTriggerTree::CTriggerTree() {
	max_distance = 0;
}

void CTriggerTree::RegisterRootTrigger(trigger_tree_leaf* trigger_leaf) {
	root_triggers.push_back(trigger_leaf);
}

void CTriggerTree::RegisterChildTrigger(trigger_tree_leaf* trigger_leaf) {
	child_triggers.push_back(trigger_leaf);
}

int CTriggerTree::BuildTree() {
	cout << "Building trigger tree..." << endl;
	cout << "\tevaluting distances" << endl;
	//Просматриваем все корневые триггеры и добавляем их потомков в дерево просмотра
	for (unsigned int i=0;i<root_triggers.size();i++) {
		CBaseTrigger* root_trig = root_triggers[i]->trigger;
		//Просматриваем всех потомков этого триггера
		for (unsigned int j=0;j<root_trig->deps.size();j++) {
			CBaseTrigger* dep = root_trig->deps[j]->trigger;
			dep->tree_leaf->distance = 1; //Устанавливаем этому триггеру первый уровень
			to_process.push(dep->tree_leaf); //Добавляем его в постобработку
		}
	}

	//Просматриваем список триггеров, нуждающихся в обработке до тех пор, пока он не закончится
	while (!to_process.empty()) {
		trigger_tree_leaf* leaf = to_process.front();
		to_process.pop();
		ProcessOneTrigger(leaf);
	}
	cout << "\tcomplete, max distance is " << max_distance << endl;
	cout << "\tbuilding tree" << endl;

	//Инициируем векторы со слоями
	static_tree.resize(max_distance+1);
	lookup_tree.resize(max_distance+1);
	for (int i=0;i<=max_distance;i++) {
		static_tree[i] = new vector<trigger_tree_leaf*>(0);
		lookup_tree[i] = new queue<trigger_tree_leaf*>;//vector<trigger_tree_leaf*>(0);
	}

	//Добавляем все корневые триггеры в нулевой слой
	vector<trigger_tree_leaf*>::iterator it;
	for (it=root_triggers.begin() ; it < root_triggers.end(); it++) {
		static_tree[0]->push_back(*it);
	}

	//Все остальные триггеры добавляем в ветви в соответсвии с их растоянием
	for (it=child_triggers.begin() ; it < child_triggers.end(); it++) {
		trigger_tree_leaf* tl = *it;
		if (tl->distance > 0) {
			static_tree[tl->distance]->push_back(tl);
		} else {
			cout << "CTriggerTree::BuildTree warning - " << tl->trigger->szTriggerName << " is out of hierarhy" << endl;
			cout << "It will be removed from trigger tree" << endl;
			fsm->UnregisterTrigger(tl->trigger);
		}
	}
	cout << "\tcomplete" << endl;
	return 0;
}

void CTriggerTree::PrintStaticTree() {
	cout << "Static trigger tree" << endl;
	for (unsigned i=0;i<static_tree.size();i++) {
		cout << "LAYER " << i << endl;

		vector<trigger_tree_leaf*>::iterator it;
		for (it=static_tree[i]->begin() ; it < static_tree[i]->end(); it++) {
			trigger_tree_leaf* tl = *it;
			cout << tl->trigger->szTriggerName << " ";
		}
		cout << endl;
	}
}

trigger_tree_leaf* CTriggerTree::ExtractCandidate() {
	CTriggerTreeVectorD::iterator it;
	for (it=lookup_tree.begin();it<lookup_tree.end();it++) {
		queue<trigger_tree_leaf*>*  qttl = *it;

		if (qttl->size() > 0) {
			trigger_tree_leaf* ttl = qttl->front();
			qttl->pop();
			ttl->trigger->touched = false;
			return ttl;
		}
	}

	return NULL;
}

trigger_tree_leaf* CTriggerTree::ExtractPostprocess() {
	if (postprocess_qq.size() > 0) {
		trigger_tree_leaf *ttl = postprocess_qq.front();
		postprocess_qq.pop();
		ttl->trigger->postprocess = false;
		return ttl;
	}

	return NULL;
}

void CTriggerTree::PrintLookupTree() {
	cout << "CTriggerTree::PrintLookupTree - function is unsupported due to architectural problems" << endl;
	//cout << "Lookup trigger tree" << endl;
	//for (unsigned i=0;i<lookup_tree.size();i++) {
	//	cout << "LAYER " << i << endl;

	//	queue<trigger_tree_leaf*>::iterator it;
	//	for (it=lookup_tree[i]->begin() ; it < lookup_tree[i]->end(); it++) {
	//		trigger_tree_leaf* tl = *it;
	//		cout << tl->trigger->szTriggerName << " ";
	//	}
	//	cout << endl;
	//}
}

void CTriggerTree::ProcessOneTrigger(trigger_tree_leaf* trigger_leaf) {
	CBaseTrigger* trigger = trigger_leaf->trigger;

	max_distance = (max_distance>trigger_leaf->distance)?max_distance:trigger_leaf->distance;

	int next_distance = trigger_leaf->distance + 1; //Расстояние, которое должно получиться в зависимых  триггерах
	vector<trig_inout_spec*>::iterator it;
	for ( it=trigger->deps.begin() ; it < trigger->deps.end(); it++ ) {
		CBaseTrigger* cur_dep = (*it)->trigger;
		if (cur_dep->tree_leaf->distance < next_distance) {
			//Увеличиваем расстояние до триггера. А сам триггер добавляем для дальнейшей обработки
			cur_dep->tree_leaf->distance = next_distance;

			to_process.push(cur_dep->tree_leaf);
		}
	}
}

CTriggerTree* trigger_tree = NULL;


void create_trigger_tree() {
	trigger_tree = new CTriggerTree();
}

void build_trigger_tree() {
	trigger_tree->BuildTree();
}

