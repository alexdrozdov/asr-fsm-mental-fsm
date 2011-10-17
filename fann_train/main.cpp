#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <string>

#include "common.h"
#include "opt_handlers.h"
#include "packet_funcs.h"
#include "interactive.h"

using namespace std;

void prepare_run_options() {
	run_options.b_i = false;
	run_options.b_o = false;
	run_options.b_h = false;
	run_options.b_l = false;
	run_options.b_t = false;
	run_options.b_n = false;

	run_options.desired_error = 1e-5;
	return;
	run_options.max_epochs = 5e5;
	run_options.epochs_between_reports = 10000;
}

bool validate_run_options() {
	if (run_options.mode_script) {
		return true;
	}
	if (run_options.mode_packet) {
		if (!run_options.b_i || !run_options.b_o || !run_options.b_l || !run_options.b_h || !run_options.b_t || !run_options.b_n) {
			cout << "Ошибка: отсутсвуют обязательные опции пакетного режима" << endl;
			return false;
		}
	}

	run_options.mode_interactive = true;
	return true;
}

int main(int argc, char *argv[]) {
	executable_path = get_executable_path(argv[0]);
	fill_option_handlers();
	prepare_run_options();
	parse_options(argc,argv);

	if (!validate_run_options()) {
		return 1;
	}

	if (run_options.mode_packet) {
		run_as_packet();
		return 0;
	}

	if (run_options.mode_interactive) {
		initialize_interactive(argc, argv);
		run_interactive();
		return 0;
	}


	return 0;
}
