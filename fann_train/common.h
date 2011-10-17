/*
 * common.h
 *
 *  Created on: 19.09.2011
 *      Author: drozdov
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <iostream>
#include <string>
#include <vector>

typedef struct _md5_checksum {
	unsigned int p1;
	unsigned int p2;
	unsigned int p3;
	unsigned int p4;

	bool ok;
} md5_checksum;

// Структура, содержащая параметры запуска, переданные пользователем программе
typedef struct _cmd_options {
	unsigned int num_input;
	unsigned int num_output;
	unsigned int num_layers;
	unsigned int num_neurons_hidden;
	double desired_error;
	unsigned int max_epochs;
	unsigned int epochs_between_reports;

	std::string train_file_name;
	std::string net_file_name;
	std::string script_file_name;

	bool b_i;
	bool b_o;
	bool b_h;
	bool b_l;
	bool b_t;
	bool b_n;

	bool mode_interactive; //Интерактивный режим работы
	bool mode_script;      //Отработка скрипта
	bool mode_packet;      //Пакетный режим
} cmd_options;

extern cmd_options run_options;

typedef struct _long_opt {
	const char *name;
	int         has_arg;
	int        *flag;
	int         val;
} long_opt_desc;

//Прототип функции-обработчика опции командной строки.
//Вызывается при обнаружении соответсвующей опции
typedef int (*option_handler_fcn)(char opt_name, char* popt_val);
#define OHRC_CONTINUE 0 //Параметр обработан правильно. Продолжить работу
#define OHRC_EXIT    1 //Параметр обработан правильно. Завершить работу
#define OHRC_ERROR   2 //Неправильный параметр. Завершить работу

typedef struct _option_handler_desc {
	char option;
	std::string long_option;
	bool has_arg;
	bool arg_strict;
	option_handler_fcn hdlr;
} option_handler_desc;

extern std::string get_executable_path(char* argv0);
extern std::string build_file_path(std::string fpath);

extern int parse_options(int argc, char *argv[]);
extern void add_option_handler(option_handler_desc ohd);

extern std::string build_version_date(unsigned int d, bool str, bool hex);
extern std::string build_device_type(unsigned int t, bool str, bool hex);

extern std::string executable_path;

#endif /* COMMON_H_ */
