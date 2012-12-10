/*
 * interactive.cpp
 *
 *  Created on: 13.10.2011
 *      Author: drozdov
 */


#include "tcl.h"

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

enum io_type {
	iot_value = 0,
	iot_file  = 1,
	iot_vect  = 2
};

typedef struct _train_io {
	int entries_count();
	int entry_size();

	fann_type* to_mem(int nrow);

	int type;                   //Тип
	std::string file_name;      //Файл, используемый для ввода данных
	std::string extract_regex;  //Регулярное выражение, используемое для извлечения данных из файла
	double value;               //Одиночное целочисленное значение
	std::vector<double> values; //Набор значений, единый для всех значений другого входа/выхода
	std::vector< std::vector<double> > fvalues; //Набор значений, загруженный из файла
} train_io;

struct train_entry {
	int entries_count();
	void to_train_rows(std::vector<train_row>& to);
	std::string name;
	train_io input;
	train_io output;
};

typedef struct _project_info {
	std::string name;
	std::string file_name;
	std::string path;

	bool loaded;
	bool file_name_present;
	bool saved;
} project_info;

std::vector<train_row> train_rows;

project_info project;

std::vector<train_entry> train_entries;

fann *ann = NULL;

bool load_input_entry_data(train_entry *ten);
bool load_output_entry_data(train_entry *ten);
bool load_project(std::string file_name);


