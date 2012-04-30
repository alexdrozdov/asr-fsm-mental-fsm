/*
 * trigger_loader.h
 *
 *  Created on: 03.08.2011
 *      Author: drozdov
 */

#ifndef TRIGGER_LOADER_H_
#define TRIGGER_LOADER_H_

#include <string>
#include <map>

#include "mental_fsm_ext.h"

extern int load_trigger_lib(std::string name, std::string libname);

extern std::map<std::string, trigger_init_ptr> load_trigger_handlers;

#endif /* TRIGGER_LOADER_H_ */
