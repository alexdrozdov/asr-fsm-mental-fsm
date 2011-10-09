/*
 * trigger_tree.h
 *
 *  Created on: 23.06.2011
 *      Author: drozdov
 */

#ifndef TRIGGER_TREE_H_
#define TRIGGER_TREE_H_

#include <iostream>
#include <vector>
#include <queue>


class CBaseTrigger;

struct trigger_tree_leaf {
	CBaseTrigger* trigger;
	int distance;
};


typedef std::vector< std::vector<trigger_tree_leaf*>* > CTriggerTreeVector;
typedef std::vector< std::queue<trigger_tree_leaf*>* > CTriggerTreeVectorD;


class CTriggerTree {
public:
	CTriggerTree();

	void RegisterRootTrigger(trigger_tree_leaf* trigger_leaf);
	void RegisterChildTrigger(trigger_tree_leaf* trigger_leaf);
	int BuildTree();

	void PrintStaticTree();
	void PrintLookupTree();

	trigger_tree_leaf* ExtractCandidate();
	trigger_tree_leaf* ExtractPostprocess();

	CTriggerTreeVector static_tree; //Неизменяемое дерево, создаваемое в процессе загрузки триггеров
	CTriggerTreeVectorD lookup_tree; //Динамически заполняемое дерево: принимает триггеры, которые должны быть обработаны
	std::queue<trigger_tree_leaf*> postprocess_qq; //Динамически заполняемое дерево: принимае триггеры, которые требуют дополнительной обработки после основной обработки
	//Например, структура может использоваться для регистрации изменений состояния триггеров в файле
private:
	std::vector<trigger_tree_leaf*> root_triggers;  //Корневые триггеры с нулевой глубиной
	std::vector<trigger_tree_leaf*> child_triggers; //зависимые триггеры

	std::queue<trigger_tree_leaf*> to_process; //Триггеры, которые должны быть обработаны для построения дерева

	void ProcessOneTrigger(trigger_tree_leaf* trigger_leaf);
	int max_distance;
};

extern CTriggerTree* trigger_tree;

extern void create_trigger_tree();
extern void build_trigger_tree();

#endif /* TRIGGER_TREE_H_ */
