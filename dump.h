/*
 * dump.h
 *
 *  Created on: 27.05.2013
 *      Author: drozdov
 */

#ifndef DUMP_H_
#define DUMP_H_

#include <string>
#include <fstream>
#include <ostream>

class DumpStream {
public:
	DumpStream();
	virtual ~DumpStream();

	virtual void disable();
	virtual bool enabled();
	virtual void set_stdout();
	virtual void set_file(std::string file_name);
	virtual void set_p2vera_stream(std::string stream_name);
	virtual DumpStream& operator<<(std::string);
	virtual DumpStream& operator<<(int);
	virtual DumpStream& operator<<(long long);
	virtual DumpStream& operator<<(double);
	virtual DumpStream& operator<<(std::ostream& (*f)(std::ostream&));
private:
	std::string file_name;
	std::ofstream dump_stream;
	bool dump_to_file;
	bool dump_to_p2vera;
	bool dump_to_stdout;
};


#endif /* DUMP_H_ */
