/*
 * tclinterp_stream.h
 *
 *  Created on: 23.05.2013
 *      Author: drozdov
 */

#ifndef TCLINTERP_STREAM_H_
#define TCLINTERP_STREAM_H_

#include <iostream>
#include <sstream>
#include <string>

#ifdef AUXPACKAGES
#include <tcl.h>
#else
#include <tcl8.5/tcl.h>
#endif

class InterpResultStreamClear {
public:
	InterpResultStreamClear();
	virtual ~InterpResultStreamClear();
};

class InterpResultStreamApply {
public:
	InterpResultStreamApply();
	virtual ~InterpResultStreamApply();
};

class InterpResultStream {
public:
	InterpResultStream(Tcl_Interp* interp);
	InterpResultStream(InterpResultStream& irs);
	virtual ~InterpResultStream();
	virtual InterpResultStream& operator<<(std::string);
	virtual InterpResultStream& operator<<(const char*);
	virtual InterpResultStream& operator<<(int);
	virtual InterpResultStream& operator<<(double);
	virtual InterpResultStream& operator<<(InterpResultStreamClear);
	virtual InterpResultStream& operator<<(InterpResultStreamApply);
private:
	Tcl_Interp* interp;
	std::ostringstream ostr;
};

extern InterpResultStreamClear irs_clear;
extern InterpResultStreamApply irs_apply;

#endif /* TCLINTERP_STREAM_H_ */
