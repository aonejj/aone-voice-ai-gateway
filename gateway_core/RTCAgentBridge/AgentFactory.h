#ifndef __AGENT_FACTORY_H__
#define __AGENT_FACTORY_H__

#include <memory>
#include <string>

#include "AgentClientInterface.h"

class AgentFactory {
public:
    static void Initialize();
    static int32_t AddAgentEndPoint(const std::string& endpoint);
    static void RemoveAgentEndPoint(const std::string& endpoint);
    static std::shared_ptr<AgentClientInterface> CreateAgentClient(const std::string& endpoint, const std::string& id);
};

#endif // __AGENT_FACTORY_H__