int train_io::entries_count() {
	switch(type) {
		case iot_value:
		case iot_vect:
			return 1;
		case iot_file:
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
			v = new fann_type[fvalues[0].size()];
			std::vector<double>& val_row = fvalues[nrow];
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
    	load_project(build_file_path(run_options.project_file));

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
    string script_path = build_file_path(run_options.script_file_name);
    size_t last_slash = script_path.rfind('/');
    project_path = script_path.substr(0,last_slash+1);

    string str_source_cmd = "source \"";
    str_source_cmd += script_path;
    str_source_cmd += "\"\n";
    Tcl_Eval(interp, str_source_cmd.c_str());

    return TCL_OK;
}

std::string build_def_regexp(int ndecimals) {
	std::string def_regexp = "^\\s*";
	for (int i=0;i<ndecimals-1;i++) {
		def_regexp += "([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\\s";
	}
	def_regexp += "([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)";

	return def_regexp;
}

bool validate_fann_opts() {
	//Проверяем, что заданы параметры сети
	if (fann_opts.inputs < 1) {
		cout << "validate_fann_opts error - не задано количество входов" << endl;
		return false;
	}
	if (fann_opts.outputs < 1) {
		cout << "validate_fann_opts error - не задано количество выходов" << endl;
		return false;
	}
	if (fann_opts.hidden_neurons < 1) {
		cout << "validate_fann_opts error - не задано количество нейронов в скрытом слое" << endl;
		return false;
	}
	if (fann_opts.layers < 1) {
		cout << "validate_fann_opts error - не задано количество скрытых слоев" << endl;
		return false;
	}
	if (fann_opts.max_epochs < 1) {
		cout << "validate_fann_opts error - не задано количество эпох обучения" << endl;
		return false;
	}
	if (fann_opts.log_period < 1) {
		cout << "validate_fann_opts error - не задано частота вывода результотов обучения" << endl;
		return false;
	}

	//Проверяем, что количество входов-выходов в данных для обучения равно заданным параметрам нейронной сети
	vector<train_entry>::iterator it;
	for (it=train_entries.begin();it!=train_entries.end();it++) {
		train_entry& ten = *it;
		switch(ten.input.type) {
			case iot_vect:
				if (fann_opts.inputs != (int)ten.input.entry_size()) {
					cout << "validate_fann_opts error - количество входных данных для обучающей выборки " << ten.name << " не соответствует количеству входов нейронной сети" << endl;
					return false;
				}
				break;
			case iot_file:
				if (fann_opts.inputs != (int)ten.input.entry_size()) {
					cout << "validate_fann_opts error - количество входных данных для обучающей выборки " << ten.name << " не соответствует количеству входов нейронной сети" << endl;
					return false;
				}
				break;
			case iot_value:
				if (fann_opts.inputs != 1) {
					cout << "validate_fann_opts error - количество входных данных для обучающей выборки " << ten.name << " не соответствует количеству входов нейронной сети" << endl;
					return false;
				}
				break;
			default:
				cout << "validate_fann_opts error - не указан тип обучающей выборки для входа" << endl;
				return false;
		}
		switch(ten.output.type) {
			case iot_vect:
				if (fann_opts.outputs != (int)ten.output.entry_size()) {
					cout << "validate_fann_opts error - количество входных данных для обучающей выборки " << ten.name << " не соответствует количеству выходов нейронной сети" << endl;
					return false;
				}
				break;
			case iot_file:
				if (fann_opts.outputs != (int)ten.output.entry_size()) {
					cout << "validate_fann_opts error - количество входных данных для обучающей выборки " << ten.name << " не соответствует количеству выходов нейронной сети" << endl;
					return false;
				}
				break;
			case iot_value:
				if (fann_opts.outputs != 1) {
					cout << "validate_fann_opts error - количество входных данных для обучающей выборки " << ten.name << " не соответствует количеству выходов нейронной сети" << endl;
					return false;
				}
				break;
			default:
				cout << "validate_fann_opts error - не указан тип обучающей выборки для выхода" << endl;
				return false;
		}
	}

	return true;
}

void usr_add_train( unsigned int num, unsigned int num_input, unsigned int num_output, fann_type* input, fann_type* output) {
	train_row& tr = train_rows[num];
	for (unsigned int i=0;i<num_input;i++)
		input[i] = tr.inputs[i];
	for (unsigned int i=0;i<num_output;i++)
		output[i] = tr.outputs[i];
}


fann_train_data* merge_train_data() {
	train_rows.clear(); //Удаляем все данные, которые могдли храниться
	vector<train_entry>::iterator it;
	for (it=train_entries.begin();it!=train_entries.end();it++) //Сводим все данные в один большой вектор
		it->to_train_rows(train_rows);

	fann_train_data* fdt = fann_create_train_from_callback(train_rows.size(), fann_opts.inputs, fann_opts.outputs, usr_add_train);

	train_rows.clear(); //Сносим все лишнее
	return fdt;
}

bool create_fann() {
	if (NULL != ann) {
		return true; //Нейронная сеть уже создана.
	}
	ann = fann_create_standard(fann_opts.layers,
			fann_opts.inputs,
			fann_opts.hidden_neurons,
			fann_opts.outputs);

	fann_set_activation_function_hidden(ann, FANN_SIGMOID_SYMMETRIC);
	fann_set_activation_function_output(ann, FANN_SIGMOID_SYMMETRIC);

	return true;
}

bool train_fann() {
	//FIXME Реализовать функцию
	if (!validate_fann_opts()) {
		cout << "train_fann error - обучение нейронной сети невозможно, т.к. настройки не прошли контроль" << endl;
		return false;
	}
	fann_train_data* fdt = merge_train_data();
	if (NULL == fdt) {
		cout << "train_fann error - обучение нейронной сети невозможно, т.к. не удалось сформировать обучающую выборку" << endl;
		return false;
	}

	create_fann(); //При необходимости создаем нейронную сеть
	fann_train_on_data(ann, fdt,fann_opts.max_epochs, fann_opts.log_period,(float)fann_opts.desired_error); //Тренируем ее
	double err = fann_get_MSE(ann); //И определяем ошибку
	cout << "Погрешность по результатам обучения: " << err << endl;
	bool ok = false;
	if (err < fann_opts.desired_error) {
		ok = true;
		cout << "Обучение прошло упешно" << endl;
	} else {
		cout << "Обучение не удалось, т.к. не достигнута заданная погрешность" << endl;
	}

	fann_opts.trained = true; //Независимо от результата сеть обучалась и может работать.

	fann_destroy_train(fdt);
	return ok;
}

bool validate_fann_train() {
	if (NULL == ann) {
		cout << "validate_fann_train error - нейронная сеть еще не создана. Перед проверкой результатов обучения нейронной сети сеть должна быть обучена" << endl;
		return false;
	}
	if (!fann_opts.trained) {
		cout << "validate_fann_train error - нейронная сеть еще не обучена" << endl;
		return false;
	}
	if (!validate_fann_opts()) {
		cout << "validate_fann_train error - проверка нейронной сети невозможна, т.к. настройки не прошли контроль" << endl;
		return false;
	}
	fann_train_data* fdt = merge_train_data();
	if (NULL == fdt) {
		cout << "validate_fann_train error - обучение нейронной сети невозможно, т.к. не удалось сформировать обучающую выборку" << endl;
		return false;
	}

	double err = fann_test_data(ann, fdt); //Проверяем сеть на обучающей выборке
	cout << "Погрешность, определенная по результатам проверки: " << err << endl;
	bool ok = false;
	if (err < fann_opts.desired_error) {
		ok = true;
		cout << "Проверка пройдена" << endl;
	} else {
		cout << "Сеть не прошла проверку на обучающих данных" << endl;
	}

	fann_destroy_train(fdt);
	return ok;
}

bool save_fann() {
	if (NULL == ann) {
		cout << "save_fann error - нейронная сеть еще не создана" << endl;
		return false;
	}
	if (!fann_opts.trained) {
		cout << "save_fann error - нейронная сеть еще не обучена" << endl;
		return false;
	}

	if(fann_opts.file_name.length() < 1) {
		cout << "save_fann error - не задано имя файла с обученной нейронной сети" << endl;
		return false;
	}

	fann_save(ann, build_project_path(fann_opts.file_name).c_str());

	return true;
}

bool load_fann(std::string file_name) {
	file_name = build_project_path(file_name);
	ann = fann_create_from_file(file_name.c_str());
	if (NULL == ann) {
		cout << "load_fann error - не удалось загрузить нейронную сеть из файла " << file_name << endl;
		return false;
	}
	return true;
}

int fann_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	if (2 == argc) {
		string aux_cmd = argv[1];
		if ("inputs" == aux_cmd) {
			stringstream ostr;
			ostr << fann_opts.inputs;
			Tcl_SetResult(interp,(char*)ostr.str().c_str(),TCL_VOLATILE);
			return TCL_OK;
		}
		if ("outputs" == aux_cmd) {
			stringstream ostr;
			ostr << fann_opts.outputs;
			Tcl_SetResult(interp,(char*)ostr.str().c_str(),TCL_VOLATILE);
			return TCL_OK;
		}
		if ("layers" == aux_cmd) {
			stringstream ostr;
			ostr << fann_opts.layers;
			Tcl_SetResult(interp,(char*)ostr.str().c_str(),TCL_VOLATILE);
			return TCL_OK;
		}
		if ("hidden" == aux_cmd) {
			stringstream ostr;
			ostr << fann_opts.hidden_neurons;
			Tcl_SetResult(interp,(char*)ostr.str().c_str(),TCL_VOLATILE);
			return TCL_OK;
		}
		if ("desired-error" == aux_cmd) {
			stringstream ostr;
			ostr << fann_opts.desired_error;
			Tcl_SetResult(interp,(char*)ostr.str().c_str(),TCL_VOLATILE);
			return TCL_OK;
		}
		if ("max-epochs" == aux_cmd) {
			stringstream ostr;
			ostr << fann_opts.max_epochs;
			Tcl_SetResult(interp,(char*)ostr.str().c_str(),TCL_VOLATILE);
			return TCL_OK;
		}
		if ("log-period" == aux_cmd) {
			stringstream ostr;
			ostr << fann_opts.log_period;
			Tcl_SetResult(interp,(char*)ostr.str().c_str(),TCL_VOLATILE);
			return TCL_OK;
		}
		if ("trained" == aux_cmd) {
			if (fann_opts.trained)
				Tcl_SetResult(interp, (char*)"yes", TCL_STATIC);
			else
				Tcl_SetResult(interp, (char*)"no", TCL_STATIC);
			return TCL_OK;
		}
		if ("file" == aux_cmd) {
			Tcl_SetResult(interp, (char*)fann_opts.file_name.c_str(), TCL_VOLATILE);
			return TCL_OK;
		}
		if ("save" == aux_cmd) {
			if (save_fann()) {
				return TCL_OK;
			}
			Tcl_SetResult(interp, (char*)"Не удалось сохранить обученную нейронную сеть", TCL_STATIC);
			return TCL_ERROR;
		}
		if ("train" == aux_cmd) {
			if (train_fann()) {
				return TCL_OK;
			}
			Tcl_SetResult(interp, (char*)"Не удалось обучить нейронную сеть", TCL_STATIC);
			return TCL_ERROR;
		}
		if ("validate" == aux_cmd) {
			if (validate_fann_train()) {
				return TCL_OK;
			}
			Tcl_SetResult(interp, (char*)"Не удалось проверить результаты обучения нейронной сети", TCL_STATIC);
			return TCL_ERROR;
		}
	}

	if (3 == argc) {
		string aux_cmd = argv[1];
		std::basic_stringstream<char> aux_param;
		aux_param << argv[2];
		if ("inputs" == aux_cmd) {
			aux_param >> fann_opts.inputs;
			fann_opts.def_in_regexp = build_def_regexp(fann_opts.inputs);
			return TCL_OK;
		}
		if ("outputs" == aux_cmd) {
			aux_param >> fann_opts.outputs;
			fann_opts.def_out_regexp = build_def_regexp(fann_opts.outputs);
			return TCL_OK;
		}
		if ("layers" == aux_cmd) {
			aux_param >> fann_opts.layers;
			return TCL_OK;
		}
		if ("hidden" == aux_cmd) {
			aux_param >> fann_opts.hidden_neurons;
			return TCL_OK;
		}
		if ("desired-error" == aux_cmd) {
			aux_param >> fann_opts.desired_error;
			return TCL_OK;
		}
		if ("max-epochs" == aux_cmd) {
			aux_param >> fann_opts.max_epochs;
			return TCL_OK;
		}
		if ("log-period" == aux_cmd) {
			aux_param >> fann_opts.log_period;
			return TCL_OK;
		}
		if ("file" == aux_cmd) {
			aux_param >> fann_opts.file_name;
			return TCL_OK;
		}
	}
	Tcl_SetResult(interp, (char*)"wrong options. Usage:\n"
"inputs        - количество входов нейронной сети\n"
"outputs       - количество выходов нейронной сети\n"
"layers        - количество слоев\n"
"hidden        - количество нейронов в скрытом слое\n"
"desired-error - максимально допустимая ошибка\n"
"max-epochs    - максимальное количество эпох обучения\n"
"log-period    - периодичность вывода результатов тренировки"
"file          - файл с обученной нейронной сетью\n"
"save          - сохранить результаты обучения в файл"
"trained       - признак того, что нейронная сеть обучена по последним данным\n"
"train         - обучить неронную сеть"
"validate data - проверить нейронную сеть по данным"
			, TCL_STATIC);
	return TCL_ERROR;
}

