#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "AgentFactory"

#include <utils/Log.h>

#include "AgentFactory.h"
#include "AgentClient.h"

#include "GRPCAgentSessionAPI.h"

void AgentFactory::Initialize() {
    GRPCAgentSessionInitialize();
}

int32_t AgentFactory::AddAgentEndPoint(const std::string& endpoint) {
    return GRPCAddAgentEndPoint(endpoint);
}

void AgentFactory::RemoveAgentEndPoint(const std::string& endpoint) {
    ALOGV("RemoveAgentEndPoint+++");
    GRPCRemoveAgentEndPoint(endpoint);
    ALOGV("RemoveAgentEndPoint---");
}

std::shared_ptr<AgentClientInterface> AgentFactory::CreateAgentClient(const std::string& endpoint, const std::string& id) {
    return AgentClient::Create(endpoint, id);
}