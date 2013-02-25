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
#include <string.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <unistd.h>
#include <fcntl.h>

#include <pthread.h>

#include "crc.h"

#include "net_link.h"
#include "mental_fsm.h"
#include "back_messages.h"
#include "global_vars.h"

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

	//Заполняем таблицу длин кодированных символов
	for (int i=0;i<256;i++) {
		coded_length[i] = 1;
	}
	coded_length[FRAME_START]  = 2;
	coded_length[FRAME_END]    = 2;
	coded_length[FRAME_ESCAPE] = 2;

	dump_enabled = false;
	dump_to_file = false;
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
			//cout << "CNetLink::accept_thread info - listening socket closed" << endl;
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

	//Включаем управление очередью
	initialize_send_queue();
	pthread_mutex_init(&to_send_mutex, NULL);

	//Запускаем поток, обеспечивающий обратный канал связи
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, &netlinksender_thread_function, this);

	while(true) {
		int bytes_read = read(sock, buf, 1024);
		if(bytes_read <= 0) {
			//cout << errno << endl;
			cout << "CNetLink::accept_thread info - child disconnected with code "<< errno << endl;
			break;
		}
		process_buffer(buf,bytes_read);
	}

	close(sock);
	//cout << "Will now exit" << endl;
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

int CNetLink::process_message(unsigned char* buf, int len) {
	dsp::dsp_package pkg;
	if (!pkg.ParseFromArray(buf, len)) {
		cout << "CNetLink::process_message atacked - wrong message format" << endl;
		return 1;
	}
	int err = 0;
	if (pkg.time_inst_size() > 0) {
		err += process_time_msg(pkg.time_inst(0));
	}
	if (pkg.samplerate_inst_size() > 0) {
		err += process_samplerate_msg(pkg.samplerate_inst(0));
	}
	if (pkg.modified_triggers_inst_size() > 0) {
		err += process_trig_msg(pkg.modified_triggers_inst(0));
	}
	return err;
}

int CNetLink::process_trig_msg(const ::dsp::modified_triggers& mt) {
	::google::protobuf::RepeatedPtrField< ::dsp::modified_triger>::const_iterator ctit;
	for (ctit=mt.items().begin();ctit!=mt.items().end();ctit++) {
		const ::dsp::modified_triger& mmt = *ctit;
		if (dump_enabled && dump_to_file) {
			dump_stream << "CNetLink::process_trig_msg info - trigger id: " << mmt.id() << "; outputs size: " << mmt.outputs_size();
			dump_stream << "; outputs: ";
			::google::protobuf::RepeatedPtrField< ::dsp::trigger_output>::const_iterator coit;
			for (coit=mmt.outputs().begin();coit!=mmt.outputs().end();coit++) {
				dump_stream << " out[" << coit->out_id() << "]=" << coit->value();
			}
			dump_stream << endl;
		} else if (dump_enabled) {
			cout << "CNetLink::process_trig_msg info - trigger id: " << mmt.id() << "; outputs size: " << mmt.outputs_size();
			cout << "; outputs: ";
			::google::protobuf::RepeatedPtrField< ::dsp::trigger_output>::const_iterator coit;
			for (coit=mmt.outputs().begin();coit!=mmt.outputs().end();coit++) {
				cout << " out[" << coit->out_id() << "]=" << coit->value();
			}
			cout << endl;
		}

		if (0 == mmt.outputs_size()) {
			continue; //Триггер был передан, но не содержит ниодного изменившегося выхода
		}

		int trig_id = mmt.id();
		CVirtTrigger* vt = virtuals[trig_id];
		//Проверяем, что триггер с таким id зарегистрирован
		if (0>trig_id || (unsigned)trig_id >= virtuals.size() || NULL == vt) {
			cout << "CNetLink::process_trig_msg atacked - trigger id is unregistered" << endl;
			exit(4);
		}

		::google::protobuf::RepeatedPtrField< ::dsp::trigger_output>::const_iterator coit;
		for (coit=mmt.outputs().begin();coit!=mmt.outputs().end();coit++) {
			vt->SetOutput(coit->out_id(), coit->value());
		}
	}
	return 0;
}

int CNetLink::process_time_msg(const ::dsp::time_message& tm) {
	long long new_time = tm.current_time();
	if (dump_enabled && dump_to_file) {
		dump_stream << "CNetLink::process_time_msg info - remote time is " << new_time << endl;
	} else if (dump_enabled) {
		cout << "CNetLink::process_time_msg info - remote time is " << new_time << endl;
	}
	if (new_time < fsm->GetCurrentTime()) {
		cout << "CNetLink::process_time_msg atacked - time " << new_time << " is less than current time " << fsm->GetCurrentTime() << endl;
		exit(4);
	}
	fsm->RunToTime(fsm->ScaleRemoteTime(new_time));
	return 0;
}

