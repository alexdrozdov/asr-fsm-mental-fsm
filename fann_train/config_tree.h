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

class ConfigItem {
public:
	std::string name;    //Название, по которому выполняется доступ к узлу
	std::string locname; //Локализованное название узла
};


class ConfigTree {
public:
};


#endif /* CONFIG_TREE_H_ */
