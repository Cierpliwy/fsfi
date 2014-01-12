#ifndef FSFI_SCENARIO_HPP
#define FSFI_SCENARIO_HPP
#include <chrono>
#include <string>

namespace fsfi
{
class Scenario
{
public:

    Scenario() : m_retries(0) {}

    struct Constraints
    {
        Constraints() : maxSeconds(0), maxSizeInBytes(0), maxIterations(0) {}
        Constraints(std::chrono::duration<float> maxSeconds,
                    unsigned long maxSizeInBytes,
                    unsigned long maxIterations)
            : maxSeconds(maxSeconds), maxSizeInBytes(maxSizeInBytes),
              maxIterations(maxIterations) {}

        std::chrono::duration<float> maxSeconds;
        unsigned long maxSizeInBytes;
        unsigned long maxIterations;
    };

    bool isCorrect() const;
    bool loadFromXML(const std::string &filePath);

    void setCommand(const std::string &command) {
        m_command = command;
    }
    const std::string getCommand() const {
        return m_command;
    }

    void setContraints(const Constraints &constraints) {
        m_constraints = constraints;
    }
    const Constraints& getConstraints() const {
        return m_constraints;
    }

    void setDescription(const std::string &description) {
        m_description = description;
    }
    const std::string& getDescription() const {
        return m_description;
    }

    void setName(const std::string &name) {
        m_name = name;
    }
    const std::string& getName() const {
        return m_name;
    }

    const std::string& getLastError() const {
        return m_lastError;
    }

    unsigned int getRetries() const {
        return m_retries;
    }
    void setRetries(unsigned int retries) {
        m_retries = retries;
    }

private:
    Constraints m_constraints;
    std::string m_command;
    std::string m_description;
    mutable std::string m_lastError;
    std::string m_name;
    unsigned int m_retries;
};
}

#endif //FSFI_SCENARIO_HPP
