/*
 * mentalstate.h
 *
 *  Created on: 16.06.2011
 *      Author: drozdov
 */

#ifndef MENTALSTATE_H_
#define MENTALSTATE_H_

#include <string>
#include "tcl.h"


int init_mentalstate_structs(Tcl_Interp *interp);

extern std::string executable_path;
extern std::string project_path;

#endif /* MENTALSTATE_H_ */
