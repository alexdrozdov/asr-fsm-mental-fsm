/*
 * interactive.cpp
 *
 *  Created on: 13.10.2011
 *      Author: drozdov
 */


#include <tcl.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>


#include <pcre.h>
#include "fann.h"

#include "common.h"
#include "interactive.h"
#include "fann_handlers.h"
#include "project_handlers.h"
#include "cfg_serialize.pb.h"
#include "train_tools.h"

using namespace std;
using namespace fann_train_cfg;

int Tcl_AppInit (Tcl_Interp *interp);
int Tcl_AppInitScript (Tcl_Interp *interp);
void create_procs(Tcl_Interp *interp);

int fann_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
int data_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
int train_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
int project_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);

extern project_info project;
extern fann *ann;

bool load_input_entry_data(train_entry *ten);
bool load_output_entry_data(train_entry *ten);
bool load_project(std::string file_name);


int train_io::entries_count() {
	switch(type) {
		case iot_value:
		case iot_vect:
			return 1;
		case iot_file:
		case iot_matrix:
			return (int)fvalues.size();
	}
	return 0;
}
int train_io::entry_size() {
	switch(type) {
		case iot_value:
			return 1;
		case iot_vect:
			return (int)values.size();
		case iot_file:
		case iot_matrix:
			return (int)fvalues[0].size();
	}
	return 0;
}

fann_type* train_io::to_mem(int nrow) {
	fann_type *v = NULL;
	switch(type) {
		case iot_value:
			v = new fann_type[1];
			*v = (fann_type)value;
			break;
		case iot_vect:
			v = new fann_type[values.size()];
			for (int i=0;i<(int)values.size();i++)
				v[i] = (fann_type)values[i];
			break;
		case iot_file:
		case iot_matrix:
			v = new fann_type[fvalues[0].size()];
			std::vector<double>& val_row = fvalues[nrow<(int)fvalues.size()?nrow:fvalues.size()-1];
			for (int i=0;i<(int)val_row.size();i++)
				v[i] = (fann_type)val_row[i];
			break;
	}
	return v;
}

int train_entry::entries_count() {
	int ie = input.entries_count();
	int oe = output.entries_count();
	return ie>oe?ie:oe;
}

void train_entry::to_train_rows(std::vector<train_row>& to) {
	int nentries = entries_count();
	for (int i=0;i<nentries;i++) {
		train_row tr;
		tr.input_count = input.entry_size();
		tr.output_count = output.entry_size();
		tr.inputs = input.to_mem(i);
		tr.outputs = output.to_mem(i);
		to.push_back(tr);
	}
}

void run_script(int argc, char *argv[]) {
	Tcl_Main(1, argv, Tcl_AppInitScript);
}

void run_interactive(int argc, char *argv[]) {
	Tcl_Main(1, argv, Tcl_AppInit);
}

void create_procs(Tcl_Interp *interp) {
	Tcl_CreateCommand(interp,
		"fann",
		fann_handler,
		(ClientData) NULL,
		(Tcl_CmdDeleteProc*) NULL
	);

	Tcl_CreateCommand(interp,
		"data",
		data_handler,
		(ClientData) NULL,
		(Tcl_CmdDeleteProc*) NULL
	);

	Tcl_CreateCommand(interp,
		"train",
		train_handler,
		(ClientData) NULL,
		(Tcl_CmdDeleteProc*) NULL
	);

	Tcl_CreateCommand(interp,
		"project",
		project_handler,
		(ClientData) NULL,
		(Tcl_CmdDeleteProc*) NULL
	);
}

int Tcl_AppInit (Tcl_Interp *interp) {

	Tcl_Eval(interp,
"proc tclInit { } {\n"
"global tcl_library\n"
"set current_dir [file dirname [info nameofexecutable]]\n"
"set tclfile [file join $current_dir library init.tcl]\n"
"set tcl_library [file join $current_dir library]\n"
"}\n"
"tclInit\n");

    if (Tcl_Init(interp) == TCL_ERROR) {
    	return TCL_ERROR;
    }

    create_procs(interp);

    if (run_options.load_project)
    	load_project(run_options.project_file);

    return TCL_OK;
}

int Tcl_AppInitScript (Tcl_Interp *interp) {

	Tcl_Eval(interp,
"proc tclInit { } {\n"
"global tcl_library\n"
"set current_dir [file dirname [info nameofexecutable]]\n"
"set tclfile [file join $current_dir library init.tcl]\n"
"set tcl_library [file join $current_dir library]\n"
"}\n"
"tclInit\n");

    if (Tcl_Init(interp) == TCL_ERROR) {
    	return TCL_ERROR;
    }

    create_procs(interp);
    string script_path = build_project_path(run_options.script_file_name);
    size_t last_slash = script_path.rfind('/');
    project_path = script_path.substr(0,last_slash+1);

    string str_source_cmd = "source \"";
    str_source_cmd += script_path;
    str_source_cmd += "\"\n";
    Tcl_Eval(interp, str_source_cmd.c_str());

    return TCL_OK;
}


void test_train_data(std::string train_name) {
	vector<train_entry>::iterator it = find_if(train_entries.begin(),train_entries.end(), check_entry_name(train_name));
	if (train_entries.end() == it) {
		return;
	}

	if (NULL == ann) {
		cout << "test_train_data error - нейронная сеть еще не создана. Перед проверкой результатов обучения нейронной сети сеть должна быть обучена" << endl;
		return;
	}
	if (!fann_opts.trained) {
		cout << "test_train_data error - нейронная сеть еще не обучена" << endl;
		return;
	}
	if (!validate_fann_opts()) {
		cout << "test_train_data error - проверка нейронной сети невозможна, т.к. настройки не прошли контроль" << endl;
		return;
	}

	fann_type *fann_input = new fann_type[fann_opts.inputs];
	fann_type *res = NULL;
	switch(it->input.type) {
		case iot_file:
		case iot_matrix:
			for (vector<vector<double> >::iterator sample_it = it->input.fvalues.begin();sample_it != it->input.fvalues.end();sample_it++) {
				for (int i=0;i<(int)sample_it->size();i++) {
					fann_input[i] = sample_it->operator [](i);
				}
				res = fann_run(ann, fann_input);
				for (int i=0;i<fann_opts.outputs;i++) {
					cout << res[i] << " ";
				}
				cout << endl;
			}
			break;
		case iot_value:
			fann_input[0] = it->input.value;
			res = fann_run(ann, fann_input);
			break;
		case iot_vect:
			cout << "\tvalues: ";
			for_each(it->input.values.begin(),it->input.values.end(),cout_numeric<double>);
			cout << endl;
			break;
	}
	delete[] fann_input;
}

int train_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	return TCL_OK;
}


check_entry_name::check_entry_name(std::string name) {
	n = name;
}

bool check_entry_name::operator()(train_entry &ten) {
	return ten.name == n;
}

template<class T> void cout_numeric(T v) {
	cout << " " << v;
}


fann_options fann_opts;

