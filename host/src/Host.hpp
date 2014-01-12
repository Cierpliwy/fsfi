#ifndef FSFI_HOST_HPP
#define FSFI_HOST_HPP
#include <string>

namespace fsfi 
{
class Host
{
public:
    bool loadSettingsFromXML(const std::string &settingsPath);


private:
    mutable std::string m_lastError;
    std::string m_name;
    std::string m_snapshotName;
    std::string m_diskName;
};
}

#endif //FSFI_HOST_HPP
