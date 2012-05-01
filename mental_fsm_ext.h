/*
 * mental_fsm_ext.h
 *
 *  Created on: 24.04.2012
 *      Author: drozdov
 */

#ifndef MENTAL_FSM_EXT_H_
#define MENTAL_FSM_EXT_H_

#ifdef MACOSX
#include <tcl.h>
#else
#include <tcl8.5/tcl.h>
#endif

#include "base_trigger.h"
#include "mental_fsm.h"
#include "trigger_tree.h"

typedef struct _trig_init_environ {
	unsigned int ck_size;
	MentalFsm* fsm;
	CTriggerTree* trigger_tree;
} trig_init_environ;

typedef int (*trigger_tcl_init_proc)(Tcl_Interp* interp);
typedef int (*trigger_core_init_proc)(trig_init_environ* env);
typedef void* (*trigger_create_proc)(std::string filename);

typedef struct _dynamictrig_init {
	unsigned int ck_size;
	trigger_tcl_init_proc  tcl_init;  //Функция, отвечающая за инициализацию интерпретатора
	trigger_core_init_proc core_init; //Функция, отвечающая за инициализацию основного функционала
	trigger_create_proc    create;    //Функция, создающая новый экземпляр триггера

	unsigned int fsm_minor;          //Минимальная совместимая версия fsm
	unsigned int fsm_major;          //Максимальная совместимая версия fsm
} dynamictrig_init;

typedef int (*triggerlib_init_proc)(dynamictrig_init* dt_init);


#endif /* MENTAL_FSM_EXT_H_ */
