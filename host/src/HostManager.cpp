#include "HostManager.hpp"
#include "ScenarioManager.hpp"
#include <iostream>
#include <dirent.h>
#include <sys/types.h>
using namespace fsfi;
using namespace std;

HostManager::~HostManager()
{
    if (m_connection) {
        virConnectClose(m_connection);
    }
}

bool HostManager::connect(const string &url)
{
    m_connection = virConnectOpen(url.c_str());
    if (m_connection) return true;
    return false;
}

void HostManager::loadHosts(const string &directoryPath)
{
    DIR *hostsDir= opendir(directoryPath.c_str());
    if (!hostsDir) return;

    dirent *scenario;
    while((scenario = readdir(hostsDir))) {

        string str(directoryPath);
        str += '/';
        str += scenario->d_name;

        if (str.length() >= 5 && 
            str.substr(str.length()-4).compare(".xml") != 0)
            continue;

        Host h;
        if (!h.loadSettingsFromXML(str)) {
            cout << "ScenarioManager> cannot load '"
                 << str << "' from XML file: " << h.getLastError() << endl;
        } else {
            if (m_hosts.find(h.getName()) == m_hosts.end())
                m_hosts[h.getName()] = h;
            else {
                cout << "ScenarioManager> '"
                     << str << "' host's name '"
                     << h.getName() << "' already exists" << endl;
            }
        }
    }
}

void HostManager::executeOnEachHost(const ScenarioManager &scenarioManager)
{
    auto &scenarios = scenarioManager.getScenarios();

    //TODO Handle each host in separate thread
    for (auto &h : m_hosts) {
        for (auto &s : scenarios) {
            if (!h.second.executeScenario(s, m_connection)) {
                cout << "'" << s.getName() << "' execution aborted by "
                     << h.second.getName() << ": "
                     << h.second.getLastError() << endl;
            }
        }
    }
}
