/*
 * fann_handlers.cpp
 *
 *  Created on: 28.02.2013
 *      Author: drozdov
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

//#include <pcre.h>

#include "interactive.h"
#include "cfg_serialize.pb.h"
#include "fann_handlers.h"
#include "common.h"
//#include "fann.h"

using namespace std;
using namespace fann_train_cfg;

std::vector<train_row> train_rows;
std::vector<train_entry> train_entries;

fann *ann = NULL;

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
		if ("test" == aux_cmd) {
			string sample_name;
			aux_param >> sample_name;
			test_train_data(sample_name);
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
