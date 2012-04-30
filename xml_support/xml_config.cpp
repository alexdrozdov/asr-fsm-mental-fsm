#include <stdlib.h>
#include <string.h>
#include "xml_config.h"

//загружает конфигурационный xml-файл
xmlConfigPtr CreateXmlConfig(char* xml_file_name) {
        xmlParserCtxtPtr ctxt;
        char chars[512];
        xmlDocPtr doc; /* the resulting document tree */
        int res;
        FILE* desc;

        if (NULL == xml_file_name) {
                fprintf(stderr, "Null ptr to configuration file name\n");
                return NULL;
        }
        desc = fopen(xml_file_name, "rb");
        if (NULL == desc) {
                fprintf(stderr, "Couldn`t open config file\n");
                return NULL;
        }

        res = fread(chars,1,4,desc);
        if (res <= 0) {
                fclose(desc);
                fprintf(stderr, "Failed to parse %s\n", xml_file_name);
                return NULL;
        }

        ctxt = xmlCreatePushParserCtxt(NULL, NULL,
                        chars, res, xml_file_name);
        if (ctxt == NULL) {
                fclose(desc);
                fprintf(stderr, "Failed to create parser context !\n");
                return NULL;
        }

        /*
         * loop on the input getting the document data, of course 4 bytes
         * at a time is not realistic but allows to verify testing on small
         * documents.
         */
        while ((res = fread(chars,1,512,desc)) > 0) {
                xmlParseChunk(ctxt, chars, res, 0);
        }
        fclose(desc);
        /*
         * there is no more input, indicate the parsing is finished.
         */
        xmlParseChunk(ctxt, chars, 0, 1);

        /*
         * collect the document back and if it was wellformed
         * and destroy the parser context.
         */
        doc = ctxt->myDoc;
        res = ctxt->wellFormed;
        xmlFreeParserCtxt(ctxt);

        if (!res) {
                fprintf(stderr, "Failed to parse %s\n", xml_file_name);
                xmlFreeDoc(doc);
                return NULL;
        }

        xmlConfigPtr pXmlConf = (xmlConfigPtr)malloc(sizeof(xmlConfig));
        pXmlConf->doc = doc;

        return pXmlConf;
}

xmlNodePtr xmlGetNodeByName(xmlNodePtr parentNode, const char* szNodeName) {
        xmlNodePtr childPtr = parentNode->children;
        //printf("Search in node: %s\n",parentNode->name);
        while (NULL != childPtr) {
                //printf("\tChild: %s\n",childPtr->name);
                if (0==strcmp((char*)childPtr->name,szNodeName)) {
                        return childPtr;
                }
                childPtr = childPtr->next;
        }
        return NULL;
}

xmlAttrPtr xmlGetAttributeByName(xmlNodePtr parentNode, const char* szAttrName) {
        xmlAttrPtr attrPtr = parentNode->properties;
        //printf("Search in node: %s\n",parentNode->name);
        while (NULL != attrPtr) {
                //printf("\tChild: %s\n",attrPtr->name);
                if (0==strcmp((char*)attrPtr->name,szAttrName)) {
                        return attrPtr;
                }
                attrPtr = attrPtr->next;
        }
        return NULL;
}

int SplitPathToParts(const char* szPath, char ***szParts, int* nParts) {

        int nPathLen = strlen(szPath);
        int i = 0;
        int nSplitCnt = 0;
        int nPartsCount = 0;
        int nPartLen = 0;
        int nPartStart = 0;

        //Определяем количество частей в строке
        for (i=0;i<nPathLen;i++) {
                if ('/' == szPath[i]) {
                        nPartsCount++;
                }
        }
        char **szPartsIn = (char**)malloc(sizeof(char*)*nPartsCount);
        nSplitCnt = 0;
        nPartLen = 0;
        nPartStart = 1;
        for (i=1;i<nPathLen;i++) {
                if ('/' == szPath[i]){
                        char* szCurPart = (char*)malloc(nPartLen+1);
                        strncpy(szCurPart,szPath+nPartStart,nPartLen);
                        szCurPart[nPartLen] = 0;
                        szPartsIn[nSplitCnt] = szCurPart;
                        nSplitCnt++;
                        nPartLen = 0;
                        nPartStart = i + 1;
                } else {
                        nPartLen++;
                }
        }

        //По идее, в конце должен остаться один фрагмент пути.
        if (nPartLen > 0) {
                char* szCurPart = (char*)malloc(nPartLen+1);
                strncpy(szCurPart,szPath+nPartStart,nPartLen);
                szCurPart[nPartLen] = 0;
                szPartsIn[nSplitCnt] = szCurPart;
        }

        *nParts = nPartsCount;
        *szParts = szPartsIn;
        return nPartsCount;
}

