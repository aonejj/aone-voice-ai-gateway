#ifndef __GRPC_AGENT_CHANNEL_MANAGER_H__
#define __GRPC_AGENT_CHANNEL_MANAGER_H__

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "GRPCCQInterface.h"
#include "GRPCCompleteQueue.h"
#include "GRPCChannelInterface.h"

class GRPCAgentChannelManager : public GRPCCQInterface {
public:
    static GRPCAgentChannelManager& GetInstance() {
        static GRPCAgentChannelManager instance;
        return instance;
    }

    ~GRPCAgentChannelManager();

public:
    int32_t CreateCQ();
    std::shared_ptr<GRPCChannelInterface> FindChannel(const std::string &endpoint);
    void AddChannel(std::shared_ptr<GRPCChannelInterface> channel);
    void RemoveChannel(const std::string& endpoint);
    GRPCCQInterface* GetCQInterface();
    
    // GRPCCQInterface method
    void* GetCQPtr() override;

private:
    GRPCAgentChannelManager() = default;    

private:
    mutable std::mutex _mutex;
    std::unordered_map<std::string, std::shared_ptr<GRPCChannelInterface>> _channels;
    std::unique_ptr<GRPCCompleteQueue> _cq;
    
private:
    GRPCAgentChannelManager(const GRPCAgentChannelManager&) = delete;
    GRPCAgentChannelManager& operator=(const GRPCAgentChannelManager&) = delete;     
};

#endif // __GRPC_AGENT_CHANNEL_MANAGER_H__