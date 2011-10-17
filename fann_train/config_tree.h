/*
 * config_tree.h
 *
 *  Created on: 10.10.2011
 *      Author: drozdov
 */

#ifndef CONFIG_TREE_H_
#define CONFIG_TREE_H_

#include <string>
#include <vector>
#include <map>


class ConfigItem;
class ConfigSubitems;


class ConfigSubitems {
public:
	ConfigSubitems();
	ConfigItem* operator[](int n);
	ConfigItem* operator[](std::string);

	ConfigItem* AddItem(ConfigItem* itm);
	void DeleteItem(ConfigItem* itm);
	void DeleteItem(std::string name);
};

class ConfigItem {
public:
	std::string name;    //Название, по которому выполняется доступ к узлу
	std::string locname; //Локализованное название узла

	ConfigSubitems subitems;

	void Mount(ConfigItem* itm);

	union {
		std::string val_str;
		int val_int32;
		unsigned int val_uint32;
	};
};


class ConfigTree {
public:
};


#endif /* CONFIG_TREE_H_ */