int CNetLink::process_samplerate_msg(const ::dsp::samplerate_message& srtm) {
	if (dump_enabled && dump_to_file) {
		dump_stream << "CNetLink::process_samplerate_msg info - remote samplerate is " << srtm.samplerate() << endl;
	} else if (dump_enabled) {
		cout << "CNetLink::process_samplerate_msg - remote samplerate is " << srtm.samplerate() << endl;
	}
	fsm->SetRemoteSamplerate(srtm.samplerate());
	return 0;
}

int CNetLink::process_sec_msg(unsigned char* buf, int len) {
	return 0;
}

int CNetLink::process_link_msg(unsigned char* buf, int len) {
	int req_size = 4+4+sizeof(unsigned int)*2;
	if (len < req_size) {
		cout << "CNetLink::process_link_msg atacked - frame is too short to be valid" << endl;
		exit(4);
	}
	unsigned int *pcmd = (unsigned int*)(buf + 8);
	switch (*pcmd) {
		case nllt_version:
			//FIXME Реализовать
		case nllt_reset_buffers:
		case nllt_close_connection:
		case nllt_sleep:
			cout << "CNetLink::process_link_msg warning - запрос на нереализованную команду управления соединением - " << *pcmd << endl;
			break;
		case nllt_samplerate:
			{unsigned int *psamplerate = (unsigned int*)(buf + 12);
			fsm->SetRemoteSamplerate(*psamplerate);
			}
			break;
		default:
			cout << "CNetLink::process_link_msg warning - запрос на неизвестную команду управления соединением - " << *pcmd << endl;
	}
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

void CNetLink::SendTextResponse(std::string response_text) {
	NetlinkMessageString* nms = new NetlinkMessageString(response_text);
	Send(nms);
	delete nms;
}


void CNetLink::set_queue_size(int queue_size, bool signal_change) {
	/* Lock the mutex before accessing the flag value.  */
	pthread_mutex_lock(&send_queue_mutex);

	/* Set the flag value, and then signal in case thread_function is
	blocked, waiting for the flag to become set.  However,
	thread_function can’t actually check the flag until the mutex is
	unlocked.  */

	send_queue_size = queue_size;
	if (signal_change) {
		pthread_cond_signal(&send_queue_cv);
	}
	/* Unlock the mutex.  */
	pthread_mutex_unlock(&send_queue_mutex);
}

void CNetLink::incr_queue_size() {
	bool overflow = false;
	/* Lock the mutex before accessing the flag value.  */
	pthread_mutex_lock(&send_queue_mutex);

	if (send_queue_size >= MAX_QUEUE_SIZE) {
		overflow = true;
	} else {
		send_queue_size++;
		pthread_cond_signal(&send_queue_cv);
	}
	pthread_mutex_unlock(&send_queue_mutex);

	if ( overflow ) {
		cout << "NetlinkSender error - message queue overflow. Will now exit" << endl;
		exit(3);
	}
}

void CNetLink::decr_queue_size() {
	/* Lock the mutex before accessing the flag value.  */
	pthread_mutex_lock(&send_queue_mutex);

	if (send_queue_size > 0) {
		send_queue_size--;
	}
	pthread_mutex_unlock(&send_queue_mutex);
}


void CNetLink::initialize_send_queue() {
	pthread_mutex_init (&send_queue_mutex, NULL);
	pthread_cond_init (&send_queue_cv, NULL);

	send_queue_size = 0;
}

int CNetLink::Send(NetlinkMessage* msg) {
	if (NULL == msg) {
		cout << "NetlinkSender::Send error - zero pointer received" << endl;
	}

	//Извлекаем данные, которые необходимо передать
	send_message_struct sms;
	sms.length = msg->RequiredSize();
	sms.data   = (unsigned char*)malloc(sms.length);
	msg->Dump(sms.data);

	//Записываем эти данные в очередь на отправку
	pthread_mutex_lock(&to_send_mutex);
	to_send.push(sms);
	pthread_mutex_unlock(&to_send_mutex);

	//Увеличиваем размер очереди
	incr_queue_size();

	if (dump_enabled && dump_to_file) {
		dump_stream << "push " << send_queue_size << endl;
	} else if (dump_enabled) {
		cout << "push " << send_queue_size << endl;
	}

	return 0;
}


int CNetLink::thread_function() {
	while (1) {
		/* Lock the mutex before accessing the flag value.  */
		pthread_mutex_lock (&send_queue_mutex);
		while (0 == send_queue_size) {
			/* The flag is clear.  Wait for a signal on the condition
			variable, indicating that the flag value has changed.  When the
			signal arrives and this thread unblocks, loop and check the
			flag again.  */
			pthread_cond_wait (&send_queue_cv, &send_queue_mutex);
		}
		/* When we’ve gotten here, we know the flag must be set.  Unlock
		the mutex.*/
		pthread_mutex_unlock (&send_queue_mutex);

		pthread_mutex_lock(&to_send_mutex);
		send_message_struct sms = to_send.front();
		to_send.pop();
		pthread_mutex_unlock(&to_send_mutex);
		//Уменьшаем размер очереди
		decr_queue_size();

		pack_n_send(sms);

		if (dump_enabled && dump_to_file) {
			dump_stream << "pop " << send_queue_size << endl;
		} else if (dump_enabled) {
			cout << "pop " << send_queue_size << endl;
		}
	}
	return 0;
}

int CNetLink::pack_n_send(send_message_struct sms) {
	//Определяем длину сообщения после его кодирования
	int msg_len = 0;
	for (int i=0;i<sms.length;i++) {
		msg_len += coded_length[sms.data[i]];
	}
	msg_len += 4; // FRAME_START + CRC-16 + FRAME_END

	unsigned char *coded_msg = (unsigned char*)malloc(msg_len);
	unsigned char *pcoded_msg = coded_msg;

	*pcoded_msg = FRAME_START;
	pcoded_msg++;

	unsigned short crc = CRC_INIT;

	for (int i=0;i<sms.length;i++) {
		unsigned char c = sms.data[i];
		CRC(crc, c);
		if (FRAME_START == c) {
			pcoded_msg[0] = FRAME_ESCAPE;
			pcoded_msg[1] = 0xCB;
			pcoded_msg += 2;
			continue;
		}
		if (FRAME_END == c) {
			pcoded_msg[0] = FRAME_ESCAPE;
			pcoded_msg[1] = 0xCE;
			pcoded_msg += 2;
			continue;
		}
		if (FRAME_ESCAPE == c) {
			pcoded_msg[0] = FRAME_ESCAPE;
			pcoded_msg[1] = FRAME_ESCAPE;
			pcoded_msg += 2;
			continue;
		}
		pcoded_msg[0] = c;
		pcoded_msg++;
	}
	free(sms.data);

	coded_msg[msg_len-3] = 0;//(crc&0xFF00) >> 8;
	coded_msg[msg_len-2] = 0;//crc & 0xFF;
	coded_msg[msg_len-1] = FRAME_END;

	if (dump_enabled && dump_to_file) {
		char ch[6];
		for (int i=0;i<msg_len;i++) {
			sprintf(ch,"0x%X",coded_msg[i]);
			dump_stream << ch << " ";
		}
		dump_stream << endl;
	} else if (dump_enabled) {
		char ch[6];
		for (int i=0;i<msg_len;i++) {
			sprintf(ch,"0x%X",coded_msg[i]);
			cout << ch << " ";
		}
		cout << endl;
	}


	int send_res = send(sock, coded_msg, msg_len, 0);//MSG_DONTWAIT);
	free(coded_msg);
	if (-1 == send_res) {
		if (EAGAIN == errno) {
			cout << "CNetLink::pack_n_send error - socket overflow" << endl;
		} else {
			cout << "CNetLink::pack_n_send error - " << errno << endl;
			cout << strerror(errno) << endl;
			cout << "Will now exit" << endl;
			exit(3);
		}
	}

	outcomming += msg_len;

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

int CNetLink::MkDump(bool enable) {
	if (dump_enabled) {
		//Дамп и так выводится. Закрываем существующий дескрптор
		if (dump_stream != cout) {
			dump_stream.close();
		}
	}
	dump_enabled = enable;
	dump_to_file = false;
	return 0;
}

int CNetLink::MkDump(bool enable, string file_name) {
	if (dump_enabled) {
		//Дамп и так выводится. Закрываем существующий дескриптор
		if (dump_stream != cout) {
			dump_stream.close();
		}
	}

	dump_enabled = enable;
	if (dump_enabled) {
		if ("cout" == file_name) {
			dump_to_file = false;
		} else {
			dump_to_file = true;
			dump_stream.open(file_name.c_str());
		}
	}
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
	if (3<=argc) {
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
		if ("dump_ostream" == cmd_op) {
			bool bdump_enable = false;
			if (cmd_pr == "enable") {
				bdump_enable = true;
			}

			bool bfile_name = false;
			string file_name = "";
			if (argc > 3 ) {
				bfile_name = true;
				file_name = argv[3];
			}

			if (!bfile_name) {
				MkDump(bdump_enable);
			} else {
				MkDump(bdump_enable, file_name);
			}
			return TCL_OK;
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

void* netlinksender_thread_function (void* thread_arg) {
	CNetLink* nls = (CNetLink*) thread_arg;
	nls->thread_function();

	return NULL;
}


