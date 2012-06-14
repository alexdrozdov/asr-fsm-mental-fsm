/*
 * virt_trigger.h
 *
 *  Created on: 22.06.2011
 *      Author: drozdov
 */

#ifndef VIRT_TRIGGER_H_
#define VIRT_TRIGGER_H_

#include <vector>
#include <string>
#include <queue>

#include "base_trigger.h"

enum ERemoteTriggerValueType {
        remote_value_double = 0,
        remote_value_int    = 1,
        remote_value_uint   = 2,
        remote_value_ch     = 3,
        remote_value_uch    = 4
};

typedef struct _remote_trigger_out {
        int offs;
        int value_type;
        union value {
                double        dblVal;
                int           intVal;
                unsigned int  uintVal;
                char          chVal;
                unsigned char uchVal;
        };
} remote_trigger_out;

typedef struct _remote_trigger_msg {
        int client_trigger_id;
        int server_trigger_id;
        long long time;           //Момент времени, в который произошло срабатывание триггера
        int out_count;            //Количество сработавших выходов, пришедших в этом сообщении
        remote_trigger_out* outs; //Данные этих выходов
} remote_trigger_msg;


class CVirtTrigger : public CBaseTrigger {
public:
        CVirtTrigger(std::string file_name);

        bool GetEnabled();
        void SetEnabled(bool b);

        double GetIntegralState();
        double GetExtendedState(int nExtender);
        int GetExtendedOutsCount();

        int BindTo(trig_inout_spec* self, trig_inout_spec* other);
        int BindFrom(trig_inout_spec* self, trig_inout_spec* other);

        void ProcessAnchestors();
        void SetOutput(int number,double value);
        void ReceiveFromRemoteTrigger(remote_trigger_msg* msg);

        int trigger_id;

private:
        std::queue<remote_trigger_msg*> states;
        remote_trigger_msg* current_msg;

        std::string szRemoteTriggerName;
        int remote_trigger_id;
        int outputs_count;
};

CVirtTrigger* load_virt_trigger(std::string filename);


#endif /* VIRT_TRIGGER_H_ */