std::string build_data_list() {
	string res = "{";
	vector<train_entry>::iterator it;
	for (it=train_entries.begin();it != train_entries.end(); it++) {
		res += it->name + " ";
	}

	return res + "}";
}

class check_entry_name {
	string n;
public:
	check_entry_name(std::string name) {
		n = name;
	}
	bool operator()(train_entry &ten) {
		return ten.name == n;
	}
};

bool remove_data_entry (std::string name) {
	vector<train_entry>::iterator it = find_if(train_entries.begin(),train_entries.end(), check_entry_name(name));
	if (train_entries.end() == it) {
		return false;
	}
	train_entries.erase(it);
	return true;
}

bool reload_data_entry (std::string name) {
	vector<train_entry>::iterator it = find_if(train_entries.begin(),train_entries.end(), check_entry_name(name));
	if (train_entries.end() == it) {
		return false;
	}

	if (!load_input_entry_data(&(*it))) {
		return false;
	}

	if (!load_output_entry_data(&(*it))) {
		return false;
	}
	return true;
}

template<class T> void cout_numeric(T v) {
	cout << " " << v;
}

bool show_data_entry (std::string name) {
	vector<train_entry>::iterator it = find_if(train_entries.begin(),train_entries.end(), check_entry_name(name));
	if (train_entries.end() == it) {
		return false;
	}

	cout << "input:" << endl;
	switch(it->input.type) {
		case iot_file:
			cout << "\tfile:  " << it->input.file_name << endl;
			cout << "\tregex: " << it->input.extract_regex << endl;
			break;
		case iot_value:
			cout << "\tvalue: " << it->input.value << endl;
			break;
		case iot_vect:
			cout << "\tvalues: ";
			for_each(it->input.values.begin(),it->input.values.end(),cout_numeric<double>);
			cout << endl;
			break;
	}

	cout << "output:" << endl;
	switch(it->output.type) {
		case iot_file:
			cout << "\tfile:  " << it->output.file_name << endl;
			cout << "\tregex: " << it->output.extract_regex << endl;
			break;
		case iot_value:
			cout << "\tvalue: " << it->output.value << endl;
			break;
		case iot_vect:
			cout << "\tvalues: ";
			for_each(it->output.values.begin(),it->output.values.end(),cout_numeric<double>);
			cout << endl;
			break;
	}
	return true;
}

