/*
 * net_link.cpp
 *
 *  Created on: 26.06.2011
 *      Author: drozdov
 */


#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <unistd.h>
#include <fcntl.h>

#include <pthread.h>


#include "net_link.h"
#include "mental_fsm.h"

using namespace std;

CNetLink::CNetLink(string filename) {
	online = false;
	listen_active = false;
	max_connections = 1;

	buf_proc_state = 0;
	cur_buf = (unsigned char*)malloc(MAX_RCV_BUF);
	pcur_buf = cur_buf;
	cur_buf_usage = 0;

	dump_instream = false;
}

int CNetLink::OpenConection() {
	listener = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(listener, F_SETFL, 0);
	if(listener < 0) {
		cout << "CNetLink::OpenConection error - failed to create socket" << endl;
		return -1;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(5000);//port_number);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		cout << "CNetLink::OpenConection error - failed to bind socket" << endl;
		return -1;
	}

	cout << "Network online" << endl;
	online = true;
	return 0;
}

int CNetLink::BeginListen() {
	if (!online) {
		cout << "CNetLink::BeginListen error - netlink is offline" << endl;
		return 1;
	}
	if (listen_active) {
		cout << "Already listens" << endl;
		return 0;
	}

	if (listen(listener,max_connections)) {
		cout << "CNetLink::BeginListen error - listen failed" << endl;
		return 1;
	}
	cout << "Listening socket" << endl;
	listen_active = true;
	return 0;
}

int CNetLink::Accept() {
	pthread_t thread_id;
	pthread_create (&thread_id, NULL, &netlink_accept_thread, this);
	return 0;
}

int CNetLink::accept_thread() {
	while (true) {
		sock = accept(listener, NULL, NULL);
		if(sock>=0) {
			struct sockaddr remote_addr;
			unsigned int len = sizeof(sockaddr);
			if (0 == getpeername(sock,&remote_addr, &len)) {
				char ipstr[INET6_ADDRSTRLEN];
				int port;
				struct sockaddr_in *s = (struct sockaddr_in *)&remote_addr;
				port = ntohs(s->sin_port);
				inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));

				cout << "CNetLink::Accept info - accepted connection from " << ipstr << ":" << port << endl;
			} else {
				cout << "CNetLink::Accept info - accepted connection " << endl;
				cout << "CNetLink::Accept warning - unknown remote peer" << endl;
			}

		} else if (EINVAL == errno || EBADF==errno) {
			cout << "CNetLink::accept_thread info - server stopped" << endl;
			pthread_exit(0);
			return 0;
		} else {
			cout << errno << endl;
			cout << "CNetLink::accept_thread warning - accept was interupted without connection" << endl;
			continue;
		}

		if (0 == fork()) {
			//Дочерний процесс.

			//Первым делом закрываем слушающий сокет
			close(listener);
			online = false;
			listen_active = false;
			cout << "CNetLink::accept_thread info - listening socket closed" << endl;
			close(STDIN_FILENO);

			handle_network();
		} else {
			//Родительский. Закрываем клиентский сокет
			close(sock);
		}
	}

	return 0;
}

int CNetLink::handle_network() {
	unsigned char buf[1024];

	while(true) {
		int bytes_read = read(sock, buf, 1024);
		if(bytes_read <= 0) {
			cout << errno << endl;
			cout << "CNetLink::accept_thread info - child disconnected with code "<< errno << endl;
			break;
		}
		process_buffer(buf,bytes_read);
	}

	close(sock);
	cout << "Will now exit" << endl;
	exit(0);

	return 0;
}

