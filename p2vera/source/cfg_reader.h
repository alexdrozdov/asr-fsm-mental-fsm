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

class CfgReaderIterator;
class CfgReader;

class CfgItem {
public:
	CfgItem(std::string section, std::string name, std::string value);
	//std::string std::string();
	operator int();
	operator bool();

	friend class CfgReaderIterator;
	friend class CfgReader;
private:
	std::string value;
	std::string section;
	std::string name;
};

typedef std::list<CfgItem> cfg_list;
typedef cfg_list::const_iterator cfg_item_iter;

class CfgReaderIterator {
public:
	CfgReaderIterator(std::string section, std::string name, cfg_item_iter cii, CfgReader* cr);
	CfgReaderIterator(CfgReaderIterator& cri);
	virtual ~CfgReaderIterator();
	CfgReaderIterator& operator=(CfgReaderIterator& cri);
	CfgReaderIterator& operator++();
	const CfgItem& operator->();
	friend bool operator==(CfgReaderIterator& lh, CfgReaderIterator& rh);
	friend bool operator==(CfgReaderIterator& lh, cfg_item_iter& rh);
	friend bool operator==(cfg_item_iter& lh, CfgReaderIterator& rh);
	friend bool operator!=(CfgReaderIterator& lh, CfgReaderIterator& rh);
	friend bool operator!=(CfgReaderIterator& lh, cfg_item_iter& rh);
	friend bool operator!=(cfg_item_iter& lh, CfgReaderIterator& rh);
private:
	std::string section;
	std::string name;
	cfg_item_iter cur_cii;
	CfgReader* cr;
};

bool operator==(CfgReaderIterator& lh, CfgReaderIterator& rh);
bool operator==(CfgReaderIterator& lh, cfg_item_iter& rh);
bool operator==(cfg_item_iter& lh, CfgReaderIterator& rh);
bool operator!=(CfgReaderIterator& lh, CfgReaderIterator& rh);
bool operator!=(CfgReaderIterator& lh, cfg_item_iter& rh);
bool operator!=(cfg_item_iter& lh, CfgReaderIterator& rh);

class CfgReader {
public:
	CfgReader(std::string cfg_file);
	virtual ~CfgReader();
	CfgReaderIterator get_cfg_items(std::string section, std::string name);
	cfg_item_iter end();
	cfg_item_iter begin();

	friend class CfgReaderIterator;
private:
	cfg_list items;
};

#endif /* CFG_READER_H_ */