void dump_train_io(train_io *tio) {
	switch(tio->type) {
		case iot_file:
			cout << "\ffile: " << tio->file_name << endl;
			for (vector<vector<double> >::iterator it = tio->fvalues.begin();it != tio->fvalues.end();it++) {
				cout << "\t\t";
				for_each(it->begin(),it->end(),cout_numeric<double>);
				cout << endl;
			}
			break;
		case iot_value:
			cout << "\tvalue: " << tio->value << endl;
			break;
		case iot_vect:
			cout << "\tvalues: ";
			for_each(tio->values.begin(),tio->values.end(),cout_numeric<double>);
			cout << endl;
			break;
	}
}

bool dump_data_entry (std::string name) {
	vector<train_entry>::iterator it = find_if(train_entries.begin(),train_entries.end(), check_entry_name(name));
	if (train_entries.end() == it) {
		return false;
	}

	cout << "input:" << endl;
	dump_train_io(&(it->input));
	cout << "output:" << endl;
	dump_train_io(&(it->output));
	return true;
}

void split_to_double(const std::string &s, char delim, std::vector<double> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
		std::basic_stringstream<char> stream_item;
		stream_item << item;
		double val = 0.0;
		stream_item >> val;
        elems.push_back(val);
    }
}

bool load_input_entry_data(train_entry *ten) {
	if (NULL == ten) {
		cout << "load_entry_data error - null pointer to train entry" << endl;
		return false;
	}
	if (ten->input.type != iot_file) {
		return true;
	}

	if (0 == fann_opts.inputs) {
		cout << "load_entry_data error - не указано количество входов нейронной сети" << endl;
	}

	const char *error = NULL;
	int   erroffset = 0;
	int   ovector[99];
	pcre *re = pcre_compile (ten->input.extract_regex.c_str(),          /* the pattern */
					   0, //PCRE_MULTILINE
					   &error,         /* for error message */
					   &erroffset,     /* for error offset */
					   0);             /* use default character tables */
	if (NULL == re) {
		cout << "pcre_compile error (offset: " << erroffset <<"), " << error << endl;
		return false;
	}

	ifstream ifs (build_project_path(ten->input.file_name).c_str() , ifstream::in);
	if (!ifs.is_open()) {
		cout << "load_entry_data error - не удалось загрузить данные из файла " << ten->input.file_name << endl;
		return false;
	}

	ten->input.fvalues.resize(0);
	char tmp_cstr[1024] = {0};
	while (ifs.good()) {
		ifs.getline(tmp_cstr, 1024);
		unsigned int len    = strlen(tmp_cstr);
		if (len < 1) {
			continue;
		}
		int rc = pcre_exec(re, 0, tmp_cstr, len, 0, 0, ovector, 99);
		if (rc < fann_opts.inputs+1) {
			cout << "Ошибка при обработке строки " << tmp_cstr << endl;
			return 0;
		}

		vector<double> tmp_vec(fann_opts.inputs);
		for(int i = 1; i < fann_opts.inputs+1; ++i) {
			tmp_vec[i-1] = strtod(tmp_cstr + ovector[2*i],NULL);
		}
		ten->input.fvalues.push_back(tmp_vec);
	}
	ifs.close();
	cout << ten->name << " - прочитано " << ten->input.fvalues.size() << " значений" << endl;

	return true;
}

