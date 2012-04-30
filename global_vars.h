/*
 * global_vars.h
 *
 *  Created on: 25.04.2012
 *      Author: drozdov
 */

#ifndef GLOBAL_VARS_H_
#define GLOBAL_VARS_H_

#include "mental_fsm.h"
#include "net_link.h"
#include "trigger_tree.h"

extern MentalFsm* fsm;
extern CNetLink *net_link;
extern CTriggerTree* trigger_tree;

extern std::string executable_path;
extern std::string project_path;

#endif /* GLOBAL_VARS_H_ */
