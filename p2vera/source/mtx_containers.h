/*
 * mtx_containers.h
 *
 *  Created on: 10.06.2012
 *      Author: drozdov
 */

#ifndef MTX_CONTAINERS_H_
#define MTX_CONTAINERS_H_

#include <pthread.h>
#include <queue>

template<class T> class mtx_queue {
public:
	mtx_queue();
	virtual ~mtx_queue();
	virtual void push_back(T itm);
	virtual T pop_front();
	virtual void clear();
	virtual int size();
private:
	pthread_mutex_t mtx;
	std::queue<T> qq;
};


#endif /* MTX_CONTAINERS_H_ */
