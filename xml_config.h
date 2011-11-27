#ifndef __XML_CONFIG_H__
#define __XML_CONFIG_H__

#include "libxml/parser.h"
#include "libxml/tree.h"

typedef struct _xmlConfig xmlConfig;
typedef xmlConfig *xmlConfigPtr;

struct _xmlConfig {
	xmlDocPtr doc;
};

xmlConfigPtr CreateXmlConfig(char* xml_file_name);
void DestroyXmlConfig(xmlConfigPtr xml);

int xmlGetIntValue(xmlConfigPtr xml, const char* path, int def);
double xmlGetDoubleValue(xmlConfigPtr xml, const char* path, double def);
char* xmlGetStringValue(xmlConfigPtr xml, const char* path);
int xmlGetBooleanValue(xmlConfigPtr xml, const char* path, int def);
int xmlGetDelayUsValue(xmlConfigPtr xml, const char* path, int def);
int xmlValidatePath(xmlConfigPtr xml, const char* path);


#endif
