/*
 * train_tools.cpp
 *
 *  Created on: 16.11.2011
 *      Author: drozdov
 */

#include <stdlib.h>
#include "train_tools.h"

train_row::train_row() {
	input_count = 0;
	output_count = 0;

	inputs = NULL;
	outputs = NULL;
}

train_row::~train_row() {
	//FIXME Обеспечить очистку памяти
	//if (NULL != inputs)
	//	delete[] inputs;
	//if (NULL != outputs)
	//	delete[] outputs;
}


