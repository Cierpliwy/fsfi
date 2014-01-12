#ifndef FSFI_HOST_MANAGER_H
#define FSFI_HOST_MANAGER_H
#include <libvirt/libvirt.h>
#include <string>
#include <map>
#include "Host.hpp"

namespace fsfi
{
class ScenarioManager;

class HostManager
{
public:
    HostManager() : m_connection(nullptr) {}
    ~HostManager();
    
    bool connect(const std::string &url);
    void loadHosts(const std::string &directoryPath = "hosts");

    void executeOnEachHost(const ScenarioManager& scenarioManager);

    const std::map<std::string, Host>& getHosts() const {
        return m_hosts;
    }

private:
    virConnectPtr m_connection;
    std::map<std::string, Host> m_hosts;
};
}

#endif //FSFI_HOST_MANAGER_H