bool load_output_entry_data(train_entry *ten) {
	if (NULL == ten) {
		cout << "load_entry_data error - null pointer to train entry" << endl;
		return false;
	}
	if (ten->output.type != iot_file) {
		return true;
	}

	if (0 == fann_opts.outputs) {
		cout << "load_entry_data error - не указано количество выходов нейронной сети" << endl;
	}

	const char *error = NULL;
	int   erroffset = 0;
	int   ovector[99];
	pcre *re = pcre_compile (ten->output.extract_regex.c_str(),          /* the pattern */
					   0, //PCRE_MULTILINE
					   &error,         /* for error message */
					   &erroffset,     /* for error offset */
					   0);             /* use default character tables */
	if (NULL == re) {
		cout << "pcre_compile error (offset: " << erroffset <<"), " << error << endl;
		return false;
	}

	ifstream ifs (build_project_path(ten->output.file_name).c_str() , ifstream::in);
	if (!ifs.is_open()) {
		cout << "load_entry_data error - не удалось загрузить данные из файла " << ten->output.file_name << endl;
		return false;
	}

	ten->output.fvalues.resize(0);
	char tmp_cstr[1024] = {0};
	while (ifs.good()) {
		ifs.getline(tmp_cstr, 1024);
		unsigned int len    = strlen(tmp_cstr);
		if (len < 1) {
			continue;
		}
		int rc = pcre_exec(re, 0, tmp_cstr, len, 0, 0, ovector, 99);
		if (rc < fann_opts.outputs+1) {
			cout << "Ошибка при обработке строки " << tmp_cstr << endl;
			return 0;
		}

		vector<double> tmp_vec(fann_opts.outputs);
		for(int i = 1; i < fann_opts.outputs+1; ++i) {
			tmp_vec[i-1] = strtod(tmp_cstr + ovector[2*i],NULL);
		}
		ten->output.fvalues.push_back(tmp_vec);
	}
	ifs.close();
	cout << ten->name << " - прочитано " << ten->output.fvalues.size() << " значений" << endl;

	return true;
}

