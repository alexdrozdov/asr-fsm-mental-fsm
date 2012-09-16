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

ICfgItemList::~ICfgItemList(){
}

CfgItem::CfgItem(std::string section, std::string name, std::string value) {
	this->section = section;
	this->name = name;
	this->value = value;
}

CfgItem::~CfgItem(){
	for (cfg_item_iter cii=items.begin();cii!=items.end();) {
		CfgItem *ci = *cii;
		if (NULL != ci) delete ci;
		cii = items.erase(cii);
	}
	items.clear();
}

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

cfg_item_iter CfgItem::end() {
	return items.end();
}

cfg_item_iter CfgItem::begin() {
	return items.begin();
}

CfgReaderIterator CfgItem::get_cfg_items(std::string name) {
	for (cfg_item_iter cii=items.begin();cii!=items.end();cii++) {
		if ((*cii)->name==name) {
			CfgReaderIterator cri(this->name, name, cii, this);
			return cri;
		}
	}
	CfgReaderIterator cri(this->name, name, items.end(), this);
	return cri;
}

//////////////////////////////////////////////////////////////

CfgReaderIterator::CfgReaderIterator(std::string section, std::string name, cfg_item_iter cii, ICfgItemList* cr) {
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
	while (cur_cii!=cr->end()) {
		if ((*cur_cii)->section==section && (*cur_cii)->name==name) break;
		cur_cii++;
	}
	return *this;
}

CfgReaderIterator& CfgReaderIterator::operator++(int) {
	cur_cii++;
	while (cur_cii!=cr->end()) {
		if ((*cur_cii)->section==section && (*cur_cii)->name==name) break;
		cur_cii++;
	}
	return *this;
}

CfgItem* CfgReaderIterator::operator->() {
	CfgItem& ci = *(*cur_cii);
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
			int eff_len = get_eff_line_length(data, 1023); //Определяем конец строки и удаляем пробелы в ее конце
			if (0==eff_len) continue;
			data[eff_len] = 0;

			if ('[' == data[0]) { //Строки, начинающиеся с [, обрабатываем как названия секций
				parse_section(data, eff_len);
			} else { //Все остальные строки обрабатываем, как значения
				parse_option(data, eff_len);
			}
		}
	} catch (...) {
		cout << "CfgReader::CfgReader error - unhandled exception while parsing file " << cfg_file.c_str() << endl;
	}
	fclose(f);
}

CfgReader::~CfgReader() {
	for (cfg_item_iter cii=items.begin();cii!=items.end();) {
		CfgItem *ci = *cii;
		if (NULL != ci) delete ci;
		cii = items.erase(cii);
	}
	items.clear();
}

CfgReaderIterator CfgReader::get_cfg_items(std::string section, std::string name) {
	for (cfg_item_iter cii=items.begin();cii!=items.end();cii++) {
		if ((*cii)->section==section && (*cii)->name==name) {
			CfgReaderIterator cri(section, name, cii, this);
			return cri;
		}
	}
	CfgReaderIterator cri(section, name, items.end(), this);
	return cri;
}

bool CfgReader::has_item(std::string section, std::string name) {
	CfgReaderIterator c_it = get_cfg_items(section, name);
	return (c_it != items.end());
}

int CfgReader::item_count(std::string section, std::string name) {
	int cnt = 0;
	CfgReaderIterator c_it = get_cfg_items(section, name);
	while (c_it!=items.end()) {
		c_it++;
		cnt++;
	}
	return cnt;
}

std::string CfgReader::get_item_as_string(std::string section, std::string name, std::string def_value) {
	CfgReaderIterator c_it = get_cfg_items(section, name);
	if (c_it != items.end()) return c_it->to_string();
	return def_value;
}

int CfgReader::get_item_as_int(std::string section, std::string name, int def_value) {
	CfgReaderIterator c_it = get_cfg_items(section, name);
	if (c_it != items.end()) return c_it->to_int();
	return def_value;
}

bool CfgReader::get_item_as_bool(std::string section, std::string name, bool def_value) {
	CfgReaderIterator c_it = get_cfg_items(section, name);
	if (c_it != items.end()) return c_it->to_bool();
	return def_value;
}

cfg_item_iter CfgReader::end() {
	return items.end();
}

cfg_item_iter CfgReader::begin() {
	return items.begin();
}

int CfgReader::get_eff_line_length(const char *str, int len) {
	int eff_len = 0;
	for (int i=0;i<len;i++) {
		if ('#'==str[i]) break;
		if (0 == str[i]) break;
		if (str[i]!='\r' && str[i]!='\n') eff_len++;
	}
	if (0 == eff_len) return 0;
	while (isspace(str[eff_len-1])) eff_len--;
	return eff_len;
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
	//Определяем уровень вложенности опции по количеству табуляций в ее начале
	int option_level = 0; //Уровень вложенности опции (синоним количества табуляций перед ее названием)
	for (int i=0;i<len;i++) {
		if (!isspace(str_copy[i])) break; //Начались символы, отличные от пробельных.
		if (str_copy[i] != '\t') {
			cout << "CfgReader::parse_option error - trailing white symbol differs from TAB: '" << str << "'" << endl;
			free(str_copy);
			throw;
		}
		option_level++;
	}
	char* eff_str_copy = str_copy + option_level; //Строка, не содержащая знаков табуляции
	char* equal_pos = strstr(eff_str_copy, "=");
	if (NULL == equal_pos) {
		cout << "CfgReader::parse_option error - wrong option format: '" << str << "', = is required" << endl;
		free(str_copy);
		throw;
	}
	*equal_pos = 0;
	//Выделяем название опции
	char* name_begin = eff_str_copy;
	char* name_end = name_begin;
	while (0 != *name_end) {
		if (isspace(*name_end)) {
			*name_end = 0;
			break;
		}
		name_end++;
	}
	if (0==strnlen(name_begin, len-option_level)) {
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
		if ('"' == eff_str_copy[len-1-option_level]) {
			eff_str_copy[len-1-option_level] = 0;
		}
	}
	string name = name_begin;
	string value = value_begin;
	if (0==option_level) { //Опции, расположенные на нулевом уровне, добавляем в основной список
		CfgItem *ci = new CfgItem(current_section, name, value);
		items.push_back(ci);
		item_levels.clear(); //Удаляем все уровни вложенности, текущий элемент помещаем на первый уровень
		item_levels.push_back(ci);
	} else { //Все остальные опции добавляем в качестве вложенных к элементу со сложенностью, на единицу меньшей
		if ((int)item_levels.size() < option_level) {
			cout << "CfgReader::parse_option error - wrong option format: " << str << ", required level " << option_level << " while previous level was " << item_levels.size() << endl;
			free(str_copy);
			throw;
		}
		CfgItem *ci_parent = item_levels[option_level-1];
		CfgItem *ci = new CfgItem(ci_parent->name, name, value);
		ci_parent->items.push_back(ci);
		if ((int)item_levels.size()==option_level+1) { //Эта опция располагается на том же уровне, что и предыдущая, соответсвенно, заменяет ее собой. Новые подопции при необходимости будут прикреплены к ней
			item_levels[option_level] = ci;
		} else if ((int)item_levels.size()<option_level+1) { //Это первая подопция предыдущей опции. Добавляем ее
			item_levels.push_back(ci);
		} else { //Опция на уровень выше. Удаляем все что лежит ниже нее
			item_levels.resize(option_level+1);
			item_levels[option_level] = ci;
		}
	}
	free(str_copy);
}




