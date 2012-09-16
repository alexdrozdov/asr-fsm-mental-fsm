/*
 * inet_find.h
 *
 * Файл описывает интерфейсы компонента обнаружения программ, использующих
 * p2vera в пределах одного сегмента ЛВС.
 *
 *  Created on: 10.06.2012
 *      Author: drozdov
 */


#ifndef INET_FIND_H_
#define INET_FIND_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <list>

class INetFind;
class IRemoteNfServer;
class RemoteSrvUnit;
class IP2VeraStreamHub;
struct _stream_config;

class server_not_found {
public:
	server_not_found() {};
};

//Конфигурация модуля поиска приложений в сети
typedef struct _net_find_config {
	std::string nf_name;    // Кодовое название этой части приложения
	std::string nf_caption; // Полное  название этой части приложения
	std::string nf_address; // IP адрес, на котором обнаружен сервер
	std::string nf_port;    // Порт, на котором должен запускаться сервер
	std::string nf_hash;    // Идентификатор экземпляра приложения
	std::string nf_cluster; // Идентификатор кластера, к которому относится сервер
	int usage;              // Степень использования этого приложения. Меньше - лучше.
	bool scanable;          // Необходимость пинга сервера. При ложном значении сервер считается
	                        // работоспособным, его пинг запрещен
} net_find_config;

class IRemoteNfServer {
public:
	virtual ~IRemoteNfServer() = 0;
	virtual bool is_alive() = 0;      //Проверить возможность установки связи с этим приложением.
						              //Не гарантирует жизнеспособность приложения в текущий момент,
						              //опирается на наличие ответов в недавнем прошлом.
	virtual void forbide_usage() = 0; //Запретить проверку этого приложения на жизнеспособность,
						              //принудительно считать приложение отсутствующим.
	virtual void enable_usage() = 0;  //Разрешает работу с этим сервером, т.е. с любым сервером,
						              //расположенным на этой удаленной машине

	virtual int get_id() = 0;         //Получение идентификатора сервера в пределах этого приложения
	virtual std::string get_uniq_id() = 0;   //Получение уникального идентификатора
	virtual sockaddr_in& get_remote_sockaddr() = 0;
	virtual void add_alternate_addr(sockaddr_in& alt_addr) = 0; //Добавить альтернативный адрес, по которому можно найти этот сервер
	virtual bool validate_addr(sockaddr_in& alt_addr) = 0; //Проверить переданный адрес на принадлежность к этому серверу

	virtual void add_ping_request(int ping_id) = 0;  //Регистрация отправленного запроса
	virtual void add_ping_response(int ping_id) = 0; //Регистрация принятого ответа
	virtual void validate_alive() = 0;               //Проверка на количество потеряных запросов-ответов за единицу времени
	virtual bool ping_allowed() = 0;                 //Проверка возможности пинга. Пинг может быть запрещен, т.к. запрещено
										 //использование этого удаленного сервера или не прошел минимальный период ожидания
										 //с момента отправки последнего пинга.

	virtual bool requires_info_request() = 0; //Сервер нуждается в сборе дополнительной информации

	virtual bool is_broadcast() = 0; // Сервер на самом деле не существует, а используется для хранения информации о вещательных запросах
	virtual bool is_localhost() = 0; // Сервер рабоает на тойже машине, что и эта копия библиотеки
	virtual bool is_self() = 0;      // Сервер запущен в этом экземпляре программы
	virtual void print_info() = 0;   // Вывести параметры сервера в консоль

	virtual bool increase_ref_count() = 0; //Увеличивает счетчик ссылок на экземпляр класса
	virtual int decrease_ref_count() = 0; //Уменьшает счетчик ссылок на экземпляр класса
	virtual bool is_referenced() = 0;      //Позволяет проверить наличие ссылок на экземпляр и возможность его удаления
};

