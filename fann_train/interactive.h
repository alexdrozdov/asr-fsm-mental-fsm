/*
 * interactive.h
 *
 *  Created on: 13.10.2011
 *      Author: drozdov
 */

#ifndef INTERACTIVE_H_
#define INTERACTIVE_H_


extern void initialize_interactive(int argc, char *argv[]);
extern void run_interactive();


typedef struct _fann_options {
	int inputs;
	int outputs;
	int layers;
	int hidden_neurons;

	double desired_error;
	int max_epochs;
	int log_period;
} fann_options;


extern fann_options fann_opts;


#endif /* INTERACTIVE_H_ */
