#ifndef FSFI_SCENARIO_MANAGER_H
#define FSFI_SCENARIO_MANAGER_H
#include <vector>
#include <string>
#include "Scenario.hpp"

namespace fsfi 
{
class ScenarioManager
{
public:
    void loadScenarios(const std::string &directoryPath = "scenarios");
    const std::vector<Scenario>& getScenarios() const {
        return m_scenarios;
    }

private:
    std::vector<Scenario> m_scenarios;

};
}

#endif //FSFI_SCENARIO_MANAGER_H
