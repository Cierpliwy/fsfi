#include "HostManager.hpp"
using namespace fsfi;
using namespace std;

bool HostManager::connect(const string &url)
{
    m_connection = virConnectOpen(url.c_str());
    if (m_connection) return true;
    return false;
}
