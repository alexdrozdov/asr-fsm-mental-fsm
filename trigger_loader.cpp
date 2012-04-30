/*
 * processor_loader.cpp
 *
 *  Created on: 03.08.2011
 *      Author: drozdov
 */

#include <dlfcn.h>

#include <iostream>
#include "trigger_tree.h"
#include "base_trigger.h"
#include "mental_fsm.h"
#include "global_vars.h"

#include "trigger_loader.h"

using namespace std;

std::map<std::string, trigger_init_ptr> load_trigger_handlers;

int load_trigger_lib(std::string name, std::string libname) {
	if ('.' == libname[0]) {
		libname = libname.substr(1);
	}
	if ('/' == libname[0]) {
		libname = libname.substr(1);
	}

	if (0 != libname.compare(0,3,"lib")) {
		cout << "load_trigger_lib error: library name " << libname << "couldn`t be used because prefix 'lib' is missing" << endl;
		return 1;
	}
	if (0 != libname.compare(libname.size()-3,3,".so")) {
		cout << "load_trigger_lib error: library name " << libname << "couldn`t be used because extension '.so' is missing" << endl;
		return 1;
	}

	string driver_name = libname.substr(3,libname.length()-6);

	string library_name = executable_path + libname;
	cout << "Loading library " << library_name << endl;
	string init_func_name = "fsm_trigger_init";

	dlerror();
	void* libhandle = dlopen(library_name.c_str(),RTLD_LAZY);
	if (0 == libhandle) {
		cout << "load_trigger_lib error: couldn`t load library" << endl;
		char* msg = dlerror();
		if (NULL != msg) {
			cout << "Error was: " << msg << endl;
		}
		return 2;
	}

	triggerlib_init_proc tr_init_ptr = (triggerlib_init_proc)dlsym(libhandle,init_func_name.c_str());
	if (NULL == tr_init_ptr) {
		cout << "load_trigger_lib error: couldn`t find symbol " << init_func_name << " in library" << endl;
		dlclose(libhandle);
		return 2;
	}

	dynamictrig_init dti;
	dti.ck_size = sizeof(dynamictrig_init);

	if (0 != tr_init_ptr(&dti)) {
		return 2;
	}

	trig_init_environ tie;
	tie.ck_size = sizeof(trig_init_environ);
	tie.fsm = fsm;
	tie.trigger_tree = trigger_tree;

	if (0 != dti.core_init(&tie)) {
		cout << "load_trigger_lib error: couldn`t execute core init proc" << endl;
		dlclose(libhandle);
		return 2;
	}

	load_trigger_handlers[name] = (trigger_init_ptr)dti.create;
	return 0;
}
