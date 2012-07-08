/*
 * p2vera.h
 *
 *  Created on: 14.05.2012
 *      Author: drozdov
 */

#ifndef P2VERA_H_
#define P2VERA_H_

#include <string>

#include "inet_find.h"


class P2Vera {
public:
	P2Vera();
	std::string get_uniq_id();
private:
	char md5_data[MD5_DIGEST_LENGTH];
	void generate_uniq_id();
	std::string uniq_id;
	INetFind* nf;
};

#endif /* P2VERA_H_ */
