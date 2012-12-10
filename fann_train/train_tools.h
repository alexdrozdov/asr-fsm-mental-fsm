/*
 * train_tools.h
 *
 *  Created on: 16.11.2011
 *      Author: drozdov
 */

#ifndef TRAIN_TOOLS_H_
#define TRAIN_TOOLS_H_

#include <floatfann.h>

 struct train_row {
	train_row();
	virtual ~train_row();
	int input_count;
	int output_count;

	fann_type *inputs;
	fann_type *outputs;
};

#endif /* TRAIN_TOOLS_H_ */
