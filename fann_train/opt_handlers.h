/*
 * opt_handlers.h
 *
 *  Created on: 21.09.2011
 *      Author: drozdov
 */

#ifndef OPT_HANDLERS_H_
#define OPT_HANDLERS_H_

#include "common.h"

extern void fill_option_handlers();


extern int opt_help_handler(char opt_name, char* popt_val);


extern int opt_script_handler(char opt_name, char* popt_val);
extern int opt_inputs_count_handler(char opt_name, char* popt_val);
extern int opt_outputs_count_handler(char opt_name, char* popt_val);
extern int opt_hidden_neurons_handler(char opt_name, char* popt_val);
extern int opt_hidden_layers_handler(char opt_name, char* popt_val);
extern int opt_train_file_handler(char opt_name, char* popt_val);
extern int opt_net_file_handler(char opt_name, char* popt_val);
extern int opt_desired_error_handler(char opt_name, char* popt_val);
extern int opt_max_epochs_handler(char opt_name, char* popt_val);
extern int opt_project_handler(char opt_name, char* popt_val);



#endif /* OPT_HANDLERS_H_ */
