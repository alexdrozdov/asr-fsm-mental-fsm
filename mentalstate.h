/*
 * mentalstate.h
 *
 *  Created on: 16.06.2011
 *      Author: drozdov
 */

#ifndef MENTALSTATE_H_
#define MENTALSTATE_H_

#include <string>

#ifdef AUXPACKAGES
#include <tcl.h>
#else
#include <tcl8.5/tcl.h>
#endif


int init_mentalstate_structs(Tcl_Interp *interp);


#endif /* MENTALSTATE_H_ */
