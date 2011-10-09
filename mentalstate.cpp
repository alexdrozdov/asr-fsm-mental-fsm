//============================================================================
// Name        : mentalstate.cpp
// Author      : Drozdov A. V.
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <map>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>


#include "tcl.h"

#include "base_trigger.h"
#include "time_trigger.h"
#include "log_trigger.h"
#include "virt_trigger.h"
#include "neuro_trigger.h"
#include "mental_fsm.h"
#include "trigger_tree.h"
#include "net_link.h"

#include "xml_config.h"

using namespace std;

int load_trigger(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
int trigger_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
int bindtrigger_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
int netlink_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
int tree_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
int mkdump_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);

typedef CBaseTrigger* (*trigger_init_ptr)(std::string);

map<string,trigger_init_ptr> load_trigger_handlers;
int init_trigger_handles() {
	load_trigger_handlers["time"]   = (trigger_init_ptr)load_time_trigger;
	load_trigger_handlers["log"]    = (trigger_init_ptr)load_log_trigger;
	load_trigger_handlers["virt"]   = (trigger_init_ptr)load_virt_trigger;
	load_trigger_handlers["neuro"]  = (trigger_init_ptr)load_neuro_trigger;

	return 0;
}

string extract_trigger_type(string filename) {
	xmlConfigPtr xml = CreateXmlConfig((char*)filename.c_str());
	if (NULL == xml) {
		cout << "extract_trigger_type error: couldn`t load file <" << filename << ">" << endl;
		return "error";
	}

	string trigger_type = xmlGetStringValue(xml,"/trigger/type");
	return trigger_type;
}

bool folder_exists(std::string folder_name) {
	struct stat st;
	if (0 != stat(folder_name.c_str(), &st)) {
		return false;
	}
	if (!S_ISDIR(st.st_mode)) {
		return false;
	}
	return true;
}

int load_triggers(Tcl_Interp *interp, string foldername) {
	if (!folder_exists(foldername)) {
		cout << "load_triggers error: folder " << foldername << " not found" << endl;
		return -1;
	}
	FTS      *tree;
	FTSENT   *node;

	char *paths[2];
	paths[0] = (char*)foldername.c_str();
	paths[1] = NULL;

	if ((tree = fts_open(paths, FTS_NOCHDIR, NULL)) == NULL) {
		cout << "load_triggers error: couldn`t open folder " << foldername << endl;
		return -1;
	}

	cout << "Loading triggers from folder <" << foldername <<">..." << endl;
	while ((node = fts_read(tree))) {
		if (S_ISDIR(node->fts_statp->st_mode)) {
			continue;
		}

		string full_xml_name = foldername + node->fts_name;
		if (0 != full_xml_name.compare(full_xml_name.size()-4,4,".xml")) {
			continue;
		}

		string tcl_command = "loadtrigger {" + full_xml_name + "}";
		Tcl_Eval(interp,tcl_command.c_str());
	}

	fts_close(tree);
	return 0;
}

int bind_triggers (Tcl_Interp *interp, string foldername) {
	if (!folder_exists(foldername)) {
		cout << "bind_triggers error: folder " << foldername << " not found" << endl;
		return -1;
	}

	FTS      *tree;
	FTSENT   *node;

	char *paths[2];
	paths[0] = (char*)foldername.c_str();
	paths[1] = NULL;

	if ((tree = fts_open(paths, FTS_NOCHDIR, NULL)) == NULL) {
		cout << "load_triggers error: couldn`t open folder " << foldername << endl;
		return -1;
	}

	cout << "Loading links from folder <" << foldername <<">..." << endl;
	while ((node = fts_read(tree))) {
		if (S_ISDIR(node->fts_statp->st_mode)) {
			continue;
		}

		string full_link_name = foldername + node->fts_name;

		if (0 != full_link_name.compare(full_link_name.size()-4,4,".tcl")) {
			continue;
		}

		cout << full_link_name << endl;
		string tcl_command = "source " + full_link_name;
		Tcl_Eval(interp,tcl_command.c_str());
	}

	fts_close(tree);
	return 0;
}

