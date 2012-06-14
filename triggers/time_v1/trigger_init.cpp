/*
 * trigger_init.cpp
 *
 *  Created on: 29.04.2012
 *      Author: drozdov
 */

#include <iostream>

#include "time_trigger.h"

using namespace std;

int fsm_trigger_init(dynamictrig_init* dt_init) {
	if (NULL == dt_init) {
		return 1;
	}
	if (dt_init->ck_size != sizeof(dynamictrig_init)) {
		std::cout << "time_trigger_v1_init error: supplied dt_init->ck_size differes from local structure size" << std::endl;
		std::cout << "This means, library is incompatible with asr core" << std::endl;
		return 2;
	}

	dt_init->core_init = time_v1_init_core;
	dt_init->tcl_init  = time_v1_init_tcl;
	dt_init->create    = time_v1_create;

	dt_init->fsm_minor = 0;
	dt_init->fsm_major = 0x00010000;

	return 0;
}

int time_v1_init_tcl(Tcl_Interp* interp) {
	return 0;
}

int time_v1_init_core(trig_init_environ* env) {
	if (NULL == env) {
		cout << "time_v1_init_core error - zero pointer to trig_init_environ" << endl;
		return 2;
	}
	if (env->ck_size < sizeof(trig_init_environ)) {
		std::cout << "time_v1_init_core error: supplied env->ck_size is less than local structure size" << std::endl;
		std::cout << "This means, library is incompatible with fsm core" << std::endl;
		return 2;
	}
	if (NULL == env->fsm || NULL==env->trigger_tree) {
		cout << "time_v1_init_core error - zero pointer to fsm or trigger tree object" << endl;
		return 2;
	}
	fsm = env->fsm;
	trigger_tree = env->trigger_tree;
	return 0;
}

void* time_v1_create(std::string filename) {
	return (void*)load_time_trigger(filename);
}

