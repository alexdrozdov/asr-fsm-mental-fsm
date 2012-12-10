/*
 * interactive.h
 *
 *  Created on: 13.10.2011
 *      Author: drozdov
 */

#ifndef INTERACTIVE_H_
#define INTERACTIVE_H_

#include <string>

extern void run_interactive(int argc, char *argv[]);
extern void run_script(int argc, char *argv[]);


typedef struct _fann_options {
	int inputs;
	int outputs;
	int layers;
	int hidden_neurons;

	double desired_error;
	int max_epochs;
	int log_period;

	std::string def_in_regexp;
	std::string def_out_regexp;

	std::string file_name;
	bool trained;
} fann_options;


extern fann_options fann_opts;


#endif /* INTERACTIVE_H_ */
