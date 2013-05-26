/*
 * tclinterp_stream.cpp
 *
 *  Created on: 23.05.2013
 *      Author: drozdov
 */

#include "tclinterp_stream.h"

using namespace std;

InterpResultStream::InterpResultStream(Tcl_Interp* interp) {
	if (NULL == interp) {
		cout << "InterpResultStream::InterpResultStream error - null pointer to interp" << endl;
		return;
	}
	this->interp = interp;
}

InterpResultStream::InterpResultStream(InterpResultStream& irs) {
	if (NULL == irs.interp) {
		cout << "InterpResultStream::InterpResultStream error - null pointer to interp" << endl;
		return;
	}
	this->interp = irs.interp;
}

InterpResultStream::~InterpResultStream(){}

InterpResultStream& InterpResultStream::operator<<(std::string str) {
	ostr << str;
	return *this;
}

InterpResultStream& InterpResultStream::operator<<(const char* str) {
	ostr << str;
	return *this;
}

InterpResultStream& InterpResultStream::operator<<(int v) {
	ostr << v;
	return *this;
}

InterpResultStream& InterpResultStream::operator<<(double v) {
	ostr << v;
	return *this;
}

InterpResultStream& InterpResultStream::operator<<(InterpResultStreamClear) {
	ostr.str("");
	return *this;
}

InterpResultStream& InterpResultStream::operator<<(InterpResultStreamApply) {
	if (NULL == interp) {
		cout << "InterpResultStream::operator<<(InterpResultStreamApply) error - NULL pointer to interp" << endl;
		return *this;
	}
	Tcl_SetResult(interp,(char*)ostr.str().c_str(),TCL_VOLATILE);
	return *this;
}

//////////////////////////////////////////

InterpResultStreamClear::InterpResultStreamClear() {}
InterpResultStreamClear::~InterpResultStreamClear() {}
InterpResultStreamApply::InterpResultStreamApply() {}
InterpResultStreamApply::~InterpResultStreamApply() {}

//////////////////////////////////////////

InterpResultStreamClear irs_clear;
InterpResultStreamApply irs_apply;

