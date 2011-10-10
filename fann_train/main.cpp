#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <string>

#include "common.h"
#include "opt_handlers.h"
#include "fann.h"

using namespace std;

int main(int argc, char *argv[]) {
	executable_path = get_executable_path(argv[0]);
	fill_option_handlers();
	parse_options(argc,argv);

	if (1 == argc) {
		cout << "Usage:" << endl;
		cout << "Required options" << endl;
		cout << "\t-i value - inputs count" << endl;
		cout << "\t-o value - outputs count" << endl;
		cout << "\t-h value - hidden neurons count" << endl;
		cout << "\t-l value - layers count" << endl;
		cout << "\t-t file -  train file name" << endl;
		cout << "\t-n file - net data file name" << endl;

		cout << "Additional options" << endl;
		cout << "\t-e value - desired error" << endl;
		cout << "\t-m value - max epochs count" << endl;
		return 0;
	}

	const char *opts = "i:o:h:l:t:n:"; // доступные опции, каждая принимает аргумент
	int opt = 0;


	unsigned int num_input = 2;
	unsigned int num_output = 1;
	unsigned int num_layers = 3;
	unsigned int num_neurons_hidden = 3;
	float desired_error = (const float) 0.001;
	unsigned int max_epochs = 500000;
	unsigned int epochs_between_reports = 1000;

	string train_file_name = "";
	string net_file_name   = "";

	bool b_i = false;
	bool b_o = false;
	bool b_h = false;
	bool b_l = false;
	bool b_t = false;
	bool b_n = false;




	while((opt = getopt(argc, argv, opts)) != -1) { // вызываем getopt пока она не вернет -1
		switch(opt) {
			case 'i':
				num_input = atoi(optarg);
				b_i = true;
				break;
			case 'o':
				num_output = atoi(optarg);
				b_o = true;
				break;
			case 'l':
				num_layers = atoi(optarg);
				b_l = true;
				break;
			case 'h':
				num_neurons_hidden = atoi(optarg);
				b_h = true;
				break;
			case 't':
				train_file_name = optarg;
				b_t = true;
				break;
			case 'n':
				net_file_name = optarg;
				b_n = true;
				break;
			case 'e':
				desired_error = atof(optarg);
				break;
			case 'm':
				max_epochs = atoi(optarg);
				break;

		}
	}

	if (!b_i || !b_o || !b_l || !b_h || !b_t || !b_n) {
		cout << "error: some required options are missing" << endl;
		return 0;
	}


	struct fann *ann = fann_create_standard(num_layers, num_input, num_neurons_hidden, num_output);

	fann_set_activation_function_hidden(ann, FANN_SIGMOID_SYMMETRIC);
	fann_set_activation_function_output(ann, FANN_SIGMOID_SYMMETRIC);

	fann_train_on_file(ann,train_file_name.c_str(), max_epochs, epochs_between_reports, desired_error);

	fann_save(ann, net_file_name.c_str());

	fann_destroy(ann);

	return 0;
}
