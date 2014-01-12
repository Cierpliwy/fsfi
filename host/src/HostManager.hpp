#ifndef FSFI_HOST_MANAGER_H
#define FSFI_HOST_MANAGER_H
#include <libvirt/libvirt.h>
#include <string>
#include <vector>
#include "Host.hpp"

namespace fsfi
{
class HostManager
{
public:
    HostManager() : m_connection(nullptr) {}

    bool connect(const std::string &url);
    void addHost(const std::string &hostName);

private:
    virConnectPtr m_connection;
    std::vector<Host> m_hosts;
};
}

#endif //FSFI_HOST_MANAGER_H
