/*
 * base_trigger.cpp
 *
 *  Created on: 09.07.2011
 *      Author: drozdov
 */

#include <vector>
#include <string>

#include "base_trigger.h"
#include "trigger_tree.h"
#include "global_vars.h"

using namespace std;

CBaseTrigger::CBaseTrigger() :
	trigger_type(0),
	value(0),
	enabled(true),
	prohibit_time(0),
	tree_leaf(NULL),
	touched(false),
	postprocess(false),
	dump_enabled(false),
	dump_to_file(false)
{}

void CBaseTrigger::AddToDynamicTree() {
	if (touched) {
		//Триггер и так уже добавлен в дерево
		return;
	}
	touched = true;

	trigger_tree->lookup_tree[tree_leaf->distance]->push(tree_leaf);
}

void CBaseTrigger::PostprocessTasks() {
	//Дамп. Ничего другого по-умолчанию в постпроцессинг не входит
	if (dump_enabled && dump_to_file) {
		int n = (int)values.size();
		for (int i=0;i<n;i++) {
			dump_stream << values[i] << " ";
		}
		dump_stream << endl;
	} else if (dump_enabled) {
		int n = (int)values.size();
		for (int i=0;i<n;i++) {
			cout << values[i] << " ";
		}
		cout << endl;
	}
}


void CBaseTrigger::AddToPostprocess() {
	if (postprocess) {
		//Триггер и так уже добавлен в дерево
		return;
	}
	postprocess = true;

	trigger_tree->postprocess_qq.push(tree_leaf);
}

int CBaseTrigger::MkDump(bool enable) {
	if (dump_enabled) {
		//Дамп и так выводится. Закрываем существующий дескрптор
		if (dump_stream != cout) {
			dump_stream.close();
		}
	}
	dump_enabled = enable;
	dump_to_file = false;
	return 0;
}

int CBaseTrigger::MkDump(bool enable, string file_name) {
	if (dump_enabled) {
		//Дамп и так выводится. Закрываем существующий дескриптор
		if (dump_stream != cout) {
			dump_stream.close();
		}
	}

	dump_enabled = enable;
	if (dump_enabled) {
		if ("cout" == file_name) {
			dump_to_file = false;
		} else {
			dump_to_file = true;
			dump_stream.open(file_name.c_str());
		}
	}
	return 0;
}




