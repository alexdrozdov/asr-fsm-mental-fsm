/*
 * p2vera.cpp
 *
 *  Created on: 14.05.2012
 *      Author: drozdov
 */

#include <string.h>

#include <unistd.h>
#include <sys/time.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include <iostream>
#include <string>
#include <vector>

#include "p2vera.h"

using namespace std;

#define MAX_HOSTNAME_LEN 100
typedef struct _app_uniq_info {
	timeval tv_now;
	int proc_id;
	int parent_proc_id;
	int host_id;
	char hostname[MAX_HOSTNAME_LEN];
} app_uniq_info;


P2Vera::P2Vera() {
	generate_uniq_id();
}

void P2Vera::generate_uniq_id() {
	app_uniq_info aui;
	gettimeofday(&aui.tv_now, NULL);
	aui.proc_id = getpid();
	aui.parent_proc_id = getppid();
	aui.host_id = gethostid();
	gethostname(aui.hostname, MAX_HOSTNAME_LEN);

	MD5((const unsigned char*)(&aui), sizeof(app_uniq_info), (unsigned char*)md5_data);

	//Перекодируем в Base-64
	BIO *bmem, *b64;
	BUF_MEM *bptr;

	b64 = BIO_new(BIO_f_base64());
	bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, md5_data, MD5_DIGEST_LENGTH);
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);
	char *buff = new char[bptr->length+1];
	memcpy(buff, bptr->data, bptr->length-1);
	buff[bptr->length-1] = 0;
	uniq_id = buff;
	delete[] buff;
	BIO_free_all(b64);
}

std::string P2Vera::get_uniq_id() {
	return uniq_id;
}
