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

#include "common.h"

using namespace std;

string executable_path;
string project_path;

map<char,option_handler_desc> option_handlers;

std::string get_executable_path(char* argv0) {
	string full_exe_path = "";
	if ('/' != argv0[0]) {
		string cwd = getcwd(NULL,0);
		string argv_p = argv0 + 1;
		full_exe_path = cwd + argv_p;
	} else {
		full_exe_path = argv0;
	}
	size_t last_slash = full_exe_path.rfind('/');

	return full_exe_path.substr(0,last_slash+1);
}

std::string build_file_path(string fpath) {
	if (fpath.length()>2 && '.'==fpath[0] && '/'==fpath[1]) {
		fpath = fpath.substr(2);
	} else if ('/' == fpath[0]) {
		return fpath;
	}
	string full_path = executable_path + fpath;
	return full_path;
}

std::string build_project_path(string fpath) {
	if (fpath.length()>2 && '.'==fpath[0] && '/'==fpath[1]) {
		fpath = fpath.substr(2);
	} else if ('/' == fpath[0]) {
		return fpath;
	}
	string full_path = project_path + fpath;
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
int parse_options(int argc, char *argv[]) {
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
	int c = 0;
	while (true) {
		c = getopt_long(argc, argv, short_opts, (option*)long_opts, NULL);
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

string build_version_date(unsigned int d, bool str=true, bool hex=false) {
	int year  = (d>>24) & 0xFF;
	int month = (d>>16) & 0xFF;
	int day   = (d>>8) & 0xFF;
	int bld   = d & 0xFF;

	char tmp_str[100] = {0};

	if (str && hex) {
		if (bld > 0) {
			sprintf(tmp_str, "%02X.%02X.20%02X-%02X (0x%08X)", day,month,year,bld,d);
		} else {
			sprintf(tmp_str, "%02X.%02X.20%02X (0x%08X)", day,month,year,d);
		}
		return tmp_str;
	}

	if (str && !hex) {
		if (bld > 0) {
			sprintf(tmp_str, "%02X.%02X.20%02X-%02X", day,month,year,bld);
		} else {
			sprintf(tmp_str, "%02X.%02X.20%02X", day,month,year);
		}
		return tmp_str;
	}

	if (!str && hex) {
		sprintf(tmp_str, "0x%08X",d);
		return tmp_str;
	}
	return "";
}

string build_device_type(unsigned int t, bool str=true, bool hex=false) {
	const char *dev_generations[] = {"0","1","2","3","4","5","6", "7", "8"};
	const char *dev_projects[] = {"М","Д"};
	const char *dev_hierarchy[]   = {"М", "Я", "Б"};

	string dev_type_str = "";
	int dev_gen = (t>>24) & 0x0F;
	if (dev_gen>8 || dev_gen<0) {
		return "Неверный формат";
	}
	int dev_project = (t>>28) & 0x0F;
	if (dev_project>1 || dev_project<0) {
		return "Неверный формат";
	}
	int dev_h = (t>>20) & 0x0F;
	if (dev_h>2 || dev_h<0) {
		return "Неверный формат";
	}
	int dev_number = (t>>8) & 0xFFF;
	int dev_subnumber = t&0xFF;

	char tmp_str[100] = {0};
	sprintf(tmp_str,"%s%s%s-%03i",
			dev_generations[dev_gen],
			dev_projects[dev_project],
			dev_hierarchy[dev_h],
			dev_number);

	if (dev_subnumber > 0) {
		strcat(tmp_str,"-");
		char tmp_str2[10] = {0};
		sprintf(tmp_str2, "%02i",dev_subnumber);
		strcat(tmp_str,tmp_str2);
	}

	dev_type_str = tmp_str;

	sprintf(tmp_str,"0x%08X",t);

	string dev_type_num = tmp_str;

	if (str && hex) {
		return dev_type_str + " (" + dev_type_num + ")";
	}
	if (str && !hex) {
		return dev_type_str;
	}
	if (!str && hex) {
		return dev_type_num;
	}

	return "";
}




