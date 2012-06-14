/*
 * packet_funcs.cpp
 *
 *  Created on: 11.10.2011
 *      Author: drozdov
 */

#include <string>
#include <iostream>

#include "fann.h"
#include "common.h"
#include "packet_funcs.h"

using namespace std;


bool run_as_packet() {

	fann *ann = fann_create_standard(run_options.num_layers,
			run_options.num_input,
			run_options.num_neurons_hidden,
			run_options.num_output);

	fann_set_activation_function_hidden(ann, FANN_SIGMOID_SYMMETRIC);
	fann_set_activation_function_output(ann, FANN_SIGMOID_SYMMETRIC);

	fann_train_on_file(ann,
			run_options.train_file_name.c_str(),
			run_options.max_epochs,
			run_options.epochs_between_reports,
			run_options.desired_error);

	fann_save(ann, run_options.net_file_name.c_str());

	fann_destroy(ann);

	return true;
}