int CNetLink::process_buffer(unsigned char* buf, int len) {
	unsigned char* pbuf = buf;

	if (dump_instream) {
		cout << "CNetLink::process_buffer: " << len << endl;
		cout << "Initial state is " << buf_proc_state << endl;

		char ch[6];
		for (int i=0;i<len;i++) {
			sprintf(ch,"0x%X",buf[i]);
			cout << ch << " ";
		}
		cout << endl;
	}

	int i = 0;
	while (i<len) {
		switch (buf_proc_state) {
			case bps_unknown:
				//cout << "bps_unknown" << endl;
				if (FRAME_START == *pbuf) {
					//Из неизвестного состояния перешли в начало фрейма
					buf_proc_state = bps_start;
				} else {
					pbuf++;
					i++;
				}
				break;

			case bps_start:
				//cout << "bps_start" << endl;
				//Безусловно переходим к обработке тела сообщения
				buf_proc_state = bps_body;
				pbuf++;
				i++;

				pcur_buf = cur_buf;
				cur_buf_usage = 0;
				break;

			case bps_body:
				//cout << "bps_body" << endl;
				if (FRAME_ESCAPE == *pbuf) {
					pbuf++;
					i++;
					buf_proc_state = bps_escape;
					break;
				} else if (FRAME_START == *pbuf) {
					cout << "CNetLink::process_buffer atacked - frame start in message body" << endl;

					cout << "Position " << i << endl;

					cout << "Will now exit" << endl;
					exit(4);
				} else if (FRAME_END == *pbuf) {
					//Прием буфера закончился. Обрабатываем принятый буфер
					buf_proc_state = bps_unknown;

					if (len < 8) {
						//Фрейм имеет правильную струткуру, но содержит менее 8 байт
						//В нем гарантированно отсутствует тип сообщения и длина
						//Проверить правильность данных в этом буфере невозможно
						cout << "CNetLink::process_buffer atacked - frame is too short to be valid" << endl;
						exit(4);
					}
					process_message(cur_buf,cur_buf_usage);
					pbuf++;
					i++;
					break;
				}
				//Обычный символ. Записываем его в буфер декодированного сообщения
				*pcur_buf = *pbuf;
				pcur_buf++;
				cur_buf_usage++;
				i++;
				pbuf++;
				if (MAX_RCV_BUF <= cur_buf_usage) {
					cout << "CNetLink::process_buffer atacked - frame length overflow" << endl;
					cout << "Will now exit" << endl;
					exit(4);
				}
				break;
			case bps_escape:
				//cout << "bps_escape" << endl;
				{
					unsigned char c = 0;
					if (0xCB == *pbuf) {
						c = FRAME_START;
					} else if (0xCE == *pbuf) {
						c = FRAME_END;
					} else if (FRAME_ESCAPE == *pbuf) {
						c = FRAME_ESCAPE;
					} else {
						cout << "CNetLink::process_buffer atacked - unknown escape symbol" << endl;
						cout << "Will now exit" << endl;
						exit(4);
					}
					*pcur_buf = c;
					pcur_buf++;
					cur_buf_usage++;
					i++;
					pbuf++;
					buf_proc_state = bps_body;
					if (MAX_RCV_BUF <= cur_buf_usage) {
						cout << "CNetLink::process_buffer atacked - frame length overflow" << endl;
						cout << "Will now exit" << endl;
						exit(4);
					}
				}
				break;
		}
	}
	return 0;
}

int CNetLink::process_message(unsigned char* buf, int len) {
	int msg_type = *(int*)buf;
	int msg_len  = *(int*)(buf+4);

	//Проверяем длину сообщения
	if (msg_len != len-2) {
		cout << "CNetLink::process_message atacked - wrong message length specified" << endl;
		cout << "Will now exit" << endl;
		exit(4);
	}
	//Длина сообщения, заданная в самом сообщении совпадает с полученной длиной фрейма

	//Разбираем сообщение по его типу
	switch (msg_type) {
		case nlmt_link:
			return process_link_msg(buf, len);
			break;
		case nlmt_security:
			return process_sec_msg(buf, len);
			break;
		case nlmt_time:
			return process_time_msg(buf, len);
			break;
		case nlmt_trig:
			return process_trig_msg(buf, len);
			break;
		case nlmt_tcl:
			break;
		default:
			cout << "CNetLink::process_message atacked - unknown message type" << endl;
			cout << "Will now exit" << endl;
			exit(4);
	}
	return 0;
}

int CNetLink::process_trig_msg(unsigned char* buf, int len) {
	if (len < 12) {
		//Фрейм имеет правильную струткуру, но содержит менее 12 байт
		//В количество триггеров
		//Проверить правильность данных в этом буфере невозможно
		cout << "CNetLink::process_trig_msg atacked - frame is too short to be valid" << endl;
		exit(4);
	}
	int n_triggers = *(int*)(buf+8);

	//Просматриваем триггера, описываемые этим сообщением
	unsigned char* trig_start = buf + 12;
	int offs = 12;
	for (int trig_cnt=0;trig_cnt<n_triggers;trig_cnt++) {
		//cout << "len " << len << endl;
		//cout << trig_cnt << " " << (offs+8) << endl;
		if ((offs+8)>len) {
			cout << "CNetLink::process_trig_msg atacked - trigger data is too short" << endl;
			exit(4);
		}
		int trig_id = *(int*)trig_start;
		trig_start += 4;
		offs += 4;
		int out_count = *(int*)trig_start;
		trig_start += 4;
		offs += 4;

		//Проверяем, что триггер с таким id зарегистрирован
		if (0>trig_id || (unsigned)trig_id >= virtuals.size() || NULL == virtuals[trig_id]) {
			cout << "CNetLink::process_trig_msg atacked - trigger id is unregistered" << endl;
			exit(4);
		}

		if (out_count <= 0) {
			//Триггер не содержит выходов. Переходим к следующим триггерам
			continue;
		}

		CVirtTrigger* vt = virtuals[trig_id];

		//Проверяем, что все выходы могут уместиться в остатке сообщеия
		if ((offs+out_count*(int)sizeof(trig_msg_item)) > len) {
			cout << "CNetLink::process_trig_msg atacked - trigger outs section is too short" << endl;
			exit(4);
		}
		trig_msg_item *ptmi = (trig_msg_item*)trig_start;
		for (int out_cnt=0;out_cnt<out_count;out_cnt++) {
			vt->SetOutput(ptmi->out_id,ptmi->value);
			ptmi++;
		}

		trig_start += out_count*sizeof(trig_msg_item);
		offs += out_count*sizeof(trig_msg_item);
	}

	return 0;
}

