#ifndef __XML_CONFIG_H__
#define __XML_CONFIG_H__

#include "libxml/parser.h"
#include "libxml/tree.h"

typedef struct _xmlConfig xmlConfig;
typedef xmlConfig *xmlConfigPtr;

struct _xmlConfig {
	xmlDocPtr doc;
};

#ifdef __cplusplus
extern "C" {
#endif

extern xmlConfigPtr CreateXmlConfig(char* xml_file_name);
extern void DestroyXmlConfig(xmlConfigPtr xml);

extern int xmlGetIntValue(xmlConfigPtr xml, const char* path, int def);
extern double xmlGetDoubleValue(xmlConfigPtr xml, const char* path, double def);
extern char* xmlGetStringValue(xmlConfigPtr xml, const char* path);
extern char* xmlGetStringValueSafe(xmlConfigPtr xml, const char* path, const char* def);
extern int xmlGetBooleanValue(xmlConfigPtr xml, const char* path, int def);
extern int xmlGetDelayUsValue(xmlConfigPtr xml, const char* path, int def);
extern int xmlValidatePath(xmlConfigPtr xml, const char* path);

#ifdef __cplusplus
}
#endif

#endif
