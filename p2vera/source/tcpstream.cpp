/*
 * tcpstream.cpp
 *
 *  Created on: 06.08.2012
 *      Author: drozdov
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>

#include <list>
#include <map>
#include <iostream>

#include "p2stream.h"
#include "p2message.h"
#include "tcpstream.h"

#include "msg_tcpstream.pb.h"

using namespace std;
using namespace tcpstream;

enum EBufProcessState {
	bps_unknown = 0,
	bps_start   = 1,
	bps_body    = 2,
	bps_escape  = 3,
	bps_end     = 4
};


#define FRAME_START  0xFB
#define FRAME_END    0xFE
#define FRAME_ESCAPE 0xFC

#define MAX_RCV_BUF 1024
#define MAX_TCP_CONNECTIONS 100


TcpStream::TcpStream(RemoteSrvUnit& rsu, int remote_port) {
	cout << "TcpStream::TcpStream info - launched as master for remote server" << endl;
	this->rsu = rsu;
	this->remote_port = remote_port;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(fd, F_SETFL, 0);
	if(fd < 0) {
		cout << "TcpStream::TcpStream error - failed to create socket" << endl;
		return;
	}

	sockaddr_in addr;
	memcpy(&addr, &rsu.get_remote_sockaddr(), sizeof(sockaddr_in));
	addr.sin_port = htons(remote_port);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(sockaddr_in)) < 0) {
		cout << "TcpStream::TcpStream error - couldn`t connect to remote host " << rsu.get_uniq_id() << endl;
		cout << strerror(errno) << endl;
		return;
	}

	buf_proc_state = 0;
	cur_buf = new unsigned char[MAX_RCV_BUF];
	pcur_buf = cur_buf;
	cur_buf_usage = 0;
	cout << "TcpStream::TcpStream info - completed as master, fd is " << fd << endl;
}

TcpStream::TcpStream(int socket) {
	cout << "TcpStream::TcpStream info - launched as slave for remote server" << endl;
	fd = socket;
	remote_port = 0;

	buf_proc_state = 0;
	cur_buf = new unsigned char[MAX_RCV_BUF];
	pcur_buf = cur_buf;
	cur_buf_usage = 0;
}

TcpStream::~TcpStream() {
	delete[] cur_buf;
}

void TcpStream::add_hub(IP2VeraStreamHub* p2h) {
	if (NULL == p2h) {
		cout << "TcpStream::add_hub error - zero pointer to hub" << endl;
		return;
	}
	last_flow_id++;
	map<IP2VeraStreamHub*, int>::iterator p2h_it = hub_to_flow.find(p2h);
	if (p2h_it != hub_to_flow.end()) {
		cout << "TcpStream::add_hub warning - hub was already registered" << endl;
		return;
	}
	flow_to_hub[last_flow_id] = p2h;
	hub_to_flow[p2h] = last_flow_id;
}



void TcpStream::send_message(IP2VeraMessage& p2m, IP2VeraStreamHub* p2h) {
}

void TcpStream::receive_message() {
	int ret = 0;
	unsigned char buf[1024];
	while( (ret = read(fd, buf, 1024))>0 ) {
		process_buffer(buf,ret);
	}
}


int TcpStream::process_buffer(unsigned char* buf, int len) {
	unsigned char* pbuf = buf;

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

					if (cur_buf_usage < 1) {
						//Фрейм имеет правильную струткуру, но содержит менее 8 байт
						//В нем гарантированно отсутствует контрольная сумма
						//Проверить правильность данных в этом буфере невозможно
						cout << "CNetLink::process_buffer atacked - frame is too short to be valid" << endl;
						exit(4);
					}
					process_message(cur_buf,cur_buf_usage-2); //- CRC-16 length
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

int TcpStream::process_message(unsigned char* buf, int len) {
	tcp_wrapper wrpr;
	if (!wrpr.ParseFromArray(buf, len)) {
		cout << "TcpStream::process_message atacked - wrong message format" << endl;
		return 1;
	}
	if (wrpr.has_add_flow_rq()) {
	}
	if (wrpr.has_add_flow_rp()) {
	}
	for (int i=0;i<wrpr.tcp_flow_size();i++) {
		const tcp_flow_wrapper& tfw = wrpr.tcp_flow(i);
		map<int, IP2VeraStreamHub*>::iterator f2h_it = flow_to_hub.find(tfw.flow_id());
		if (f2h_it != flow_to_hub.end()) {
			cout << "TcpStream::process_message warning - unknown flow with id " << tfw.flow_id() << endl;
			continue;
		}
		IP2VeraStreamHub* p2vh = f2h_it->second;
		P2VeraBasicMessage p2bm(tfw.flow_data());
		p2vh->receive_message(p2bm);
	}
	return 0;
}

int TcpStream::get_fd() {
	return fd;
}


///////////////////////////////////////////////////////////////////////////////

TcpStreamManager::TcpStreamManager(int start_port) {
	pthread_mutex_init (&mtx, NULL);
	sock_listen = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sock_listen, F_SETFL, 0);
	if(sock_listen < 0) {
		cout << "TcpStreamManager::TcpStreamManager error - failed to create socket" << endl;
		return;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	accept_port = start_port;
	bool binded = false;

	while (accept_port < 65534) {
		addr.sin_port = htons(accept_port);
		if(bind(sock_listen, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			accept_port++;
			continue;
		}
		cout << "TcpStreamManager::TcpStreamManager info - binded on port " << accept_port << endl;
		binded = true;
		break;
	}
	if (!binded) {
		cout << "TcpStreamManager::TcpStreamManager error - failed to bind socket" << endl;
		return;
	}
	listen(sock_listen,MAX_TCP_CONNECTIONS);

	//Создаем потоки
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, &tcp_accept_thread_fcn, this); //Поток, принимающий входящие соединения
	pthread_create(&thread_id, NULL, &tcp_poll_thread_fcn, this); //Поток, принимающий данные по tcp
}

int TcpStreamManager::accept_thread() {
	while (true) {
		int sock = accept(sock_listen, NULL, NULL);
		if(sock>=0) {
			//Создаем ведомый канал с удаленным сервером. В процессе обмена параметры сервера будут
			//уточнены, и тогда, возможно, он будет удален или преобразован в ведущий
			pthread_mutex_lock(&mtx);
			rsu_fd_item rfi;
			rfi.tcp_stream = new TcpStream(sock);
			rfi.fd = sock;
			rsu_fd_list_modified = true;
			rsu_fd_items.push_back(rfi);
			pthread_mutex_unlock(&mtx);
		} else if (EINVAL == errno || EBADF==errno) {
			cout << "TcpStreamManager::accept_thread info - server stopped" << endl;
			pthread_exit(0);
			return 0;
		} else {
			cout << errno << endl;
			cout << "TcpStreamManager::accept_thread warning - accept was interupted without connection" << endl;
			continue;
		}
	}

	return 0;
}

void TcpStreamManager::add_server(RemoteSrvUnit& rsu, int remote_port) {
	pthread_mutex_lock(&mtx);
	for (list<rsu_fd_item>::iterator it=rsu_fd_items.begin();it!=rsu_fd_items.end();it++) {
		if (rsu == it->rsu) { //Такой сервер уже зарегистрирован.
			pthread_mutex_unlock(&mtx);
			cout << "TcpStreamManager::add_server info - server " << rsu.get_uniq_id() << " was already registered" << endl;
			return;
		}
	}

	cout << "TcpStreamManager::add_server info - registering server " << rsu.get_uniq_id() << endl;
	rsu_fd_item rfi;
	rfi.rsu = rsu;
	rfi.tcp_stream = new TcpStream(rsu, remote_port);
	rfi.fd = rfi.tcp_stream->get_fd();

	rsu_fd_list_modified = true;
	rsu_fd_items.push_back(rfi);
	pthread_mutex_unlock(&mtx);
}

void TcpStreamManager::remove_server(RemoteSrvUnit& rsu) {
	pthread_mutex_lock(&mtx);
	pthread_mutex_unlock(&mtx);
}

int TcpStreamManager::get_fd() {
	return sock_listen;
}

int TcpStreamManager::get_port() {
	return accept_port;
}

void TcpStreamManager::rcv_thread() {
	pollfd *pfd = NULL;
	while (true) {
		//Обновляем список файловых дескрипторов присоединенных серверов.
		pthread_mutex_lock(&mtx);
		if (rsu_fd_list_modified) {
			rsu_fd_list_modified = false;

			if (NULL!=pfd && last_rsu_fd_count!=(int)rsu_fd_items.size()) {
				delete[] pfd;
			}
			last_rsu_fd_count = rsu_fd_items.size();
			pfd = new pollfd[last_rsu_fd_count];
			int fd_cnt = 0;
			for (list<rsu_fd_item>::iterator it=rsu_fd_items.begin();it!=rsu_fd_items.end();it++, fd_cnt++) {
				pfd[fd_cnt].fd = it->fd;
				pfd[fd_cnt].events = POLLIN | POLLHUP;
				pfd[fd_cnt].revents = 0;
			}
		}
		pthread_mutex_unlock(&mtx);
		if (0 == last_rsu_fd_count) { //Если список дескрипторов пуст, засыпаем, ждем пока он увеличится
			usleep(10000);
			continue;
		}

		//Ожидаем прихода данных
		if (poll(pfd, last_rsu_fd_count, 100) < 1) {
			continue;
		}
		//Проверяем сработавшие дескрипторы
		pthread_mutex_lock(&mtx);
		for (int i=0;i<last_rsu_fd_count;i++) {
			if (0==pfd[i].revents) continue;
			pfd[i].revents = 0;
			for (list<rsu_fd_item>::iterator it=rsu_fd_items.begin();it!=rsu_fd_items.end();it++) {
				if (pfd[i].fd == it->fd) {
					it->tcp_stream->receive_message();
				}
			}
		}
		pthread_mutex_unlock(&mtx);
	}
	if (NULL != pfd)
		delete[] pfd;
}


void* tcp_accept_thread_fcn (void* thread_arg) {
	TcpStreamManager* tsm = (TcpStreamManager*)thread_arg;
	tsm->accept_thread();
	return NULL;
}

void* tcp_poll_thread_fcn (void* thread_arg) {
	TcpStreamManager* tsm = (TcpStreamManager*)thread_arg;
	tsm->rcv_thread();
	return NULL;
}



