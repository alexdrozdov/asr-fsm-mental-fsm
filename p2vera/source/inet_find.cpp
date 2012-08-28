/*
 * inet_find.cpp
 *
 *  Created on: 14.06.2012
 *      Author: drozdov
 */

#include <string>
#include <iostream>

#include "inet_find.h"

using namespace std;

IRemoteNfServer::~IRemoteNfServer() {};

RemoteSrvUnit::RemoteSrvUnit(IRemoteNfServer* itm) {
	if (NULL == itm) {
		cout << "RemoteSrvUnit::RemoteSrvUnit error - null pointer to remote server interface" << endl;
		return;
	}
	irnfs = itm;
	irnfs->increase_ref_count(); //Сообщаем объекту, что он уже используется
	if (!irnfs->is_referenced()) {
		cout << "RemoteSrvUnit::RemoteSrvUnit error - couldnt create wrapper for IRemoteServer. Server was already unlinked" << endl;
	}
}

RemoteSrvUnit::RemoteSrvUnit() {
	irnfs = NULL;
}

RemoteSrvUnit::RemoteSrvUnit(const RemoteSrvUnit& original) {
	irnfs = original.irnfs;
	if (irnfs) irnfs->increase_ref_count();
}

RemoteSrvUnit::~RemoteSrvUnit() {
	if (!irnfs) return;
	if (0 == irnfs->decrease_ref_count()) { //Больше на этот объект ссылок не осталось. Удаляем его.
		delete irnfs;
	}
}

bool RemoteSrvUnit::is_alive() {
	if (!irnfs) return false;
	return irnfs->is_alive();
}

bool RemoteSrvUnit::is_broadcast() {
	if (!irnfs) return false;
	return irnfs->is_broadcast();
}

bool RemoteSrvUnit::is_localhost() {
	if (!irnfs) return false;
	return irnfs->is_localhost();
}

bool RemoteSrvUnit::is_self() {
	return irnfs->is_self();
}

std::string RemoteSrvUnit::get_uniq_id() {
	return irnfs->get_uniq_id();
}

std::string RemoteSrvUnit::get_name() {
	return "";//irnfs->get_name();
}

std::string RemoteSrvUnit::get_caption() {
	return "";//irnfs->get_caption();
}

sockaddr_in& RemoteSrvUnit::get_remote_sockaddr() {
	return irnfs->get_remote_sockaddr();
}

IRemoteNfServer* RemoteSrvUnit::irnfs_ptr() {
	return irnfs;
}

RemoteSrvUnit& RemoteSrvUnit::operator=(RemoteSrvUnit& original) {
	irnfs = original.irnfs;
	if (irnfs) irnfs->increase_ref_count();
	return *this;
}

RemoteSrvUnit& RemoteSrvUnit::operator=(IRemoteNfServer* original) {
	cout << "RemoteSrvUnit::operator= error - null pointer to remote server interface" << endl;
	irnfs = original;
	if (irnfs) irnfs->increase_ref_count();
	return *this;
}

bool operator==(const RemoteSrvUnit& lh, const RemoteSrvUnit& rh) {
	return (lh.irnfs == rh.irnfs);
}

bool operator==(const RemoteSrvUnit& lh, const IRemoteNfServer* irns) {
	return (irns==lh.irnfs);
}

bool operator==(const IRemoteNfServer* irns, const RemoteSrvUnit& rh) {
	return (irns==rh.irnfs);
}

bool operator!=(const RemoteSrvUnit& lh, const RemoteSrvUnit& rh) {
	return (lh.irnfs != rh.irnfs);
}

bool operator!=(const RemoteSrvUnit& lh, const IRemoteNfServer* irns) {
	return (irns!=lh.irnfs);
}

bool operator!=(const IRemoteNfServer* irns, const RemoteSrvUnit& rh) {
	return (irns!=rh.irnfs);
}