int init_mentalstate_structs(Tcl_Interp *interp) {

	Tcl_SetVar(interp, "mental_fsm(executable)", executable_path.c_str(), TCL_GLOBAL_ONLY);
	Tcl_SetVar(interp, "mental_fsm(project)", project_path.c_str(), TCL_GLOBAL_ONLY);
	Tcl_SetVar(interp, "mental_fsm(loaded)", "0",TCL_GLOBAL_ONLY);

	Tcl_CreateCommand(interp,
			"loadtrigger",
			load_trigger,
			(ClientData) NULL,
			(Tcl_CmdDeleteProc*) NULL
	);

	Tcl_CreateCommand(interp,
			"trigger",
			trigger_handler,
			(ClientData) NULL,
			(Tcl_CmdDeleteProc*) NULL
	);

	Tcl_CreateCommand(interp,
			"bindtrigger",
			bindtrigger_handler,
			(ClientData) NULL,
			(Tcl_CmdDeleteProc*) NULL
	);

	Tcl_CreateCommand(interp,
			"tree",
			tree_handler,
			(ClientData) NULL,
			(Tcl_CmdDeleteProc*) NULL
	);


	fsm = new MentalFsm();
	init_trigger_handles();
	create_trigger_tree();
	create_net_link(project_path + "netlink.xml");
	if (0 != load_triggers(interp,project_path + "triggers/")) {
		return TCL_ERROR;
	}

	if (0 != bind_triggers(interp,project_path + "links/") ) {
		return TCL_ERROR;
	}

	build_trigger_tree();

	Tcl_CreateCommand(interp,
			"netlink",
			netlink_handler,
			(ClientData) NULL,
			(Tcl_CmdDeleteProc*) NULL
	);

	Tcl_CreateCommand(interp,
			"mkdump",
			mkdump_handler,
			(ClientData) NULL,
			(Tcl_CmdDeleteProc*) NULL
	);

	Tcl_SetVar(interp, "mental_fsm(loaded)", "1",TCL_GLOBAL_ONLY);
	return 0;
}

int load_trigger(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	if (argc < 2) {
		Tcl_SetResult(interp,(char*)"loadtrigger requires file name",TCL_STATIC);
		cout << "loadtrigger error: requires file name" << endl;
		return TCL_ERROR;
	}
	string full_xml_name = argv[1];
	string trigger_type = extract_trigger_type(full_xml_name);

	cout << full_xml_name << ": " << trigger_type << endl;

	//По типу триггера определяем обработчик, который должен его загрузить
	map<string,trigger_init_ptr>::iterator hit = load_trigger_handlers.find(trigger_type);
	if (hit == load_trigger_handlers.end()) {
		//Такой обработчик не зарегистрирован
		cout << "\tError: handler for this trigger type wasn`t registered" << endl;
	} else {
		trigger_init_ptr tip = load_trigger_handlers[trigger_type];
		CBaseTrigger* trig = (CBaseTrigger*)tip(full_xml_name);
		cout << "\tOK: trigger <" << trig->szTriggerName << "> loaded" << endl;
	}
	return TCL_OK;
}

int trigger_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	return TCL_OK;
}

