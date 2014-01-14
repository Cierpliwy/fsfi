#ifndef FSFI_HOST_HPP
#define FSFI_HOST_HPP
#include <libvirt/libvirt.h>
#include <string>
#include <random>

namespace fsfi 
{
class Scenario;

class Host
{
public:
    bool loadSettingsFromXML(const std::string &settingsPath);

    bool executeScenario(const Scenario& scenario, virConnectPtr conn,
                         const std::string &rootFolder);

    const std::string& getLastError() const {
        return m_lastError;
    }

    const std::string& getName() const {
        return m_name;
    }
    void setName(const std::string &name) {
        m_name = name;
    }

    const std::string& getSnapshotPath() const {
        return m_snapshotPath;
    }
    void setSnapshotPath(const std::string &snapshotPath) {
        m_snapshotPath = snapshotPath;
    }

    const std::string& gestDiskPath() const {
        return m_diskPath;
    }
    void setDiskPath(const std::string &path) {
        m_diskPath = path;
    }

    bool isSavedDisk() const {
        return m_saveDisk;
    }
    void setSaveDisk(bool isSaved) {
        m_saveDisk = isSaved;
    }

    const std::string& getAddress() const {
        return m_address;
    }
    void setAddress(const std::string &address) {
        m_address = address;
    }

    unsigned int getPortNr() const {
        return m_portNr;
    }
    void setPortNr(unsigned int portNr) {
        m_portNr = portNr;
    }

    const std::string& getKernelLogPath() const {
        return m_kernelLogPath;
    }
    void setKernelLogPath(const std::string &kernelLogPath) {
        m_kernelLogPath = kernelLogPath;
    }

    const std::string& getProgramLogPath() const {
        return m_programLogPath;
    }
    void setProgramLogPath(const std::string &programLogPath) {
        m_programLogPath = programLogPath;
    }

private:

    bool setupSocket();
    void closeSocket();
    bool sendData(const std::string &msg);
    char getData();

    enum class InjectionResult {
        FAILURE,
        NOT_STARTED,
        TIMEOUT,
        OK,
        ERROR
    };

    InjectionResult executeInjection(const Scenario &scenario, 
                                     virConnectPtr conn);

    bool copyFile(const std::string &filePathA, const std::string &filePathB);

    mutable std::string m_lastError;
    std::string m_name;
    std::string m_snapshotPath;
    std::string m_kernelLogPath;
    std::string m_programLogPath;
    std::string m_diskPath;
    std::string m_address;
    unsigned int m_portNr;
    bool m_saveDisk;
    std::mt19937 m_mt19937;
    std::uniform_int_distribution<long> m_dist;
    
    int m_socket;
};
}

#endif //FSFI_HOST_HPP
