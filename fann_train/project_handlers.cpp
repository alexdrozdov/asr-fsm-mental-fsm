/*
 * project_handlers.cpp
 *
 *  Created on: 28.02.2013
 *      Author: drozdov
 */

#include <iostream>
#include <vector>
#include <string>

#include "cfg_serialize.pb.h"
#include "interactive.h"
#include "project_handlers.h"

using namespace std;
using namespace fann_train_cfg;

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





