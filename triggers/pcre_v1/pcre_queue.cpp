/*
 * pcre_queue.cpp
 *
 *  Created on: 03.12.2011
 *      Author: drozdov
 */

#include <iostream>
#include <string>
#include <vector>
#include <queue>

#include "pcre_trigger.h"

using namespace std;

CPcreStringQueue::CPcreStringQueue(int max_size) {
	this->max_size = max_size;

	q = new char[max_size+1];
	cur_size = 0;
}

void CPcreStringQueue::push_back(char c) {
	if (cur_size == max_size) //При необходимости освобождаем место, удаляя старые значения
		pop_front(1);

	q[cur_size] = c;
	q[cur_size+1] = 0;
	cur_size++;
}

void CPcreStringQueue::pop_front(int count) {
	count = (count>cur_size)?cur_size:count;
	for (int i=count;i<cur_size;i++)
		q[i-count] = q[i];

	cur_size -= count;
}

std::string CPcreStringQueue::str() {
	return (string)q;
}
