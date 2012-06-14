/*
 * main.cpp
 *
 *  Created on: 14.06.2011
 *      Author: drozdov
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <string>

#ifdef AUXPACKAGES
#include <tcl.h>
#else
#include <tcl8.5/tcl.h>
#endif

#include "mentalstate.h"
#include "common.h"

using namespace std;

string get_project_path(char* argv0);
bool check_project_path ();

int Tcl_AppInit(Tcl_Interp *interp);

int
main(
    int argc,
    char **argv)
{
	if (argc < 2) {
		cout << "main error: no project specified" << endl;
		cout << "Use --help option for more info" << endl;
		return 1;
	}

	executable_path = get_executable_path(argv[0]);
	project_path = get_project_path(argv[1]);

	cout << "Running from " << executable_path << endl;
	cout << "Project is " << project_path << endl;

	if (!check_project_path()) {
		cout << "main error: project path doesn`t exist or file operate.tcl is missing" << endl;
		return 1;
	}

	Tcl_Main(1, argv, Tcl_AppInit);

    return 0;
}

string get_project_path (char* argv1) {
	string tmp_project_path;
	if ('/' == argv1[0]) {
		//Задан абсолютный путь
		tmp_project_path = argv1;
	} else {
		//Задан относительный путь
		string cwd = getcwd(NULL,0);
		string pr;
		if ('.' == argv1[0]) {
			if ('/' == argv1[1]) {
				pr = argv1+1;
			} else {
				pr = "/";
				pr += argv1+1;
			}
		}
		if ('/' == pr[0]) {
			tmp_project_path = cwd + pr;
		} else {
			tmp_project_path = cwd + "/" + pr;
		}
	}

	if ('/' != tmp_project_path[tmp_project_path.length()-1]) {
		tmp_project_path += "/";
	}

	return tmp_project_path;
}

bool check_project_path () {
	struct stat st;
	if (0 != stat((project_path + "operate.tcl").c_str(),&st)) {
		return false;
	}
	if (!S_ISREG(st.st_mode)) {
		return false;
	}
	return true;
}


int
Tcl_AppInit(
    Tcl_Interp *interp)
{

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


    Tcl_SetVar(interp, "tcl_rcFileName",
       		(project_path + "operate.tcl").c_str(),
       		TCL_GLOBAL_ONLY);

    if (0 != init_mentalstate_structs(interp)) {
    	printf("Couldn`t initialize mentalstate core.\n");
    	return TCL_ERROR;
    }

    return TCL_OK;
}



