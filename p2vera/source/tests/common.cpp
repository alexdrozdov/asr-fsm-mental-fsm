/*
 * common.cpp
 *
 *  Created on: 19.09.2011
 *      Author: drozdov
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

#include <string>
#include <iostream>
#include <map>
#include <sstream>

#include "common.h"

using namespace std;

string executable_path;

map<char,option_handler_desc> option_handlers;

std::string get_executable_path(const char* argv0) {
	string cwd = getcwd(NULL,0);
	string argv_p = argv0 + 1;
	string full_exe_path = cwd + argv_p;
	size_t last_slash = full_exe_path.rfind('/');

	return full_exe_path.substr(0,last_slash+1);
}

std::string build_file_path(string fpath) {
	if ('.' == fpath[0]) {
		fpath = fpath.substr(1);
	}
	if ('/' == fpath[0]) {
		fpath = fpath.substr(1);
	}

	string full_path = executable_path + fpath;

	return full_path;
}

//Добавляет обработчик опции по ее описанию
void add_option_handler(option_handler_desc ohd) {
	if (NULL == ohd.hdlr) {
		cout << "Сбой в программе - нулевой указатель на обработчик опции " << ohd.long_option << endl;
		return;
	}
	option_handlers[ohd.option] = ohd;
}

// Функция обработчик параметров командной строки
int parse_options(int argc, const char *argv[]) {
	//Массив длинных опций для getopt_long
	long_opt_desc *long_opts = new long_opt_desc[option_handlers.size()+1];
	//Массив коротких опций для getopt_long
	char *short_opts = new char[option_handlers.size()*2+1];
	//Оба массива имеют максимально возможную длину. Заполится только часть из каждого масива

	int short_opt_cnt = 0;
	int long_opt_cnt = 0;
	map<char,option_handler_desc>::iterator it;
	for (it=option_handlers.begin();it != option_handlers.end();it++) {
		option_handler_desc ohd = it->second;

		if (!ohd.has_arg) {
			short_opts[short_opt_cnt++] = ohd.option;
		} else {
			short_opts[short_opt_cnt++] = ohd.option;
			short_opts[short_opt_cnt++] = ':';
		}

		long_opts[long_opt_cnt].val = ohd.option;
		long_opts[long_opt_cnt].name = ohd.long_option.c_str();
		long_opts[long_opt_cnt].has_arg = (int)ohd.has_arg+(int)ohd.arg_strict;
		long_opts[long_opt_cnt].flag = 0;

		long_opt_cnt++;
	}

	short_opts[short_opt_cnt] = 0;
	long_opts[long_opt_cnt].flag = 0;
	long_opts[long_opt_cnt].has_arg = 0;
	long_opts[long_opt_cnt].name = NULL;
	long_opts[long_opt_cnt].val = 0;

	//Просматриваем все опции, переданные программе

	opterr = 0;
	int c = 0;
	while (true) {
		c = getopt_long(argc, (char**)argv, short_opts, (option*)long_opts, NULL);
		if (-1 == c) {
			break;
		}

		if ('?' == (char)c) {
			continue;
		}

		it = option_handlers.find((char)c);
		if (option_handlers.end() == it) {
			cout << "Сбой в программе - не обработана опция " << (char)c << endl;
			continue;
		}

		//Запускаем обработчик этой опции
		it->second.hdlr(c,optarg);
	}

	return 0;
}


