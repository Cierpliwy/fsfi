#include "Host.hpp"
#include "Scenario.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libvirt/virterror.h>
#include <map>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

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
    keyValMap["/host/snapshot/path"];
    keyValMap["/host/disk/path"];
    keyValMap["/host/disk/save"];
    keyValMap["/host/ip/address"];
    keyValMap["/host/ip/port"];
    keyValMap["/host/logs/kernel"];
    keyValMap["/host/logs/program"];

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
    m_snapshotPath = keyValMap["/host/snapshot/path"];
    m_diskPath = keyValMap["/host/disk/path"];
    m_saveDisk = keyValMap["/host/disk/save"].compare("yes") == 0;
    m_address = keyValMap["/host/ip/address"];
    m_kernelLogPath = keyValMap["/host/logs/kernel"];
    m_programLogPath = keyValMap["/host/logs/program"];

    try {
        m_portNr = stoi(keyValMap["/host/ip/port"]);
    } catch (exception &e) {
        m_lastError = "Value(s) are in incorrect format or out of range";
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return false;
    }

    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return true;
}

bool Host::executeScenario(const Scenario &scenario, virConnectPtr conn)
{
    if (!setupSocket()) return false;
    unsigned long iterations = 0;
    bool finished = false;
    chrono::high_resolution_clock clk;
    auto executionTime = clk.now();
    
    while(!finished) {
        auto injectionTime = clk.now();
        iterations++;
        cout << "[" << m_name << "/" << scenario.getName() << "/"
             << iterations << "/";
        cout.flush();

        InjectionResult res = executeInjection(scenario, conn);
        if (res == InjectionResult::FAILURE) return false;

        cout << chrono::duration_cast<chrono::milliseconds>
                (clk.now() - injectionTime).count() << "ms";

        string resultString;
        switch(res) {
            case InjectionResult::NOT_STARTED: resultString = "NOT STARTED";
                break;
            case InjectionResult::OK: resultString = "OK";
                break;
            case InjectionResult::TIMEOUT: resultString = "TIMEOUT";
                break;
            case InjectionResult::FAILURE: resultString = "FAILURE";
                break;
            case InjectionResult::ERROR: resultString = "ERROR";
                break;
        };

        cout << "/" <<resultString;

        // Check constraints
        if (scenario.getConstraints().maxIterations > 0 &&
            scenario.getConstraints().maxIterations <= iterations) {
            finished = true;
            cout << "/FINAL ITERATION";
        }
        if (scenario.getConstraints().maxSeconds.count() > 0 &&
            scenario.getConstraints().maxSeconds.count() <=
            chrono::duration_cast<chrono::seconds>
            (clk.now()-executionTime).count()) {
            finished = true;
            cout << "/ITERATION TIMEOUTED";
        }

        cout << ']' << endl;
    }

    return true;
}

Host::InjectionResult Host::executeInjection(const Scenario& scenario,
                                             virConnectPtr conn)
{

    string msg;
    msg += static_cast<char>(0);
    msg += static_cast<char>(scenario.getRetries());
    msg += scenario.getCommand();
    msg += '\a';
    msg[0] = static_cast<char>(msg.length());

    // Connect to virtual host
    virDomainPtr dom = virDomainLookupByName(conn, m_name.c_str());
    if (!dom) {
        m_lastError = "Cannot find a domain: ";
        m_lastError += virGetLastErrorMessage();
        return InjectionResult::FAILURE;
    }

    // Destroy domain
    int state;
    if (virDomainGetState(dom, &state, nullptr, 0) == -1) {
        m_lastError = "Cannot get a domain state: ";
        m_lastError += virGetLastErrorMessage();
        return InjectionResult::FAILURE;
    }
    if (state != VIR_DOMAIN_SHUTOFF) {
        if (virDomainDestroy(dom) == -1) {
            m_lastError = "Cannot stop a domain: ";
            m_lastError += virGetLastErrorMessage();
            return InjectionResult::FAILURE;
        }
    }

    // Restore domain to specified snapshot
    if (virDomainRestore(conn, m_snapshotPath.c_str()) == -1) {
        m_lastError = "Cannot restore a domain: ";
        m_lastError += virGetLastErrorMessage();
        return InjectionResult::FAILURE;
    }

    // If domain paused, resume it
    if (virDomainGetState(dom, &state, nullptr, 0) == -1) {
        m_lastError = "Cannot get a domain state: ";
        m_lastError += virGetLastErrorMessage();
        return InjectionResult::FAILURE;
    }
    if (state & VIR_DOMAIN_PAUSED) {
        if (virDomainResume(dom) == -1) {
            m_lastError = "Cannot resume a domain: ";
            m_lastError += virGetLastErrorMessage();
            return InjectionResult::FAILURE;
        }
    }

    // Send data until we got START signal
    bool started = false;
    unsigned int startTimeoutMs = 0;
    const unsigned int retryTimeoutMs = 200;
    const unsigned int maxStartTimeoutMs = 5000;

    while(!started) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(m_socket, &set);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000*retryTimeoutMs;

        sendData(msg);
        int val = select(m_socket+1, &set, nullptr, nullptr, &timeout);
        if (val == -1) {
            m_lastError = "Error during select: ";
            m_lastError += strerror(errno);
            return InjectionResult::FAILURE;
        }
        if (FD_ISSET(m_socket, &set)) {
            char d = getData();
            if (d == 'S') started = true;
        } else {
            startTimeoutMs += retryTimeoutMs;
            if (startTimeoutMs >= maxStartTimeoutMs) {
                m_lastError = "Couldn't start scenario";
                return InjectionResult::NOT_STARTED;
            }
        }
    }

    // Wait for result
    const unsigned int resultTimeoutMs = 5000;
    fd_set set;
    FD_ZERO(&set);
    FD_SET(m_socket, &set);

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000*resultTimeoutMs;

    int val = select(m_socket+1, &set, nullptr, nullptr, &timeout);
    if (val == -1) {
        m_lastError = "Error during select: ";
        m_lastError += strerror(errno);
        return InjectionResult::FAILURE;
    }
    if (FD_ISSET(m_socket, &set)) {
        char d = getData();
        if (d == 'F') return InjectionResult::OK;
        if (d == 'E') return InjectionResult::ERROR;
    }

    return InjectionResult::TIMEOUT;
}

bool Host::setupSocket()
{
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket == -1) {
        m_lastError = "Cannot create socket: ";
        m_lastError += strerror(errno);
        return false;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);

    if (bind(m_socket, reinterpret_cast<const struct sockaddr*>(&addr),
             sizeof(addr)) == -1) {
        m_lastError = "Cannot bound socket to port: ";
        m_lastError += strerror(errno);
        return false;
    }

    return true;
}

bool Host::sendData(const std::string &msg)
{
    ssize_t size;
    sockaddr_in server;
    socklen_t len = sizeof(server);
    memset(&server, 0, len); 
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(m_address.c_str());
    server.sin_port = htons(m_portNr);
    size = sendto(m_socket, msg.c_str(), msg.length(), 0, 
                  reinterpret_cast<const struct sockaddr*>(&server), len);
   
    if (size == -1) {
        m_lastError = "Cannot send data to host: ";
        m_lastError += strerror(errno);
        return false;
    }

    return true;
}

char Host::getData()
{
    char b;

    if (recvfrom(m_socket, &b, 1, 0, nullptr, nullptr) == -1) {
        cout << "WARNING: recvfrom(): " << strerror(errno) << endl;
    }

    return b;
}