int CNetLink::process_time_msg(unsigned char* buf, int len) {
	int req_size = 4+4+sizeof(long long);
	if (len < req_size) {
		cout << "CNetLink::process_time_msg atacked - frame is too short to be valid" << endl;
		exit(4);
	}
	long long *ptime = (long long*)(buf + 8);
	long long new_time = *ptime;
	if (new_time < fsm->GetCurrentTime()) {
		cout << "CNetLink::process_time_msg atacked - time " << new_time << " is less than current time " << fsm->GetCurrentTime() << endl;
		exit(4);
	}

	fsm->RunToTime(new_time);
	//cout << "time is" << new_time << endl;
	return 0;
}

int CNetLink::process_sec_msg(unsigned char* buf, int len) {
	return 0;
}

int CNetLink::process_link_msg(unsigned char* buf, int len) {
	return 0;
}

int CNetLink::CloseConnection() {
	if (online) {
		close(listener);
		cout << "Network offline" << endl;
	}
	online = false;
	listen_active = false;
	return 0;
}

int CNetLink::RegisterVirtualTrigger(CVirtTrigger* trigger) {
	if (NULL == trigger) {
		cout << "CNetLink::RegisterVirtualTrigger error - null pointer to trigger" << endl;
		return 1;
	}

	unsigned int prev_size = virtuals.size();
	if (trigger->trigger_id >= (int)virtuals.size()) {
		for (unsigned int i=prev_size;i<virtuals.size();i++) {
			virtuals[i] = NULL; //Помечаем все указатели нулями, чобы потом знать что они не использовались
 		}
		virtuals.resize(trigger->trigger_id + 1);
	} else {
		//Триггер попадает в диапазон зарегистрированных триггеров
		//Проверяем, что он не был зарегистрирован раньше
		if (NULL != virtuals[trigger->trigger_id]) {
			cout << "CNetLink::RegisterVirtualTrigger warning - trigger " << trigger->szTriggerName << " was already registered" << endl;
			return 0;
		}
	}

	//Проверяем, что триггера с таким именем не было зарегистрировано раньше
	if (by_name.find(trigger->szTriggerName) != by_name.end()) {
		cout << "CNetLink::RegisterVirtualTrigger warning - trigger " << trigger->szTriggerName << " was already registered" << endl;
		return 0;
	}

	virtuals[trigger->trigger_id] = trigger;
	by_name[trigger->szTriggerName] = trigger;

	return 0;
}

int CNetLink::TclHandler(Tcl_Interp* interp, int argc, CONST char *argv[]) {
	if (1==argc) {
		if (online)
			cout << "online" << endl;
		else
			cout << "offline" << endl;
		return TCL_OK;
	}
	if (2==argc) {
		string cmd_op = argv[1];
		if ("open" == cmd_op) {
			if (online) {
				cout << "Already opened" << endl;
				return TCL_OK;
			}

			//Необходимо установить соединение
			if (0 != OpenConection()) {
				return TCL_ERROR;
			}
			cout << "OK" << endl;
			return TCL_OK;
		}
		if ("close" == cmd_op) {
			if (!online) {
				cout << "Already closed" << endl;
				return TCL_OK;
			}

			//Необходимо установить соединение
			if (0 != CloseConnection()) {
				return TCL_ERROR;
			}
			cout << "OK" << endl;
			return TCL_OK;
		}
		if ("listen" == cmd_op) {
			//Необходимо начать прослушивание порта
			if (0 != BeginListen()) {
				return TCL_ERROR;
			}
			cout << "OK" << endl;
			return TCL_OK;
		}
		if ("accept" == cmd_op) {
			//Необходимо начать прослушивание порта
			if (0 != Accept()) {
				return TCL_ERROR;
			}
			return TCL_OK;
		}
	}
	if (3==argc) {
		string cmd_op = argv[1];
		string cmd_pr = argv[2];
		if ("dump_instream" == cmd_op) {
			if ("on" == cmd_pr) {
				dump_instream = true;
				cout << "OK" << endl;
				return TCL_OK;
			} else if ("off" == cmd_pr) {
				dump_instream = false;
				cout << "OK" << endl;
				return TCL_OK;
			}
		}
	}
	cout << "Unknown command operands" << endl;
	return TCL_ERROR;
}


CNetLink *net_link = NULL;

int create_net_link(std::string filename) {
	net_link = new CNetLink(filename);
	return 0;
}


void* netlink_accept_thread (void* thread_arg) {
	CNetLink* nl = (CNetLink*) thread_arg;
	nl->accept_thread();

	return NULL;
}