bool add_entry (int argc, const char *argv[]) {
	if (0==fann_opts.inputs || 0==fann_opts.outputs) {
		cout << "add_entry error - не задано количество входов и/или выходов нейронной сети" << endl;
		return false;
	}
	if (0 == argc) {
		cout << "data add error - no options specified. Usage:" << endl;
		cout << "data add [-input file-name]|[-inputs {values array}] ?-regexp expression? [-output file-name]|[-outputs {values array}] ?-regexp expression?" << endl;
		return true;
	}
	train_entry ten;
	vector<string> args;
	for (int i=0;i<argc;i++) {
		args.push_back(argv[i]);
	}

	ten.name = args[0];

	vector<train_entry>::iterator it = find_if(train_entries.begin(),train_entries.end(), check_entry_name(ten.name));
	if (train_entries.end() != it) {
		cout << "add_entry error - запись с именем " << ten.name << " уже существует" << endl;
		return false;
	}

	int nused_args = 1;

	if (nused_args<argc && "-input" == args[nused_args]) {
		//Вход задан как файл с входными параметрами
		ten.input.type = iot_file;
		ten.input.file_name = args[nused_args+1];

		nused_args += 2;
	} else if (nused_args<argc && "-inputs" == args[nused_args]) {
		//Входы заданы в виде единого списка для любого выходного параметра
		ten.input.type = iot_vect;
		split_to_double(args[nused_args+1],' ', ten.input.values);
		if ((int)ten.input.values.size() != fann_opts.inputs) {
			cout << "add_entry error - количество входных значений не соответсвует количеству входов нейронной сети" << endl;
			return false;
		}

		nused_args += 2;
	} else if (nused_args<argc && "-file" == args[nused_args]) {
		//Входы и выходы заданы в одном единственном файле
	} else if (nused_args>=argc) {
		cout << "data add error - неверный формат команды" << endl;
		return false;
	} else {
		cout << "Ошибка. Неизвестный тип входа" << endl;
		return false;
	}

	if (nused_args<argc && "-regexp" == args[nused_args]) {
		ten.input.extract_regex = args[nused_args+1];
		nused_args += 2;
	} else if (nused_args>=argc){
		cout << "Ошибка. Неизвестный тип входа" << endl;
		return false;
	} else {
		ten.input.extract_regex = fann_opts.def_in_regexp;
	}

	if (nused_args<argc && "-output" == args[nused_args]) {
		//Вход задан как файл с входными параметрами
		ten.output.type = iot_file;
		ten.output.file_name = args[nused_args+1];
	} else if (nused_args<argc && "-outputs" == args[nused_args]) {
		//Выходы заданы в виде единого списка для любого выходного параметра
		ten.output.type = iot_vect;
		split_to_double(args[nused_args+1],' ', ten.output.values);
		if ((int)ten.output.values.size() != fann_opts.outputs) {
			cout << "add_entry error - количество выходных значений не соответсвует количеству выходов нейронной сети" << endl;
			return false;
		}
	} else if (nused_args>=argc){
		cout << "data add error - неверный формат команды" << endl;
		return false;
	} else {
		cout << "Ошибка. Неизвестный тип выхода" << endl;
		return false;
	}

	if (nused_args<argc && "-regexp" == args[nused_args]) {
		ten.output.extract_regex = args[nused_args+1];
		nused_args += 2;
	} else {
		ten.output.extract_regex = fann_opts.def_out_regexp;
	}

	if (!load_input_entry_data(&ten)) {
		return false;
	}

	if (!load_output_entry_data(&ten)) {
		return false;
	}

	train_entries.push_back(ten);
	return true;
}

void save_project() {
	if (!project.file_name_present) {
		return;
	}

	fann_train_cfg::FannTrainProject ftp;

	ftp.set_name(project.name);
	ftp.set_file_name(project.file_name);
	ftp.set_description("");

	::fann_train_cfg::FannTrainProject_FannOptions* fo = ftp.mutable_fann_opts();
	fo->set_num_input(fann_opts.inputs);
	fo->set_num_output(fann_opts.outputs);
	fo->set_num_layers(fann_opts.layers);
	fo->set_num_neurons_hidden(fann_opts.hidden_neurons);
	fo->set_max_epochs(fann_opts.max_epochs);
	fo->set_desired_error(fann_opts.desired_error);
	fo->set_epochs_between_reports(fann_opts.log_period);

	if (fann_opts.file_name.length() > 0) {
		fo->set_fann_file_name(fann_opts.file_name);
		if (fann_opts.trained)
			fo->set_fann_trained(true);
	}

	fo->set_b_h(true);
	fo->set_b_i(true);
	fo->set_b_l(true);
	fo->set_b_n(true);
	fo->set_b_o(true);
	fo->set_b_t(true);

	std::vector<train_entry>::iterator it;
	for (it=train_entries.begin();it!=train_entries.end();it++) {
		train_entry& ten = *it;

		::fann_train_cfg::FannTrainProject_TrainEntry* fte = ftp.add_entries();
		fte->set_name(ten.name);
		::fann_train_cfg::FannTrainProject_TrainIo* tio = fte->mutable_input();
		switch(ten.input.type) {
			case iot_value:
				tio->set_type(FannTrainProject_TrainIo_IoType_IOT_VALUE);
				tio->set_value(ten.input.value);
				break;
			case iot_file:
				tio->set_type(FannTrainProject_TrainIo_IoType_IOT_FILE);
				tio->set_file_name(ten.input.file_name);
				tio->set_extract_regex(ten.input.extract_regex);
				for (vector< vector<double> >::iterator fit=ten.input.fvalues.begin();fit!=ten.input.fvalues.end();fit++) {
					FannTrainProject_TrainIo_FValues* fv = tio->add_fvalues();
					for (vector<double>::iterator dit=fit->begin();dit!=fit->end();dit++) {
						fv->add_values(*dit);
					}
				}
				break;
			case iot_vect:
				tio->set_type(FannTrainProject_TrainIo_IoType_IOT_VECTOR);
				for (vector<double>::iterator dit=ten.input.values.begin();dit!=ten.input.values.end();dit++) {
					tio->mutable_values()->Add(*dit);
				}
				break;
		}

		tio = fte->mutable_output();
		switch(ten.output.type) {
			case iot_value:
				tio->set_type(FannTrainProject_TrainIo_IoType_IOT_VALUE);
				tio->set_value(ten.output.value);
				break;
			case iot_file:
				tio->set_type(FannTrainProject_TrainIo_IoType_IOT_FILE);
				tio->set_file_name(ten.output.file_name);
				tio->set_extract_regex(ten.output.extract_regex);
				for (vector< vector<double> >::iterator fit=ten.output.fvalues.begin();fit!=ten.output.fvalues.end();fit++) {
					FannTrainProject_TrainIo_FValues* fv = tio->add_fvalues();
					for (vector<double>::iterator dit=fit->begin();dit!=fit->end();dit++) {
						fv->add_values(*dit);
					}
				}
				break;
			case iot_vect:
				tio->set_type(FannTrainProject_TrainIo_IoType_IOT_VECTOR);
				for (vector<double>::iterator dit=ten.output.values.begin();dit!=ten.output.values.end();dit++) {
					tio->mutable_values()->Add(*dit);
				}
				break;
		}
	}



	fstream output(build_project_path(project.file_name).c_str(), ios::out | ios::trunc | ios::binary);
	if (!ftp.SerializeToOstream(&output)) {
	  cerr << "Не удалось сохранить проект в файл " << project.file_name << endl;
	  return;
	}

	project.file_name_present = true;
}

