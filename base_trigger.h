/*
 * base_trigger.h
 *
 *  Created on: 11.06.2011
 *      Author: drozdov
 */

#ifndef BASE_TRIGGER_H_
#define BASE_TRIGGER_H_

#include <string>
#include <vector>
#include <fstream>

#include "trigger_tree.h"

//Условие работы триггера
enum ECommonTriggerLatch {
	combinational = 0, //Только если действует разрешивший его сигнал
	triggered = 1      //Запущенный однократно триггер отработает как минимум один цикл
};

enum ECommonTriggerSense {
	sense_trigup = 0,
	sense_trigdown = 1,
	sense_trigrange = 2
};



class CBaseTrigger;

typedef struct _trig_inout_spec {
	std::string trigger_name;
	CBaseTrigger* trigger;
	int inout;
	int offs;
} trig_inout_spec;

enum inout_spec_type {
	inout_none = 0,
	inout_in = 1,
	inout_out = 2,
};

class CTriggerTree;

class CBaseTrigger {
public:
	std::string szTriggerName; //Название триггера

	virtual bool GetEnabled() = 0;
	virtual void SetEnabled(bool b) = 0;

	virtual double GetIntegralState() = 0;
	virtual double GetExtendedState(int nExtender) = 0;
	virtual int GetExtendedOutsCount() = 0;

	virtual void ProcessAnchestors() = 0;
	virtual void PostprocessTasks();
	virtual void AddToDynamicTree();
	virtual void AddToPostprocess();

	virtual int BindTo(trig_inout_spec* self, trig_inout_spec* other) = 0;
	virtual int BindFrom(trig_inout_spec* self, trig_inout_spec* other) = 0;

	virtual int MkDump(bool enable);
	virtual int MkDump(bool enable, std::string file_name);


	int trigger_type; //Тип триггера (в числовом виде)

	std::vector<trig_inout_spec*> deps; //Список зависимых триггеров
	std::vector<trig_inout_spec*> anchs; //Список триггеров, от которых этот триггер зависит

	double value; //Базовое значение триггера
	std::vector<double> values; //Раширенные состояния триггера. Содержат более подробные вероятности возникновения событий
	bool enabled;

	long long prohibit_time; //Время до которого срабатывание триггера запрещено

	trigger_tree_leaf* tree_leaf; //Запись об этом триггере в неизменяемом дереве

	bool touched; //Признак того, что триггер уже добавлен в динамическое дерево
	bool postprocess; //Признак того, что триггер уже добавлен в список пост обработки

	bool dump_enabled; //Триггер должен выводить входные и выходные парамеры при каждом вызове ProcessAnchestors
	bool dump_to_file;
	std::ofstream dump_stream; //Поток, в который выводится дамп
};



#endif /* BASE_TRIGGER_H_ */
