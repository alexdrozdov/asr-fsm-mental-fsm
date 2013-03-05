/*
 * fann_handlers.h
 *
 *  Created on: 28.02.2013
 *      Author: drozdov
 */

#ifndef FANN_HANDLERS_H_
#define FANN_HANDLERS_H_

#include <string>
#include <tcl.h>

extern void test_train_data(std::string train_name);
extern int fann_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);
extern bool create_fann();
extern bool train_fann();
extern std::string build_def_regexp(int ndecimals);
extern bool validate_fann_opts();
extern bool validate_fann_train();
extern bool save_fann();
extern bool load_fann(std::string file_name);


#endif /* FANN_HANDLERS_H_ */
