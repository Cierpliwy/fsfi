#include "ScenarioManager.hpp"
#include <iostream>
#include <dirent.h>
#include <sys/types.h>
using namespace fsfi;
using namespace std;

void ScenarioManager::loadScenarios(const string &directoryPath)
{
    DIR *scenariosDir = opendir(directoryPath.c_str());
    if (!scenariosDir) return;

    dirent *scenario;
    while((scenario = readdir(scenariosDir))) {

        string str(directoryPath);
        str += '/';
        str += scenario->d_name;

        if (str.length() >= 5 && 
            str.substr(str.length()-4).compare(".xml") != 0)
            continue;

        Scenario s;
        if (!s.loadFromXML(str)) {
            cout << "ScenarioManage> cannot load '"
                 << str << "' from XML file: " << s.getLastError() << endl;
        } else if (!s.isCorrect()) {
            cout << "ScenarioManager> loaded '"
                 << str << "' scenario is not correct: " << s.getLastError()
                 << endl;
        } else {
            vector<Scenario>::size_type i;
            for(i = 0; i < m_scenarios.size(); ++i) {
                if (m_scenarios[i].getName().compare(s.getName()) == 0) {
                    cout << "ScenarioManager> '"
                         << str << "' scenario's name '"
                         << s.getName() << "' already exists" << endl;
                    break;
                }
            }

            if (i == m_scenarios.size()) m_scenarios.push_back(s);
        }
    }
}