void saveas_project(std::string file_name) {
	project.file_name = file_name;
	if (file_name.length()>0) {
		project.file_name_present = true;
	}
}

bool load_project(std::string file_name) {
	file_name = build_project_path(file_name); //Определяем полный путь к файлу проекта
	size_t last_slash = file_name.rfind('/');
	project_path = file_name.substr(0,last_slash+1); //Переносим рабочий каталог в папку с проектом

	fann_train_cfg::FannTrainProject ftp;
	fstream input(file_name.c_str(), ios::in | ios::binary);
	if (!input) {
	  cout << "load_project error - файл " << file_name << " не найден" << endl;
	  return false;
	} else if (!ftp.ParseFromIstream(&input)) {
	  cerr << "Не удалось загрузить проект" << endl;
	  return false;
	}

	project.name = ftp.name();
	project.file_name = file_name;
	project.loaded = true;
	project.saved  = true;

	fann_opts.inputs = ftp.fann_opts().num_input();
	fann_opts.outputs = ftp.fann_opts().num_output();
	fann_opts.layers = ftp.fann_opts().num_layers();
	fann_opts.hidden_neurons = ftp.fann_opts().num_neurons_hidden();
	fann_opts.max_epochs = ftp.fann_opts().max_epochs();
	fann_opts.desired_error = ftp.fann_opts().desired_error();
	fann_opts.log_period = ftp.fann_opts().epochs_between_reports();

	if (ftp.fann_opts().has_fann_file_name()) {
		fann_opts.file_name = ftp.fann_opts().fann_file_name();
		fann_opts.trained = load_fann(fann_opts.file_name);
	} else {
		fann_opts.file_name = "";
		fann_opts.trained = false;
	}

	fann_opts.def_in_regexp = build_def_regexp(fann_opts.inputs);
	fann_opts.def_out_regexp = build_def_regexp(fann_opts.outputs);

	//Загружаем обучающие данные
	int nentries = ftp.entries_size();
	for (int i=0;i<nentries;i++) {
		const FannTrainProject_TrainEntry& fte = ftp.entries(i);

		train_entry ten;
		ten.name = fte.name();
		ten.input.type = (int)fte.input().type();
		ten.output.type = (int)fte.output().type();

		switch(ten.input.type) {
			case iot_value:
				ten.input.value = fte.input().value();
				break;
			case iot_file:
				{
					ten.input.extract_regex = fte.input().extract_regex();
					::google::protobuf::RepeatedPtrField< ::fann_train_cfg::FannTrainProject_TrainIo_FValues >::const_iterator it;
					for (it=fte.input().fvalues().begin();it!=fte.input().fvalues().end();it++) {
						vector<double> vd;
						int nvalues = it->values_size();
						for (int j=0;j<nvalues;j++) {
							vd.push_back(it->values(j));
						}
						ten.input.fvalues.push_back(vd);
					}
				}
				break;
			case iot_vect:
				int nvalues = fte.input().values_size();
				for (int j=0;j<nvalues;j++) {
					ten.input.values.push_back(fte.input().values(j));
				}
				break;
		}

		switch(ten.output.type) {
			case iot_value:
				ten.output.value = fte.output().value();
				break;
			case iot_file:
				{
					ten.output.extract_regex = fte.output().extract_regex();
					::google::protobuf::RepeatedPtrField< ::fann_train_cfg::FannTrainProject_TrainIo_FValues >::const_iterator it;
					for (it=fte.output().fvalues().begin();it!=fte.output().fvalues().end();it++) {
						vector<double> vd;
						int nvalues = it->values_size();
						for (int j=0;j<nvalues;j++) {
							vd.push_back(it->values(j));
						}
						ten.output.fvalues.push_back(vd);
					}
				}
				break;
			case iot_vect:
				int nvalues = fte.output().values_size();
				for (int j=0;j<nvalues;j++) {
					ten.output.values.push_back(fte.output().values(j));
				}
				break;
		}

		train_entries.push_back(ten);
	}

	project.file_name_present = true;
	return true;
}