int parse_inout_spec (string spec, trig_inout_spec* spec_struct) {
	unsigned len = spec.size();

	unsigned i = 0;
	while (i<len && spec[i]!=':') {
		i++;
	}

	if (i==len) {
		//Так и не удалось найти разделителей в спецификации
		//Это значит, спецификация триггера ссылается на сам
		//триггер, а не на один из его входов-выходов
		spec_struct->trigger_name = spec;
		spec_struct->trigger = fsm->FindTrigger(spec_struct->trigger_name);
		spec_struct->inout = inout_none;
		spec_struct->offs  = -1;

		return 0;
	}

	spec_struct->trigger_name = spec.substr(0,i);
	spec_struct->trigger = fsm->FindTrigger(spec_struct->trigger_name);
	i++;
	int prev_i = i;
	while (i<len && spec[i]!=':') {
		i++;
	}

	string str_inout_type = spec.substr(prev_i,i-prev_i);
	string str_inout_offs = spec.substr(i+1);

	if ("in" == str_inout_type) {
		spec_struct->inout = inout_in;
	} else if ("out" == str_inout_type) {
		spec_struct->inout = inout_out;
	} else {
		spec_struct->inout = inout_none;
		spec_struct->offs = -1;
	}

	if ("def" == str_inout_offs) {
		spec_struct->offs = 0;
	} else {
		spec_struct->offs = atoi(str_inout_offs.c_str());
	}

	//cout << spec_struct.trigger << " " << str_inout_type << " " << str_inout_offs << endl;

	return 0;
}

int bindtrigger_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	if (argc < 3) {
		Tcl_SetResult(interp,(char*)"bindtrigger requires 2 or more parameters: bindtrigger src dst ?dst?",TCL_STATIC);
		cout << "bindtrigger error: requires 2 or more parameters" << endl;
		return TCL_ERROR;
	}

	string src_spec = argv[1];

	//Определяем триггер, который будет источником сигналов
	trig_inout_spec* tic_src = new trig_inout_spec;
	parse_inout_spec(src_spec,tic_src);
	if (NULL == tic_src->trigger) {
		string err = "bindtrigger couldn`t find source trigger <" + src_spec + ">";
		cout << err << endl;
		Tcl_SetResult(interp,(char*)err.c_str(),TCL_VOLATILE);
		return TCL_ERROR;
	}

	//Просматриваем все триггеры, которые будут приемниками его сигналов.
	for (int i=2;i<argc;i++) {
		string dst_spec = argv[i];
		trig_inout_spec* tic_dst = new trig_inout_spec;
		parse_inout_spec(dst_spec,tic_dst);

		if (NULL == tic_dst->trigger) {
			string err = "bindtrigger couldn`t find dst trigger <" + dst_spec + ">";
			cout << err << endl;
			Tcl_SetResult(interp,(char*)err.c_str(),TCL_VOLATILE);
			return TCL_ERROR;
		}

		//Связываем триггеры между собой
		tic_src->trigger->BindTo(tic_src, tic_dst);
		tic_dst->trigger->BindFrom(tic_dst, tic_src);
	}

	return TCL_OK;
}

int netlink_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	return net_link->TclHandler(interp,argc,argv);
}

int tree_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	if (2 == argc) {
		string strparam = argv[1];
		if ("static" == strparam) {
			Tcl_SetResult(interp,(char*)" ",TCL_STATIC);
			trigger_tree->PrintStaticTree();
			return TCL_OK;
		}
	}
	Tcl_SetResult(interp,(char*)"tree error - wrong params",TCL_STATIC);
	return TCL_ERROR;
}

int mkdump_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	if (argc < 3) {
		return TCL_ERROR;
	}

	string trig_id = argv[1];
	string dump_state = argv[2];

	bool bfile_name = false;
	string file_name = "";
	if (argc > 3 ) {
		bfile_name = true;
		file_name = argv[3];
	}

	bool bdump_enable = false;
	if (dump_state == "enable") {
		bdump_enable = true;
	}

	if (trig_id == "-all") {
		//Управлять дампом всех триггеров. Вывод будет производиться в "имя_файла.имя_триггера"
		cout << "mkdump error: -all wasn`t implemented" << endl;
		return TCL_ERROR;
	} else {
		//Управлять дампом одного триггера.
		CBaseTrigger* tr =  fsm->FindTrigger(trig_id);
		if (NULL == tr) {
			cout << "mkdump error: trigger " << trig_id << " not found" << endl;
			return TCL_ERROR;
		}

		if (!bfile_name) {
			tr->MkDump(bdump_enable);
		} else {
			tr->MkDump(bdump_enable, file_name);
		}
	}

	return TCL_OK;
}



