/*
 * cfg_reader.cpp
 *
 *  Created on: 09.08.2012
 *      Author: drozdov
 */

#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <list>

#include "cfg_reader.h"

using namespace std;


CfgItem::CfgItem(std::string section, std::string name, std::string value) {
	this->section = section;
	this->name = name;
	this->value = value;
}

//std::string CfgItem::string() {
//	return value;
//}

CfgItem::operator int() {
	return atoi(value.c_str());
}

CfgItem::operator bool() {
	if ("true"==value || "True"==value || "TRUE"==value || atoi(value.c_str())>0) return true;
	return false;
}

//////////////////////////////////////////////////////////////

CfgReaderIterator::CfgReaderIterator(std::string section, std::string name, cfg_item_iter cii, CfgReader* cr) {
	this->section = section;
	this->name = name;
	this->cur_cii = cii;
	this->cr = cr;
}

CfgReaderIterator::CfgReaderIterator(CfgReaderIterator& cri) {
	if (this == &cri) return;
	section = cri.section;
	name = cri.name;
	cur_cii = cri.cur_cii;
}

CfgReaderIterator::~CfgReaderIterator() {
}

CfgReaderIterator& CfgReaderIterator::operator=(CfgReaderIterator& cri) {
	if (this == &cri) return *this;
	section = cri.section;
	name = cri.name;
	cur_cii = cri.cur_cii;
	return *this;
}

CfgReaderIterator& CfgReaderIterator::operator++() {
	while (cur_cii!=cr->items.end()) {
		cur_cii++;
		if (cur_cii->section==section && cur_cii->name==name) break;
	}
	return *this;
}

const CfgItem& CfgReaderIterator::operator->() {
	return *cur_cii;
}

/////////////////////////////////////////////////////////////

bool operator==(CfgReaderIterator& lh, CfgReaderIterator& rh) {
	return (lh.cur_cii == rh.cur_cii);
}

bool operator==(CfgReaderIterator& lh, cfg_item_iter& rh) {
	return (lh.cur_cii == rh);
}

bool operator==(cfg_item_iter& lh, CfgReaderIterator& rh) {
	return (rh.cur_cii == lh);
}

bool operator!=(CfgReaderIterator& lh, CfgReaderIterator& rh) {
	return (lh.cur_cii != rh.cur_cii);
}

bool operator!=(CfgReaderIterator& lh, cfg_item_iter& rh) {
	return (lh.cur_cii != rh);
}

bool operator!=(cfg_item_iter& lh, CfgReaderIterator& rh) {
	return (rh.cur_cii != lh);
}


///////////////////////////////////////////////////////////////
CfgReader::CfgReader(std::string cfg_file) {
	FILE* f = fopen(cfg_file.c_str(), "r");
	if (NULL == f) {
		cout << "CfgReader::CfgReader error - couldn`t open file " << cfg_file << endl;
	}
	char* data[1024] = {0};
	while (!feof(f)) {
		fgets(data,1024,f);
		data[1023] = 0;
		int first_symbol = -1;
		int eff_len = 0;
		for (int i=0;i<1024;i++) {
			if ('#'==data[i]) break;
			if (0 == data[i]) break;
			if (first_symbol<0 && !isspace(data[i])) {
				first_symbol = i;
			}
			if (first_symbol>=0 && !isspace(data[i])) {
				eff_len++;
			}
		}
		if (0==eff_len) continue;
	}
	fclose(f);
}

CfgReader::~CfgReader() {
}

CfgReaderIterator CfgReader::get_cfg_items(std::string section, std::string name) {
	for (cfg_item_iter cii=items.begin();cii!=items.end();cii++) {
		if (cii->section==section && cii->name==name) {
			CfgReaderIterator cri(section, name, cii, this);
			return cri;
		}
	}
	CfgReaderIterator cri(section, name, items.end(), this);
	return cri;
}

cfg_item_iter CfgReader::end() {
	return items.end();
}

cfg_item_iter CfgReader::begin() {
	return items.begin();
}


