#include "ScenarioManager.hpp"
#include <iostream>
#include <libxml/parser.h>
using namespace std;
using namespace fsfi;

int main(int, char *[])
{
    // Initialize XML library
    xmlInitParser();
    LIBXML_TEST_VERSION

    // Load scenarios
    ScenarioManager sm;
    sm.loadScenarios();

    cout << "Loaded scenarios" << endl;
    for(const auto& s : sm.getScenarios()) {
        cout << s.getName() << endl;
    }

    return 0;
}