int data_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	if (2 == argc) {
		string aux_cmd = argv[1];
		if ("list" == aux_cmd) {
			string l = build_data_list();
			Tcl_SetResult(interp,(char*)l.c_str(),TCL_VOLATILE);
			return TCL_OK;
		}
	}
	if (3 == argc) {
		string aux_cmd = argv[1];
		string aux_opt = argv[2];

		if ("remove" == aux_cmd) { // Удалить запись по ее названию
			if (remove_data_entry(aux_opt)) {
				return TCL_OK;
			} else {
				Tcl_SetResult(interp, (char*)"Запись с таким именем не найдена",TCL_STATIC);
				return TCL_ERROR;
			}
		}

		if ("reload" == aux_cmd) { // Перезагрузить запись, если она загружена из файла
			if (reload_data_entry(aux_opt)) {
				return TCL_OK;
			} else {
				Tcl_SetResult(interp, (char*)"Запись с таким именем не найдена",TCL_STATIC);
				return TCL_ERROR;
			}
		}

		if ("show" == aux_cmd) { // Показать параметры записи
			if (show_data_entry(aux_opt)) {
				return TCL_OK;
			} else {
				Tcl_SetResult(interp, (char*)"Запись с таким именем не найдена",TCL_STATIC);
				return TCL_ERROR;
			}
		}

		if ("dump" == aux_cmd) { // Вывести содержимое записи
			if (dump_data_entry(aux_opt)) {
				return TCL_OK;
			} else {
				Tcl_SetResult(interp, (char*)"Запись с таким именем не найдена",TCL_STATIC);
				return TCL_ERROR;
			}
		}
	}

	if (argc>=2 && "add" == (string)argv[1] && add_entry(argc-2, argv+2)) { // Добавить запись
		//Только опция add может иметь переменное количество параметров. Во всех остальных случаях считаем вызов ошибкой
		return TCL_OK;
	}

	Tcl_SetResult(interp, (char*)"wrong options. Usage:\n"
			"list   - вывести список зарегистрированных наборов данных\n"
			"remove - удалить набор данных по его имени\n"
			"reload - перезагрузить набор данных, связанный с внешним файлом\n"
			"show   - отобразить основную информацию об указанном наборе данных\n"
			"dump   - вывести содержимое указанного набора данных\n"
			"add    - создать новый набор данных\n",
			TCL_STATIC);
	return TCL_ERROR;
}

int train_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	return TCL_OK;
}

int project_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]) {
	vector<string> args;
	for (int i=0;i<argc;i++) {
		args.push_back(argv[i]);
	}

	if (2 == argc) {
		if ("name" == args[1]) {
			Tcl_SetResult(interp, (char*)project.name.c_str(), TCL_VOLATILE);
			return TCL_OK;
		}
		if ("file" == args[1]) {
			Tcl_SetResult(interp, (char*)project.file_name.c_str(), TCL_VOLATILE);
			return TCL_OK;
		}
		if ("save" == args[1]) {
			if (!project.file_name_present) {
				Tcl_SetResult(interp, (char*)"Не указано имя файла для сохранения - использовать saveas",TCL_STATIC);
				return TCL_ERROR;
			}

			save_project();
			return TCL_OK;
		}
		if ("path" == args[1]) {
			Tcl_SetResult(interp, (char*)project_path.c_str(), TCL_VOLATILE);
			return TCL_OK;
		}
	}

	if (3 == argc) {
		if ("name" == args[1]) {
			project.name = args[2];
			project.file_name_present = true;
			project.saved = false;
			return TCL_OK;
		}
		if ("saveas" == args[1]) {
			saveas_project(args[2]);
			return TCL_OK;
		}
		if ("load" == args[1]) {
			return load_project(args[2])?TCL_OK:TCL_ERROR;
		}
		if ("path" == args[1]) {
			project_path = build_file_path(args[2]);
			return TCL_OK;
		}
	}
	Tcl_SetResult(interp, (char*)"wrong options. Usage project file|save|(saveas filename)|(load filename)", TCL_STATIC);
	return TCL_ERROR;
}




fann_options fann_opts;

