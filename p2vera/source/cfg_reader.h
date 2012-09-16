/*
 * cfg_reader.h
 *
 *  Created on: 09.08.2012
 *      Author: drozdov
 */

#ifndef CFG_READER_H_
#define CFG_READER_H_

#include <string>
#include <list>
#include <vector>

class CfgReaderIterator;
class CfgReader;
class CfgItem;
typedef std::list<CfgItem*> cfg_list;
typedef cfg_list::iterator cfg_item_iter;

class ICfgItemList {
public:
	virtual ~ICfgItemList() = 0;
	virtual cfg_item_iter end() = 0;
	virtual cfg_item_iter begin() = 0;
};

class CfgItem : public ICfgItemList {
public:
	CfgItem(std::string section, std::string name, std::string value);
	virtual ~CfgItem();
	virtual operator int();
	virtual operator bool();
	virtual operator std::string();

	virtual std::string to_string() const;
	virtual int to_int() const;
	virtual bool to_bool() const;

	virtual CfgReaderIterator get_cfg_items(std::string name);
	virtual cfg_item_iter end();
	virtual cfg_item_iter begin();

	friend class CfgReaderIterator;
	friend class CfgReader;
private:
	std::string value;
	std::string section;
	std::string name;

	cfg_list items;
};

class CfgReaderIterator {
public:
	CfgReaderIterator(std::string section, std::string name, cfg_item_iter cii, ICfgItemList* cr);
	CfgReaderIterator(const CfgReaderIterator& cri);
	virtual ~CfgReaderIterator();
	CfgReaderIterator& operator=(CfgReaderIterator& cri);
	CfgReaderIterator& operator++();
	CfgReaderIterator& operator++(int);
	CfgItem* operator->();
	friend bool operator==(CfgReaderIterator& lh, CfgReaderIterator& rh);
	friend bool operator==(CfgReaderIterator& lh, cfg_item_iter& rh);
	friend bool operator==(cfg_item_iter& lh, CfgReaderIterator& rh);
	friend bool operator!=(const CfgReaderIterator lh, const CfgReaderIterator rh);
	friend bool operator!=(const CfgReaderIterator lh, cfg_item_iter rh);
	friend bool operator!=(cfg_item_iter lh, const CfgReaderIterator rh);
private:
	std::string section;
	std::string name;
	cfg_item_iter cur_cii;
	ICfgItemList* cr;
};

bool operator==(CfgReaderIterator& lh, CfgReaderIterator& rh);
bool operator==(CfgReaderIterator& lh, cfg_item_iter& rh);
bool operator==(cfg_item_iter& lh, CfgReaderIterator& rh);
bool operator!=(const CfgReaderIterator lh, const CfgReaderIterator rh);
bool operator!=(const CfgReaderIterator lh, cfg_item_iter rh);
bool operator!=(cfg_item_iter lh, const CfgReaderIterator rh);

class CfgReader : public ICfgItemList {
public:
	CfgReader(std::string cfg_file);
	virtual ~CfgReader();
	virtual CfgReaderIterator get_cfg_items(std::string section, std::string name);
	virtual bool has_item(std::string section, std::string name);
	virtual int item_count(std::string section, std::string name);
	virtual std::string get_item_as_string(std::string section, std::string name, std::string def_value);
	virtual int get_item_as_int(std::string section, std::string name, int def_value);
	virtual bool get_item_as_bool(std::string section, std::string name, bool def_value);
	virtual cfg_item_iter end();
	virtual cfg_item_iter begin();

	friend class CfgReaderIterator;
private:
	cfg_list items;
	std::vector<CfgItem*> item_levels; //Массив, представляющий собой текущие уровни вложенности опций
	std::string current_section;

	virtual int get_eff_line_length(const char *str, int len);
	virtual void parse_section(const char *str, int len);
	virtual void parse_option(const char *str, int len);
};

#endif /* CFG_READER_H_ */
