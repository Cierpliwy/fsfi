#include "HostManager.hpp"
#include "ScenarioManager.hpp"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

    closedir(hostsDir);
}

void HostManager::executeOnEachHost(const ScenarioManager &scenarioManager)
{
    auto &scenarios = scenarioManager.getScenarios();

    //TODO Handle each host in separate thread
    for (auto &h : m_hosts) {

        //Create folder for a host
        struct stat dirStat;
        if (stat(".", &dirStat) == -1) {
            cout << "Error: stat(): " << strerror(errno) << endl;
            break;
        }

        time_t t = time(nullptr);
        tm *now = localtime(&t);
        string folderName = h.second.getName();
        folderName += "_";
        folderName += to_string(now->tm_mday);
        folderName += "_";
        folderName += to_string(now->tm_mon + 1);
        folderName += "_";
        folderName += to_string(now->tm_year + 1900);
        folderName += "_";
        folderName += to_string(now->tm_hour);
        folderName += "_";
        folderName += to_string(now->tm_min);
        folderName += "_";
        folderName += to_string(now->tm_sec);

        if (mkdir(folderName.c_str(), dirStat.st_mode) == -1) {
            cout << "Error: mkdir(): " << strerror(errno) << endl;
            break;
        }

        for (auto &s : scenarios) {
            string scenarioFolderName = folderName;
            scenarioFolderName += '/';
            scenarioFolderName += s.getName();
            
            if (mkdir(scenarioFolderName.c_str(), dirStat.st_mode) == -1) {
                cout << "Error: mkdir(): " << strerror(errno) << endl;
                break;
            }

            if (!h.second.executeScenario(s, m_connection, scenarioFolderName)) {
                cout << "'" << s.getName() << "' execution aborted by "
                     << h.second.getName() << ": "
                     << h.second.getLastError() << endl;
            }
        }
    }
}
