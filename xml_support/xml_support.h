/*
 * xml_support.h
 *
 *  Created on: 06.08.2011
 *      Author: drozdov
 */

#ifndef XML_SUPPORT_H_
#define XML_SUPPORT_H_

typedef void *xmlConfigPtr;


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

#endif /* XML_SUPPORT_H_ */
