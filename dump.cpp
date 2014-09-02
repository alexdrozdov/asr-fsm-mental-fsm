/*
 * dump.cpp
 *
 *  Created on: 27.05.2013
 *      Author: drozdov
 */

#include <iostream>

#include "dump.h"

using namespace std;

DumpStream::DumpStream() {
	dump_to_stdout = false;
	dump_to_file = false;
	dump_to_p2vera = false;
}

DumpStream::~DumpStream() {
	if (dump_to_file) {
		dump_stream.close();
	}
}

void DumpStream::disable() {
	if (dump_to_file) {
		dump_stream.close();
	}
	dump_to_stdout = false;
	dump_to_file = false;
	dump_to_p2vera = false;
}

bool DumpStream::enabled() {
	return dump_to_stdout | dump_to_stdout | dump_to_p2vera;
}

void DumpStream::set_stdout() {
	if (dump_to_file) {
		dump_stream.close();
	}
	dump_to_stdout = true;
	dump_to_file = false;
	dump_to_p2vera = false;
}

void DumpStream::set_file(std::string file_name) {
	dump_to_stdout = false;
	dump_to_p2vera = false;
	if (dump_to_file) {
		dump_stream.close();
	}
	dump_stream.open(file_name.c_str());
	dump_to_file = true;
}

void DumpStream::set_p2vera_stream(std::string stream_name) {
	if (dump_to_file) {
		dump_stream.close();
	}
	cout << "DumpStream::set_p2vera_stream error - not implemented" << endl;
}

DumpStream& DumpStream::operator<<(std::string str) {
	if (dump_to_stdout) cout << str;
	if (dump_to_file) dump_stream << str;
	return *this;
}

DumpStream& DumpStream::operator<<(int v) {
	if (dump_to_stdout) cout << v;
	if (dump_to_file) dump_stream << v;
	return *this;
}

DumpStream& DumpStream::operator<<(long long v) {
	if (dump_to_stdout) cout << v;
	if (dump_to_file) dump_stream << v;
	return *this;
}

DumpStream& DumpStream::operator<<(double v) {
	if (dump_to_stdout) cout << v;
	if (dump_to_file) dump_stream << v;
	return *this;
}

DumpStream& DumpStream::operator<<(std::ostream& (*f)(std::ostream&)) {
	if (dump_to_stdout) cout << endl;
	if (dump_to_file) dump_stream << endl;
	return *this;
}
