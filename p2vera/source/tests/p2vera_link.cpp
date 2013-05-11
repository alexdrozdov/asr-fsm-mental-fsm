/*
 * p2vera_link.cpp
 *
 *  Created on: 30.04.2013
 *      Author: drozdov
 */

#include <iostream>

#include "p2vera_link.h"

using namespace std;
using namespace msg_mtpx;

AsrMsg1::AsrMsg1() {
	 msg1* m1 = mmp.add_m1();
	 m1->set_txt("");
}

AsrMsg1::AsrMsg1(std::string str) {
	msg1* m1 = mmp.add_m1();
	m1->set_txt(str);
}

AsrMsg1::AsrMsg1(const IP2VeraMessage& tm) {
}

AsrMsg1::~AsrMsg1() {
}

bool AsrMsg1::get_data(std::string& str) const {
	mmp.SerializePartialToString(&str);
	return true;
}

int AsrMsg1::get_data(void* data, int max_data_size) const {
	int nsize = mmp.ByteSize();
	if (nsize > max_data_size) return 0;
	mmp.SerializeToArray(data, nsize);
	return nsize;
}

int AsrMsg1::get_data_size() const {
	return mmp.ByteSize();
}

bool AsrMsg1::set_data(void* data, int data_size) {
	if (NULL == data) return false;
	return mmp.ParseFromArray(data, data_size);
}

bool AsrMsg1::set_data(std::string &str) {
	return mmp.ParseFromString(str);
}


void AsrMsg1::print() {
	if (mmp.m1_size() < 1) {
		cout << "AsrMsg1::print error - missing m1 message" << endl;
		return;
	}
	cout << mmp.m1(0).txt() << endl;
}


//////////////////////////////////////////////////////////////////

AsrMsgCmn::AsrMsgCmn() {
	 msg1* m1 = mmp.add_m1();
	 m1->set_txt("");
}

AsrMsgCmn::AsrMsgCmn(std::string str) {
	msg1* m1 = mmp.add_m1();
	m1->set_txt(str);
}

AsrMsgCmn::AsrMsgCmn(const IP2VeraMessage& tm) {
}

AsrMsgCmn::~AsrMsgCmn() {
}

bool AsrMsgCmn::get_data(std::string& str) const {
	mmp.SerializePartialToString(&str);
	return true;
}

int AsrMsgCmn::get_data(void* data, int max_data_size) const {
	int nsize = mmp.ByteSize();
	if (nsize > max_data_size) return 0;
	mmp.SerializeToArray(data, nsize);
	return nsize;
}

int AsrMsgCmn::get_data_size() const {
	return mmp.ByteSize();
}

bool AsrMsgCmn::set_data(void* data, int data_size) {
	if (NULL == data) return false;
	return mmp.ParseFromArray(data, data_size);
}

bool AsrMsgCmn::set_data(std::string &str) {
	return mmp.ParseFromString(str);
}


void AsrMsgCmn::print() {
	if (mmp.m1_size() > 0) {
		cout << mmp.m1(0).txt() << endl;
	}
	if (mmp.m2_size() > 0) {
		cout << mmp.m2(0).txt() << endl;
	}
}

AsrMsgCmn& AsrMsgCmn::operator>>(AsrMsg1& p2m) {
	p2m.mmp.mutable_m1()->CopyFrom(mmp.m1());
	return *this;
}

AsrMsgCmn& AsrMsgCmn::operator>>(AsrMsg2& p2m) {
	p2m.mmp.mutable_m2()->CopyFrom(mmp.m2());
	return *this;
}

bool AsrMsgCmn::has_m1() {
	return mmp.m1_size() > 0;
}

bool AsrMsgCmn::has_m2() {
	return mmp.m2_size() > 0;
}


//////////////////////////////////////////////////////////////////


AsrMsg2::AsrMsg2() {
	 msg2* m2 = mmp.add_m2();
	 m2->set_txt("");
}

AsrMsg2::AsrMsg2(std::string str) {
	msg2* m2 = mmp.add_m2();
	m2->set_txt(str);
}

AsrMsg2::AsrMsg2(const IP2VeraMessage& tm) {
}

AsrMsg2::~AsrMsg2() {
}

bool AsrMsg2::get_data(std::string& str) const {
	mmp.SerializePartialToString(&str);
	return true;
}

int AsrMsg2::get_data(void* data, int max_data_size) const {
	int nsize = mmp.ByteSize();
	if (nsize > max_data_size) return 0;
	mmp.SerializeToArray(data, nsize);
	return nsize;
}

int AsrMsg2::get_data_size() const {
	return mmp.ByteSize();
}

bool AsrMsg2::set_data(void* data, int data_size) {
	if (NULL == data) return false;
	return mmp.ParseFromArray(data, data_size);
}

bool AsrMsg2::set_data(std::string &str) {
	return mmp.ParseFromString(str);
}


void AsrMsg2::print() {
	if (mmp.m2_size() < 1) {
		cout << "AsrMsg2::print error - missing m2 message" << endl;
		return;
	}
	cout << mmp.m2(0).txt() << endl;
}



