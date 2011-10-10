/*
 * opt_handlers.cpp
 *
 *  Created on: 21.09.2011
 *      Author: drozdov
 */

#include "common.h"
#include "opt_handlers.h"

using namespace std;

//Функция регистрирует обработчики опций и подготавливает программу к их обработке
void fill_option_handlers() {
	option_handler_desc ohd;

	ohd.option = 'h'; ohd.long_option = "help";
	ohd.has_arg = false; ohd.arg_strict = false; ohd.hdlr = opt_help_handler;
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

	return OHRC_CONINUE;
}


