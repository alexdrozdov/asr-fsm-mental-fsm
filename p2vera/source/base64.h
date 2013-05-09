/*
 * base64.h
 *
 *  Created on: 28.04.2013
 *      Author: drozdov
 *      Original prototype by René Nyffenegger. Licensining information is provided in base64.cpp
 */

#ifndef BASE64_H_
#define BASE64_H_

#include <string>

std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);


#endif /* BASE64_H_ */