//Читает строку из загруженного xml-файла
char* xmlGetStringValue(xmlConfigPtr xml, const char* path) {
        if (NULL == xml) {
                fprintf(stderr,"xmlGetIntegerValue: Null xml config supplied as parameter\n");
                return NULL;
        }

        int nParts = 0;
        char **szParts;
        SplitPathToParts(path, &szParts, &nParts);
        if (nParts < 1) {
                fprintf(stderr,"xmlGetIntegerValue: Couldn`t separate path to subparts\n");
                return NULL;
        }

        //Сначала ищем путь по узлам (до предпоследнего элемента в пути)
        int nPartCnt = 0;
        int bError = 0;
        xmlNodePtr parentNode = xml->doc->children;
        if (nParts > 2) {
                for (nPartCnt=1;nPartCnt<nParts-1;nPartCnt++) {
                        //fprintf(stderr, "Part number %d\n",nPartCnt);
                        char* szPart = szParts[nPartCnt];
                        //fprintf(stderr,"\tszPart %s\n",szPart);

                        parentNode = xmlGetNodeByName(parentNode, szPart);
                        if (NULL == parentNode) {
                                bError = 1;
                                break;
                        }
                }
        }

        char* szPathValue = NULL;
        //Теперь ищем последний элемент, который может быть как узлом, так и свойством
        if (!bError) {
                bError = 1;
                char* szPart = szParts[nParts-1];
                xmlAttrPtr lastAttr = xmlGetAttributeByName(parentNode, szPart);
                if (NULL != lastAttr && NULL !=lastAttr->children) {
                        szPathValue = (char*)lastAttr->children->content;
                        bError = 0;
                } else {
                        xmlNodePtr lastNode = xmlGetNodeByName(parentNode, szPart);
                        if (NULL != lastNode && NULL != lastNode->children) {
                                szPathValue = (char*)lastNode->children->content;
                                bError = 0;
                        }
                }
        }

        //Очищаем все фрагменты пути и временный массив путей
        for (nPartCnt=0;nPartCnt<nParts;nPartCnt++) {
                free(szParts[nPartCnt]);
        }
        free(szParts);
        if (bError) {
                fprintf(stderr,"xmlGetStringValue: Couldn`t find required path %s\n",path);
                return NULL;
        }

        if (NULL == szPathValue) {
                fprintf(stderr,"xmlGetStringValue: Null value on path %s\n",path);
        }
        return szPathValue;
}

char* xmlGetStringValueSafe(xmlConfigPtr xml, const char* path, const char* def) {
        if (xmlValidatePath(xml, path)) {
                return xmlGetStringValue(xml, path);
        }
        return (char*)def;
}

int xmlGetIntValue(xmlConfigPtr xml, const char* path, int def) {
        char* szVal = xmlGetStringValue(xml,path);
        if (NULL == szVal) {
                return def;
        }
        return atoi(szVal);
}

double xmlGetDoubleValue(xmlConfigPtr xml, const char* path, double def) {
        char* szVal = xmlGetStringValue(xml,path);
        if (NULL == szVal) {
                return def;
        }
        return atof(szVal);
}

int xmlGetBooleanValue(xmlConfigPtr xml, const char* path, int def) {
        char* szVal = xmlGetStringValue(xml,path);
        if (NULL == szVal) {
                return def;
        }
        if (strcmp(szVal,"true")==0) {
                return 1;
        }
        return atoi(szVal);
}

int xmlGetDelayUsValue(xmlConfigPtr xml, const char* path, int def) {
        char* szVal = xmlGetStringValue(xml,path);
        if (NULL == szVal) {
                return def;
        }
        int orig_val = atoi(szVal);

        if (NULL != strstr(szVal,"ms"))
                return orig_val * 1000;
        if (NULL != strstr(szVal,"us"))
                return orig_val;
        if (NULL != strstr(szVal,"ns"))
                return orig_val / 1000;
        if (NULL != strstr(szVal,"s"))
                return orig_val * 1000000;
        if (0 == orig_val)
                return 0;

        fprintf(stderr,"xmlGetDelayUsValue warning - не указана размерность времени для %s\n", path);
        return orig_val;
}

int xmlValidatePath(xmlConfigPtr xml, const char* path) {
        char *szVal = xmlGetStringValue(xml, path);
        if (NULL == szVal) {
                return 0;
        }

        return 1;
}

