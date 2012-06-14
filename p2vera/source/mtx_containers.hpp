/*
 * mtx_containers.cpp
 *
 *  Created on: 10.06.2012
 *      Author: drozdov
 */

#ifndef MTX_CONTAINERS_HPP_
#define MTX_CONTAINERS_HPP_

#include <pthread.h>

#include <iostream>
#include <queue>

#include "mtx_containers.h"

template<class T> mtx_queue<T>::mtx_queue() {
	pthread_mutex_init (&mtx, NULL);
}

template<class T> mtx_queue<T>::~mtx_queue() {
	pthread_mutex_destroy(&mtx);
}

template<class T> void mtx_queue<T>::push_back(T itm) {
	pthread_mutex_lock(&mtx);
	qq.push(itm);
	pthread_mutex_unlock(&mtx);
}

template<class T> T mtx_queue<T>::pop_front() {
	pthread_mutex_lock(&mtx);
	T itm = qq.front();
	qq.pop();
	pthread_mutex_unlock(&mtx);
	return itm;
}

template<class T> void mtx_queue<T>::clear() {
	pthread_mutex_lock(&mtx);
	std::queue<IRemoteNfServer*> empty_qq;
	std::swap(qq, empty_qq);
	pthread_mutex_unlock(&mtx);
}

template<class T> int mtx_queue<T>::size() {
	pthread_mutex_lock(&mtx);
	int s = qq.size();
	pthread_mutex_unlock(&mtx);
	return s;
}

#endif
