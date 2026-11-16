#include "GRPCAgentChannel.h"
#include "GRPCAgentChannelManager.h"

GRPCAgentChannelManager::~GRPCAgentChannelManager() {

}

int32_t GRPCAgentChannelManager::CreateCQ() {
    _cq = std::make_unique<GRPCCompleteQueue>();
    if(_cq == nullptr) {
        return -1;
    }

    return 0;
}

std::shared_ptr<GRPCChannelInterface> GRPCAgentChannelManager::FindChannel(const std::string &endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _channels.find(endpoint);
    if(it == _channels.end()) {
        return nullptr;
    }
    return it->second;
}

void GRPCAgentChannelManager::AddChannel(std::shared_ptr<GRPCChannelInterface> channel) {
    std::lock_guard<std::mutex> lock(_mutex);
    _channels[channel->GetEndpoint()] = channel;    
}

void GRPCAgentChannelManager::RemoveChannel(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(_mutex);
    _channels.erase(endpoint);
}

GRPCCQInterface* GRPCAgentChannelManager::GetCQInterface() {
    return this;
}

void* GRPCAgentChannelManager::GetCQPtr() {
    if(_cq != nullptr) {
        return _cq->nativeCQPtr();
    }

    return nullptr;
}