/*
 * net_link.h
 *
 *  Created on: 26.06.2011
 *      Author: drozdov
 */

#ifndef NET_LINK_H_
#define NET_LINK_H_

#include <sys/types.h>
#include <sys/socket.h>

#include <vector>
#include <map>
#include <string>

#include "tcl.h"
#include "base_trigger.h"
#include "virt_trigger.h"

enum ENetlinkMsgType {
	nlmt_link = 0,       // Управление потоком передачи данных
	nlmt_security = 1,   // Управление безопасностью. Смена паролей и режима шифрования
	nlmt_time = 2,       // Информация о переключении времени
	nlmt_trig = 3,       // Информация о переключении триггеров в текущем такте
	nlmt_tcl = 4,        // tcl-команда. Предполагает ответ сервера
	nlmt_trig_manage = 5 // Управление работой триггеров клиента
};

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

//Максимальный размер принимающего буфера
#define MAX_RCV_BUF 10000
//Количество принимающих буферов
#define RCV_BUF_COUNT 10

typedef struct _time_msg {
	long long current_time;
} time_msg;

typedef struct _trig_msg_item {
	int out_id;
	double value;
} trig_msg_item, *ptrig_msg_item;

typedef struct _trig_msg_trg_item {
	int trig_id;
	int out_count;
	ptrig_msg_item outs;
} trig_msg_trg_item, *ptrig_msg_trg_item;

typedef struct _trig_msg {
	int trig_count;
	ptrig_msg_trg_item triggers;
} trig_msg;


typedef struct _netlink_msg {
	int msg_type;
	int msg_length;
	char* msg_data;
} netlink_msg;

class CNetLink {
public:
	CNetLink(std::string filename);

	int RegisterVirtualTrigger(CVirtTrigger* trigger);
	int TclHandler(Tcl_Interp* interp, int argc, CONST char *argv[]);

	virtual int OpenConection();
	virtual int BeginListen();
	virtual int Accept();
	virtual int CloseConnection();
private:
	int listener;
	int port_number;

	bool online;
	bool listen_active;
	int max_connections;

	int sock; //Сокет клиентского соединения

	int buf_proc_state;

	unsigned char *cur_buf; //Текущий буфер для декодируемого сообщения
	unsigned char *pcur_buf;
	int cur_buf_usage; //Количество использованных ячеек буфера

	int accept_thread();
	int handle_network();
	int process_buffer(unsigned char* buf, int len);
	int process_message(unsigned char* buf, int len);
	int process_trig_msg(unsigned char* buf, int len);
	int process_time_msg(unsigned char* buf, int len);
	int process_sec_msg(unsigned char* buf, int len);
	int process_link_msg(unsigned char* buf, int len);
	friend void* netlink_accept_thread (void* thread_arg);

	std::vector<CVirtTrigger*> virtuals;
	std::map<std::string,CVirtTrigger*> by_name;

	bool dump_instream;
};

extern CNetLink *net_link;
extern int create_net_link(std::string filename);
extern void* netlink_accept_thread (void* thread_arg);


#endif /* NET_LINK_H_ */
