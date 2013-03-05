/*
 * interactive.h
 *
 *  Created on: 13.10.2011
 *      Author: drozdov
 */

#ifndef INTERACTIVE_H_
#define INTERACTIVE_H_

#include <string>
#include "train_tools.h"
#include "fann.h"

extern void run_interactive(int argc, char *argv[]);
extern void run_script(int argc, char *argv[]);


typedef struct _fann_options {
	int inputs;
	int outputs;
	int layers;
	int hidden_neurons;

	double desired_error;
	int max_epochs;
	int log_period;

	std::string def_in_regexp;
	std::string def_out_regexp;

	std::string file_name;
	bool trained;
} fann_options;


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


extern std::vector<train_row> train_rows;
extern std::vector<train_entry> train_entries;
extern fann_options fann_opts;


#endif /* INTERACTIVE_H_ */
