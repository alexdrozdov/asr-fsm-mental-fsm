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
#include <tcl8.5/tcl.h>

#include <vector>
#include <map>
#include <string>

#include "base_trigger.h"
#include "virt_trigger.h"

enum ENetlinkMsgType {
	nlmt_link = 0,       // Управление потоком передачи данных
	nlmt_security = 1,   // Управление безопасностью. Смена паролей и режима шифрования
	nlmt_time = 2,       // Информация о переключении времени
	nlmt_trig = 3,       // Информация о переключении триггеров в текущем такте
	nlmt_tcl = 4,        // tcl-команда. Предполагает ответ сервера
	nlmt_trig_manage = 5,// Управление работой триггеров клиента
	nlmt_text = 6        // Передача текстового отчета от сервера клиенту. Может интерпретироваться по соглашению клиента и сервера
};

enum ENetlinkLinkType {
	nllt_version  = 1,         //Определение текущей версии клиента и сервера. Не предполагает дополнительных данных
	nllt_reset_buffers = 2,    //Сброс буферов. Не предполагает дополнительных данных
	nllt_close_connection = 3, //Корректное завершение соединения. Не предполагает дополнительных данных.
	nllt_sleep = 4,            //Усыпление клиента на заданное время. Позволяет снизить нагрузку на сервер. Дополнительные данные: длина 1, время в миллисекундах (int).
	nllt_samplerate = 5        //Установить частоту дискретизации
};

enum EBufProcessState {
	bps_unknown = 0,
	bps_start   = 1,
	bps_body    = 2,
	bps_escape  = 3,
	bps_end     = 4
};

typedef struct _netlink_header {
	int msg_type;
	int msg_length;
} netlink_hdr;

#define FRAME_START  0xFB
#define FRAME_END    0xFE
#define FRAME_ESCAPE 0xFC

//Максимальный размер принимающего буфера
#define MAX_RCV_BUF 10000
//Количество принимающих буферов
#define RCV_BUF_COUNT 10

#define MAX_QUEUE_SIZE 200

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

//Прототип класса, отправляемого через netlink
class NetlinkMessage {
public:
	virtual ~NetlinkMessage();
	virtual int RequiredSize() = 0;
	virtual void Dump(unsigned char* space) = 0;
	virtual void Clear() = 0;
};

//Структура описывает буфер, готовый к отправке через Netlink
typedef struct _send_message_struct {
	unsigned char* data;
	int length;
} send_message_struct, *psend_message_struct;


void* netlinksender_thread_function (void* thread_arg);


class CNetLink {
public:
	CNetLink(std::string filename);

	int RegisterVirtualTrigger(CVirtTrigger* trigger);
	int TclHandler(Tcl_Interp* interp, int argc, CONST char *argv[]);

	virtual int OpenConection();
	virtual int BeginListen();
	virtual int Accept();
	virtual int CloseConnection();

	int MkDump(bool enable);
	int MkDump(bool enable, std::string file_name);

	virtual void SendTextResponse(std::string response_text);
	int Send(NetlinkMessage* msg);
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

	//Методы и члены, обеспечивающие передачу данных по обратному каналу
	int send_queue_size;
	pthread_mutex_t send_queue_mutex;
	pthread_cond_t  send_queue_cv;

	pthread_mutex_t to_send_mutex;
	std::queue<send_message_struct> to_send;

	int coded_length[256];
	unsigned int outcomming;

	bool dump_enabled;
	bool dump_to_file;
	std::ofstream dump_stream; //Поток, в который выводится дамп

	int thread_function();
	void initialize_send_queue();
	void set_queue_size(int queue_size, bool signal_change);
	void incr_queue_size();
	void decr_queue_size();

	int pack_n_send(send_message_struct sms);

	friend void* netlinksender_thread_function (void* thread_arg);
};

extern int create_net_link(std::string filename);
extern void* netlink_accept_thread (void* thread_arg);


#endif /* NET_LINK_H_ */
