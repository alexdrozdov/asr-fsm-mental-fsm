/*
 * data_handlers.h
 *
 *  Created on: 28.02.2013
 *      Author: drozdov
 */

#ifndef DATA_HANDLERS_H_
#define DATA_HANDLERS_H_

#include <string>
#include <vector>

#include <tcl.h>

#include "interactive.h"

std::string build_data_list();
bool remove_data_entry (std::string name);
bool remove_data_entries (int argc, const char *argv[]);
bool reload_data_entry (std::string name);
bool show_data_entry (std::string name);
void dump_train_io(train_io *tio);
bool dump_data_entry (std::string name);
void split_to_double(const std::string &s, char delim, std::vector<double> &elems);
bool load_input_entry_data(train_entry *ten);
bool load_output_entry_data(train_entry *ten);
bool add_entry (int argc, const char *argv[]);
int data_handler(ClientData clientData, Tcl_Interp* interp, int argc, CONST char *argv[]);


#endif /* DATA_HANDLERS_H_ */
