/*
 * opt_handlers.cpp
 *
 *  Created on: 21.09.2011
 *      Author: drozdov
 */

#include <stdlib.h>

#include "common.h"
#include "opt_handlers.h"

using namespace std;

//Функция регистрирует обработчики опций и подготавливает программу к их обработке
void fill_option_handlers() {
	option_handler_desc ohd;

	ohd.option = '@'; ohd.long_option = "help";
	ohd.has_arg = false; ohd.arg_strict = false; ohd.hdlr = opt_help_handler;
	add_option_handler(ohd);

	ohd.option = 's'; ohd.long_option = "script";
	ohd.has_arg = true; ohd.arg_strict = true; ohd.hdlr = opt_script_handler;
	add_option_handler(ohd);

	ohd.option = 'i'; ohd.long_option = "inputs_count";
	ohd.has_arg = true; ohd.arg_strict = true; ohd.hdlr = opt_inputs_count_handler;
	add_option_handler(ohd);

	ohd.option = 'o'; ohd.long_option = "outputs_count";
	ohd.has_arg = true; ohd.arg_strict = true; ohd.hdlr = opt_outputs_count_handler;
	add_option_handler(ohd);

	ohd.option = 'h'; ohd.long_option = "hidden-neurons";
	ohd.has_arg = true; ohd.arg_strict = true; ohd.hdlr = opt_hidden_neurons_handler;
	add_option_handler(ohd);

	ohd.option = 'l'; ohd.long_option = "hidden-layers";
	ohd.has_arg = true; ohd.arg_strict = true; ohd.hdlr = opt_hidden_layers_handler;
	add_option_handler(ohd);

	ohd.option = 't'; ohd.long_option = "train-file";
	ohd.has_arg = true; ohd.arg_strict = true; ohd.hdlr = opt_train_file_handler;
	add_option_handler(ohd);

	ohd.option = 'n'; ohd.long_option = "net-file";
	ohd.has_arg = true; ohd.arg_strict = true; ohd.hdlr = opt_net_file_handler;
	add_option_handler(ohd);

	ohd.option = 'e'; ohd.long_option = "desired-error";
	ohd.has_arg = true; ohd.arg_strict = true; ohd.hdlr = opt_desired_error_handler;
	add_option_handler(ohd);

	ohd.option = 'm'; ohd.long_option = "max-epochs";
	ohd.has_arg = true; ohd.arg_strict = true; ohd.hdlr = opt_max_epochs_handler;
	add_option_handler(ohd);
}


//Функция-обработчик справки
int opt_help_handler(char opt_name, char* popt_val) {
	cout << "Использование" << endl;
	cout << "Поддерживаемые режимы:" << endl;
	cout << "\t-интерактивный. Включается при запуске без параметров" << endl;
	cout << "\t-скрипт. Включается при запуске с параметром -s" << endl;
	cout << "\t-пакетный. Включается при передаче хотя бы одного параметра пакетного режима." << endl;
	cout << endl;
	cout << "Режим скрипта" << endl;
	cout << "\t-s --script file - имя файла со скриптом. Поддерживаются Tcl и Python скрипты" << endl;
	cout << endl;
	cout << "Пакетный режим" << endl;
	cout << "Обязательные опции" << endl;
	cout << "\t-i --inputs-count   value - количество входов" << endl;
	cout << "\t-o --outputs-count  value - количество выходов" << endl;
	cout << "\t-h --hidden-neurons value - количество нейронов в скрытых слоях" << endl;
	cout << "\t-l --hidden-layers  value - количество скрытых слоев" << endl;
	cout << "\t-t --train-file     file -  имя файла с данными для обучения" << endl;
	cout << "\t-n --net-file       file -  имя выходного файла с сетью" << endl;

	cout << "Дополнительные опции" << endl;
	cout << "\t-e --desired-error value - максимально допустимая ошибка" << endl;
	cout << "\t-m --max-epochs    value - максимальное количество эпох обучения" << endl;
	cout << endl;

	return OHRC_CONTINUE;
}


int opt_script_handler(char opt_name, char* popt_val) {
	run_options.script_file_name = popt_val;
	run_options.mode_script = true;
	return OHRC_CONTINUE;
}

int opt_inputs_count_handler(char opt_name, char* popt_val) {
	run_options.num_input = strtol(popt_val,NULL,0);
	run_options.b_i = true;
	run_options.mode_packet = true;
	return OHRC_CONTINUE;
}

int opt_outputs_count_handler(char opt_name, char* popt_val) {
	run_options.num_output = strtol(popt_val,NULL,0);
	run_options.b_o = true;
	run_options.mode_packet = true;
	return OHRC_CONTINUE;
}

int opt_hidden_neurons_handler(char opt_name, char* popt_val) {
	run_options.num_neurons_hidden = strtol(popt_val,NULL,0);
	run_options.b_h = true;
	run_options.mode_packet = true;
	return OHRC_CONTINUE;
}

int opt_hidden_layers_handler(char opt_name, char* popt_val) {
	run_options.num_layers = strtol(popt_val,NULL,0);
	run_options.b_l = true;
	run_options.mode_packet = true;
	return OHRC_CONTINUE;
}

int opt_train_file_handler(char opt_name, char* popt_val) {
	run_options.train_file_name = popt_val;
	run_options.b_t = true;
	run_options.mode_packet = true;
	return OHRC_CONTINUE;
}

int opt_net_file_handler(char opt_name, char* popt_val) {
	run_options.net_file_name = popt_val;
	run_options.b_n = true;
	run_options.mode_packet = true;
	return OHRC_CONTINUE;
}

int opt_desired_error_handler(char opt_name, char* popt_val) {
	run_options.desired_error = strtod(popt_val,NULL);
	run_options.mode_packet = true;
	return OHRC_CONTINUE;
}

int opt_max_epochs_handler(char opt_name, char* popt_val) {
	run_options.max_epochs = strtol(popt_val,NULL,0);
	run_options.mode_packet = true;
	return OHRC_CONTINUE;
}


