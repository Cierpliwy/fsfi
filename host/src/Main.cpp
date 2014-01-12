#include "ScenarioManager.hpp"
#include "HostManager.hpp"
#include <iostream>
#include <libxml/parser.h>
using namespace std;
using namespace fsfi;

int main(int, char *[])
{
    // Initialize XML library
    xmlInitParser();
    LIBXML_TEST_VERSION

    cout << "File System Fault Injector" << endl
         << "--------------------------" << endl;

    // Load scenarios
    ScenarioManager sm;
    sm.loadScenarios();

    cout << "Loaded scenarios:" << endl;
    for(const auto& s : sm.getScenarios()) {
        cout << " - " << s.getName() << endl;
    }

    // Load hosts
    HostManager hm;
    hm.loadHosts();

    cout << "Loaded hosts:" << endl;
    for(const auto& h : hm.getHosts()) {
        cout << " - " << h.second.getName() << endl;
    }

    // Execute scenarios on each host
    cout << "Executing on each host:" << endl;
    hm.connect("qemu:///system");
    hm.executeOnEachHost(sm);

    return 0;
}
