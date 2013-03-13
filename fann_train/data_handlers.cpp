/*
 * data_handlers.cpp
 *
 *  Created on: 28.02.2013
 *      Author: drozdov
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include <pcre.h>

#include <string.h>

#include "common.h"
#include "interactive.h"
#include "data_handlers.h"

using namespace std;

std::string build_data_list() {
	string res = "{";
	vector<train_entry>::iterator it;
	for (it=train_entries.begin();it != train_entries.end(); it++) {
		res += it->name + " ";
	}

	return res + "}";
}


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
		case iot_matrix:
			cout << "\tmatrix:" << endl;
			cout << "\trow count: " << it->input.fvalues.size() << endl;
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
		case iot_matrix:
			cout << "\tmatrix:" << endl;
			cout << "\trow count: " << it->output.fvalues.size() << endl;
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
		case iot_matrix:
			cout << "\tmatrix: " << endl;
			for (vector<vector<double> >::iterator it = tio->fvalues.begin();it != tio->fvalues.end();it++) {
				cout << "\t\t";
				for_each(it->begin(),it->end(),cout_numeric<double>);
				cout << endl;
			}
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

#define ST_NONE   0
#define ST_INPUT  1
#define ST_OUTPUT 2
#define ST_NAME   3

bool merge_entries(int argc, const char *argv[]) {
	if (0 == argc) {
		cout << "Usage: data merge -inp state [-inp state ]* -out state [-out state]* -name state_name" << endl;
		return false;
	}
	//Определяем список состояний, которые надо объединить
	vector<string> input_names;  //Названия состояний, для которых необходимо объединить входные обучающие выборки
	vector<string> output_names; //Названия состояний, для которых необходимо объединить выходные обучающие выборки
	string merged_name = "";
	bool remove_merged = false;
	int state = ST_NONE;
	for (int i=0;i<argc;i++) {
		switch(state) {
			case ST_NONE:
				if ("-inp" == (string)argv[i]) {
					state = ST_INPUT;
					continue;
				}
				if ("-out" == (string)argv[i]) {
					state = ST_OUTPUT;
					continue;
				}
				if ("-name" == (string)argv[i]) {
					state = ST_NAME;
					continue;
				}
				if ("-remove-merged" == (string)argv[i]) {
					remove_merged = true;
					continue;
				}
				cout << "merge_entries error - unknown option " << argv[i] << endl;
				return false;
				break;
			case ST_INPUT:
				input_names.push_back((string)argv[i]);
				state = ST_NONE;
				break;
			case ST_OUTPUT:
				output_names.push_back((string)argv[i]);
				state = ST_NONE;
				break;
			case ST_NAME:
				merged_name = (string)argv[i];
				state = ST_NONE;
				break;
		}
	}
	if (ST_NONE != state) {
		cout << "merge_entries error - missing option value" << endl;
		return false;
	}

	train_entry ten;
	ten.input.type = iot_matrix;
	ten.input.fvalues.resize(0);
	ten.output.type = iot_matrix;
	ten.output.fvalues.resize(0);
	for (vector<string>::iterator it=input_names.begin();it!=input_names.end();it++) {
		vector<train_entry>::iterator ten_inp = find_if(train_entries.begin(),train_entries.end(), check_entry_name(*it));
		if (train_entries.end() == ten_inp) {
			cout << "merge_entries error - unknown state " << *it << endl;
			return false;
		}
		switch (ten_inp->input.type) {
			case iot_vect:
				ten.input.fvalues.push_back(ten_inp->input.values);
				break;
			case iot_file:
			case iot_matrix:
				for (vector< vector<double> >::iterator fval_it=ten_inp->input.fvalues.begin();fval_it!=ten_inp->input.fvalues.end();fval_it++) {
					ten.input.fvalues.push_back(*fval_it);
				}
				break;
		}
	}

	for (vector<string>::iterator it=output_names.begin();it!=output_names.end();it++) {
		vector<train_entry>::iterator ten_out = find_if(train_entries.begin(),train_entries.end(), check_entry_name(*it));
		if (train_entries.end() == ten_out) {
			cout << "merge_entries error - unknown state " << *it << endl;
			return false;
		}
		switch (ten_out->output.type) {
			case iot_vect:
				ten.output.fvalues.push_back(ten_out->output.values);
				break;
			case iot_file:
			case iot_matrix:
				for (vector< vector<double> >::iterator fval_it=ten_out->output.fvalues.begin();fval_it!=ten_out->output.fvalues.end();fval_it++) {
					ten.output.fvalues.push_back(*fval_it);
				}
				break;
		}
	}

	if (remove_merged) {
		for (vector<string>::iterator it=input_names.begin();it!=input_names.end();it++) {
			remove_data_entry(*it);
		}
		for (vector<string>::iterator it=output_names.begin();it!=output_names.end();it++) {
			remove_data_entry(*it);
		}
	}
	remove_data_entry(merged_name);
	ten.name = merged_name;

	train_entries.push_back(ten);
	return true;
}

bool add_entry(int argc, const char *argv[]) {
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

	//"merge -inp st1 -inp st2 -inp st3 -out st1 -name st1"
	if (argc>=2 && "merge" == (string)argv[1] && merge_entries(argc-2, argv+2)) {
		return TCL_OK;
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
			"add    - создать новый набор данных\n"
			"merge  - объединить наборы данных\n",
			TCL_STATIC);
	return TCL_ERROR;
}


