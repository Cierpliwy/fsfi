#include "Host.hpp"
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <map>
using namespace fsfi;
using namespace std;

bool Host::loadSettingsFromXML(const string &settingsPath)
{
    xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;
    
    doc = xmlParseFile(settingsPath.c_str());
    if (!doc) {
        m_lastError = "Unable to parse XML file";
        return false;
    }

    xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx) {
        m_lastError = "Unable to create XPath context";
        xmlFreeDoc(doc);
        return false;
    }

    // Get values from XML file
    map<string,string> keyValMap;
    keyValMap["/host/name"];
    keyValMap["/snapshot/name"];

    for(auto &keyVal : keyValMap) {
        xpathObj = xmlXPathEvalExpression(
                reinterpret_cast<const xmlChar*>(keyVal.first.c_str()),
                xpathCtx);
        
        if (xpathObj && xpathObj->nodesetval->nodeNr > 0) {
            xmlNodePtr n = xpathObj->nodesetval->nodeTab[0];
            if (n && n->children && n->children->type == XML_TEXT_NODE) {
                keyVal.second = reinterpret_cast<const char*>(
                        n->children->content);
            }
        }

        if (xpathObj) xmlXPathFreeObject(xpathObj);
    }

    // Set values from gathered string values
    m_name = keyValMap["/host/name"];
    m_snapshotName = keyValMap["/snapshot/name"];

    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return true;
    return true;
}
