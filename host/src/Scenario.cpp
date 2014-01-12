#include "Scenario.hpp"
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <iostream>
#include <map>
using namespace std;
using namespace fsfi;

bool Scenario::isCorrect() const
{
    if (m_name.empty()) {
        m_lastError = "Name of scenario cannot be empty";
        return false;
    }

    if (m_command.empty()) {
        m_lastError = "Command for injector has to be specified";
        return false;
    }

    if (m_constraints.maxSeconds.count() == 0 &&
        m_constraints.maxIterations == 0 &&
        m_constraints.maxSizeInBytes == 0) {
        m_lastError = "At least one of limits must be specified";
        return false;
    }

    return true;
}

bool Scenario::loadFromXML(const string& filePath)
{
    xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;
    
    doc = xmlParseFile(filePath.c_str());
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
    keyValMap["/scenario/name"];
    keyValMap["/scenario/description"];
    keyValMap["/scenario/limits/iterations"];
    keyValMap["/scenario/limits/time"];
    keyValMap["/scenario/limits/disk"];
    keyValMap["/scenario/command"];
    keyValMap["/scenario/retries"];

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
    m_name = keyValMap["/scenario/name"];
    m_description = keyValMap["/scenario/description"];
    m_command = keyValMap["/scenario/command"];

    try {
        m_constraints.maxIterations = 
            stoul(keyValMap["/scenario/limits/iterations"]);
        m_constraints.maxSeconds = 
            chrono::seconds(stoul(keyValMap["/scenario/limits/time"]));
        m_constraints.maxSizeInBytes =
            stoul(keyValMap["/scenario/limits/disk"]);
        m_retries = stoi(keyValMap["/scenario/retries"]);
    } catch(std::exception &inv) {
        m_lastError = "Value(s) are in incorrect format or out of range";
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return false;
    }

    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return true;
}
