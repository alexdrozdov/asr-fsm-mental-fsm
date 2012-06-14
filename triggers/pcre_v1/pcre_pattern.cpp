/*
 * pcre_pattern.cpp
 *
 *  Created on: 03.12.2011
 *      Author: drozdov
 */

#include <iostream>
#include <sstream>
#include <vector>

#include <pcre.h>

#include "pcre_trigger.h"
#include "xml_support.h"

using namespace std;

CPcrePattern::CPcrePattern(CPcreTrigger *trigger, int ntemplate) {
	xmlConfigPtr xml = (xmlConfigPtr)trigger->pxml_tmp;
	this->trigger = trigger;

	ostringstream ostr_base_path;
	ostr_base_path << "/trigger/patterns/i" << ntemplate;
	string base_path = ostr_base_path.str();

	pattern = xmlGetStringValue(xml, (base_path+"/pattern").c_str());
	const char *error = NULL;
	int   erroffset = 0;
	pcre *re = pcre_compile (pattern.c_str(),          /* the pattern */
					   0, //PCRE_MULTILINE
					   &error,         /* for error message */
					   &erroffset,     /* for error offset */
					   0);             /* use default character tables */
	if (NULL == re) {
		cout << "CPcrePattern::CPcrePattern error - не удалось скомпилировать регулярное выражение " << pattern << " (offset: " << erroffset <<"), " << error << endl;
		return;
	}

	ppcre = (void*)re;

	int nout_changes = xmlGetIntValue(xml, (base_path+"/output_links/count").c_str(),-1);
	if (nout_changes < 1) {
		cout << "CPcrePattern::CPcrePattern error - не задано количество изменяемых выходов в " << base_path << endl;
		return;
	}
	output_changes.resize(nout_changes);

	//Загружаем список изменяемых выходов
	for (int i=0;i<nout_changes;i++) {
		ostringstream ostr;
		ostr << base_path << "/output_links/i" << i;
		output_changes[i].out = xmlGetIntValue(xml, (ostr.str()+"/out").c_str(),-1);
		output_changes[i].new_value = xmlGetDoubleValue(xml, (ostr.str()+"/value").c_str(),-1);
		if (output_changes[i].out < 0)
			cout << "CPcrePattern::CPcrePattern error - неправильно задан номер выхода для " << ostr.str() << endl;
	}
}

int CPcrePattern::ProcessString(CPcreStringQueue* strq) {
	pcre *re = (pcre*)ppcre;
	int   ovector[99] = {0};
	string str = strq->str();
	int rc = pcre_exec(re, 0, str.c_str(), str.length(), 0, 0, ovector, 99);
	if (rc < 1)
		return 0; //Искомый шаблон не найден
	//cout << "Найдено " << pattern << " rc=" << rc << " " << ovector[0] << " " << ovector[1] << " " << endl; //FIXME Обесечить выбор уровня детализации

	//Шаблон найден. Копируем соответсвующие ему выходы
	for (vector<out_change>::iterator it=output_changes.begin();it!=output_changes.end();it++)
		trigger->values[it->out] = it->new_value;
	return ovector[1];
}
