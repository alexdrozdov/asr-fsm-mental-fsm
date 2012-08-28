/*
 * cfg_reader.cpp
 *
 *  Created on: 09.08.2012
 *      Author: drozdov
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

CfgItem::operator std::string() {
	return value;
}

std::string CfgItem::to_string() const {
	return value;
}

int CfgItem::to_int() const {
	return atoi(value.c_str());
}

bool CfgItem::to_bool() const {
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

CfgReaderIterator::CfgReaderIterator(const CfgReaderIterator& cri) {
	if (this == &cri) return;
	section = cri.section;
	name = cri.name;
	cur_cii = cri.cur_cii;
	cr = cri.cr;
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
	cur_cii++;
	while (cur_cii!=cr->items.end()) {
		if (cur_cii->section==section && cur_cii->name==name) break;
		cur_cii++;
	}
	return *this;
}

CfgReaderIterator& CfgReaderIterator::operator++(int) {
	cur_cii++;
	while (cur_cii!=cr->items.end()) {
		if (cur_cii->section==section && cur_cii->name==name) break;
		cur_cii++;
	}
	return *this;
}

const CfgItem* CfgReaderIterator::operator->() {
	//return *cur_cii;
	CfgItem& ci = *cur_cii;
	return &ci;
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

bool operator!=(const CfgReaderIterator lh, const CfgReaderIterator rh) {
	return (lh.cur_cii != rh.cur_cii);
}

bool operator!=(const CfgReaderIterator lh, cfg_item_iter rh) {
	return (lh.cur_cii != rh);
}

bool operator!=(cfg_item_iter lh, const CfgReaderIterator rh) {
	return (rh.cur_cii != lh);
}


///////////////////////////////////////////////////////////////
CfgReader::CfgReader(std::string cfg_file) {
	current_section = "_global";
	FILE* f = fopen(cfg_file.c_str(), "r");
	if (NULL == f) {
		cout << "CfgReader::CfgReader error - couldn`t open file " << cfg_file << endl;
		return;
	}
	try {
	char data[1024] = {0};
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
				if (first_symbol>=0 && data[i]!='\r' && data[i]!='\n') {
					eff_len++;
				}
			}
			data[eff_len] = 0;
			while (isspace(data[eff_len-1])) {
				eff_len--;
			}
			data[eff_len] = 0;
			if (0==eff_len) continue;

			char *str_start = data+first_symbol;
			if ('[' == str_start[0]) {
				parse_section(str_start, eff_len);
				continue;
			}
			parse_option(str_start, eff_len);
		}
	} catch (...) {
		cout << "CfgReader::CfgReader error - unhandled exception while parsing file " << cfg_file.c_str() << endl;
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

void CfgReader::parse_section(const char *str, int len) {
	char *str_copy = strndup(str, len);
	if (']' != str_copy[len-1]) {
		cout << "CfgReader::parse_section error - wrong section format " << str << endl;
		free(str_copy);
		throw;
	}
	str_copy[len-1] = 0;
	current_section = str_copy+1;
	free(str_copy);
}

void CfgReader::parse_option(const char *str, int len) {
	char *str_copy = strndup(str, len);
	char* equal_pos = strstr(str_copy, "=");
	if (NULL == equal_pos) {
		cout << "CfgReader::parse_option error - wrong option format: " << str << ", = is required" << endl;
		free(str_copy);
		throw;
	}
	*equal_pos = 0;
	//Выделяем название опции
	char* name_begin = str_copy;
	char* name_end = name_begin;
	while (0 != *name_end) {
		if (isspace(*name_end)) {
			*name_end = 0;
			break;
		}
		name_end++;
	}
	if (0==strnlen(name_begin, len)) {
		cout << "CfgReader::parse_option error - wrong option format: " << str << ", no name specified" << endl;
		free(str_copy);
		throw;
	}
	//Выделяем параметр опции
	char* value_begin = equal_pos+1;
	while(*value_begin) {
		if (!isspace(*value_begin)) {
			break;
		}
		value_begin++;
	}
	if (0==*value_begin) {
		cout << "CfgReader::parse_option error - wrong option format: " << str << ", no value specified" << endl;
		free(str_copy);
		throw;
	}
	if ('"' == *value_begin) {
		value_begin++;
		if ('"' == str_copy[len-1]) {
			str_copy[len-1] = 0;
		}
	}
	string name = name_begin;
	string value = value_begin;
	items.push_back(CfgItem(current_section, name, value));
	free(str_copy);
}




