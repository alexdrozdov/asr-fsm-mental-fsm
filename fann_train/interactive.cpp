/*
 * interactive.cpp
 *
 *  Created on: 13.10.2011
 *      Author: drozdov
 */


#include "tcl.h"

#include <iostream>
#include <string>
#include <sstream>


#include "interactive.h"

using namespace std;

int Tcl_AppInit (Tcl_Interp *interp);
void create_procs(Tcl_Interp *interp);

int fann_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
int add_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
int train_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
int project_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);


void initialize_interactive(int argc, char *argv[]) {
	Tcl_Main(1, argv, Tcl_AppInit);
}


void run_interactive() {
}

void create_procs(Tcl_Interp *interp) {
	Tcl_CreateCommand(interp,
		"fann",
		fann_handler,
		(ClientData) NULL,
		(Tcl_CmdDeleteProc*) NULL
	);

	Tcl_CreateCommand(interp,
		"add",
		add_handler,
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

    return TCL_OK;
}

int fann_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	if (2 == argc) {
		string aux_cmd = argv[1];
		if ("-inputs" == aux_cmd) {
			stringstream ostr;
			ostr << fann_opts.inputs;
			Tcl_SetResult(interp,(char*)ostr.str().c_str(),TCL_VOLATILE);
			return TCL_OK;
		}
	}
	return TCL_OK;
}

int add_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	return TCL_OK;
}

int train_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	return TCL_OK;
}

int project_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	return TCL_OK;
}




fann_options fann_opts;

