/*
 * test_rcv.cpp
 *
 *  Created on: 02.08.2012
 *      Author: drozdov
 */

#include <sys/poll.h>

#include <iostream>
#include <string>
#include <list>
#include <map>

#include "common.h"
#include "p2vera.h"
#include "p2message.h"
#include "p2stream.h"


using namespace std;

// Структура, содержащая параметры запуска, переданные пользователем программе
typedef struct _cmd_options {
	std::list<std::string> streams;
	int server_port;
	bool show_version;
} cmd_options;

cmd_options run_options;


int opt_help_handler(char opt_name, char* popt_val);
int opt_port_handler(char opt_name, char* popt_val);
int opt_version_handler(char opt_name, char* popt_val);
int opt_stream_handler(char opt_name, char* popt_val);


//Функция регистрирует обработчики опций и подготавливает программу к их обработке
void fill_option_handlers() {
	run_options.server_port = 7300;
	run_options.show_version = false;

	option_handler_desc ohd;

	ohd.option = 'h'; ohd.long_option = "help";
	ohd.has_arg = false; ohd.arg_strict = false; ohd.hdlr = opt_help_handler;
	add_option_handler(ohd);

	ohd.option = 's'; ohd.long_option = "stream";
	ohd.has_arg = true; ohd.arg_strict = true; ohd.hdlr = opt_stream_handler;
	add_option_handler(ohd);

	ohd.option = 'p'; ohd.long_option = "port";
	ohd.has_arg = true; ohd.arg_strict = true; ohd.hdlr = opt_port_handler;
	add_option_handler(ohd);

	ohd.option = 'v'; ohd.long_option = "version";
	ohd.has_arg = false; ohd.arg_strict = false; ohd.hdlr = opt_version_handler;
	add_option_handler(ohd);
}

//Функция-обработчик справки
int opt_help_handler(char opt_name, char* popt_val) {
	cout << "Usage" << endl;
	cout << "\t-v --version      - вывести информацию о версиях и завершить работу" << endl;
	cout << "\t-s --stream       - добавление поток для приема сообщений" << endl;
	cout << "\t-p --port [=7300] - указать номер основного порта сети" << endl;
	cout << endl;

	return OHRC_CONINUE;
}

//Функция-обработчика номера РЭКа
int opt_port_handler(char opt_name, char* popt_val) {
	return OHRC_CONINUE;
}

int opt_version_handler(char opt_name, char* popt_val) {
	run_options.show_version = true;
	return OHRC_CONINUE;
}

int opt_stream_handler(char opt_name, char* popt_val) {
	run_options.streams.push_back(popt_val);
	return OHRC_CONINUE;
}


int main(int argc, const char *argv[]) {
	fill_option_handlers();
	parse_options(argc, argv);
	if (run_options.show_version) {
		cout << "P2Vera receive test - version 0.2" << endl;
		return 0;
	}

	P2Vera* p2v = new P2Vera();

	map<int, P2VeraStream> stream_fd;

	int fd_count = run_options.streams.size();
	pollfd *pfd = new pollfd[fd_count];
	int fd_cnt = 0;
	for (list<string>::iterator it = run_options.streams.begin();it!=run_options.streams.end();it++) {
		stream_config stream_cfg;
		stream_cfg.name = *it;
		stream_cfg.direction = stream_direction_income;
		stream_cfg.order = stream_msg_order_any;
		stream_cfg.type = stream_type_dgram;
		p2v->register_stream(stream_cfg);

		P2VeraStream p2s = p2v->create_instream(*it);
		int fd = p2s.get_fd();
		stream_fd[fd] = p2s;
		pfd[fd_cnt].fd = fd;
		pfd[fd_cnt].events = POLLIN | POLLHUP;
		pfd[fd_cnt].revents = 0;
	}
	p2v->start_network();

	while(true) {
		if (poll(pfd, fd_count, 100) < 1) {
			continue;
		}
		for (int i=0;i<fd_count;i++) {
			if (0==pfd[i].revents) continue;
			pfd[fd_cnt].revents = 0;
			P2VeraTextMessage p2tm;
			stream_fd[pfd[i].fd] >> p2tm;
			p2tm.print();
		}
	}

	return 0;
}