//Инетерфейс, предоставляющий доступ к удаленному серверу. Клиенты должны использовать в
//своих задачах именно этот класс, т.к. он обеспечивает подсчет ссылок на интерфейс
//IRemoteNfServer. Назначение класса - сборка мусора и удаление серверов, ставших недоступными
//по мере прекращения их использования
class RemoteSrvUnit {
public:
	RemoteSrvUnit();
	RemoteSrvUnit(const RemoteSrvUnit& original);
	RemoteSrvUnit(IRemoteNfServer* itm);
	RemoteSrvUnit& operator=(IRemoteNfServer* original);
	virtual ~RemoteSrvUnit();
	virtual bool is_alive();     //Признак активности узла
	virtual bool is_broadcast(); //Признак вещательного узла
	virtual bool is_localhost(); //Признак узла, расположенного на локальной машине
	virtual bool is_self();      //Признак узла, относящегося к этой программе
	virtual std::string get_uniq_id();
	virtual std::string get_name();
	virtual std::string get_caption();
	virtual sockaddr_in& get_remote_sockaddr();
	virtual IRemoteNfServer* irnfs_ptr();

	RemoteSrvUnit& operator=(RemoteSrvUnit& original);
	friend class INetFind;
	friend bool operator==(const RemoteSrvUnit& lh, const RemoteSrvUnit& rh);
	friend bool operator==(const RemoteSrvUnit& lh, const IRemoteNfServer* irns);
	friend bool operator==(const IRemoteNfServer* irns, const RemoteSrvUnit& rh);
	friend bool operator!=(const RemoteSrvUnit& lh, const RemoteSrvUnit& rh);
	friend bool operator!=(const RemoteSrvUnit& lh, const IRemoteNfServer* irns);
	friend bool operator!=(const IRemoteNfServer* irns, const RemoteSrvUnit& rh);
private:
	IRemoteNfServer* irnfs;
protected:
};

extern bool operator==(const RemoteSrvUnit& lh, const RemoteSrvUnit& rh);
extern bool operator==(const RemoteSrvUnit& lh, const IRemoteNfServer* irns);
extern bool operator==(const IRemoteNfServer* irns, const RemoteSrvUnit& rh);
extern bool operator!=(const RemoteSrvUnit& lh, const RemoteSrvUnit& rh);
extern bool operator!=(const RemoteSrvUnit& lh, const IRemoteNfServer* irns);
extern bool operator!=(const IRemoteNfServer* irns, const RemoteSrvUnit& rh);

//Интерфейс класса, обнаруживающего и отслеживающего списки приложений в сети
class INetFind {
public:

	virtual bool is_server() = 0;                                               //Позволяет проверить текущий режим этой копии библиотеки
	virtual int add_scanable_server(std::string address, std::string port) = 0; //Добавить адрес сервера, на котором могут запускаться приложения. Сервер будет периодически опрашиваться классом
	virtual int add_broadcast_servers(std::string port) = 0;                    //Автоматическое обнаружение серверов, отвечающих на вещательные запросы по указанному порту

	virtual void get_alive_servers(std::list<RemoteSrvUnit>& srv_list) = 0;     //Заполняет список действующих серверов.
	virtual RemoteSrvUnit get_by_sockaddr(sockaddr_in& sa) = 0;                 //Поиск приложения, слушающего по указанному адресу (ip+порт)
	virtual RemoteSrvUnit get_by_uniq_id(std::string uniq_id) = 0;              //Поиск приложения по его уникальному идентификатору

	virtual void print_servers() = 0;                                           //Вывод в консоль списка обнеруженных приложений

	virtual std::string get_uniq_id() = 0;                                      //Узнать уникальный идентификатор этого экземпляра класса
	virtual std::string get_name() = 0;                                         //Узнать название
	virtual std::string get_caption() = 0;                                      //Узнать название (отображаемое для пользователя)
	virtual std::string get_cluster() = 0;                                      //Определить кластер приложений, в котором заерегистрировался экземляр класса

	virtual int register_stream(_stream_config& stream_cfg, IP2VeraStreamHub* sh) = 0;         //Зарегистрировать новый поток сообщений, по которому возможно взаимодействие
	virtual IP2VeraStreamHub* find_stream_hub(std::string stream_name) = 0;     //Поиск зарегистрированного потока по его имени
};

extern INetFind* net_find_create(net_find_config *nfc);

#endif /* INET_FIND_H_ */
