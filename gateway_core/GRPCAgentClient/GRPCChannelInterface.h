#ifndef __GRPC_CHANNEL_INTERFACE_H__
#define __GRPC_CHANNEL_INTERFACE_H__

#include <chrono>
#include <string>

#include "../RTCAudioFormat/RTCAudioFormat.h"

class GRPCChannelInterface {
public:
   GRPCChannelInterface() = default;
   virtual ~GRPCChannelInterface() = default;
   
public:
    virtual void* GetStubPtr() = 0;
    virtual void* GetCQPtr() = 0;
    virtual int32_t CreateChannel() = 0;
    virtual std::string GetEndpoint() const = 0;
    virtual int32_t ConnectAndHandshake(std::chrono::milliseconds total_timeout) = 0;
    virtual RTCAudioFormat GetAgentInAudioFormat() = 0;
    virtual RTCAudioFormat GetAgentOutAudioFormat() = 0;

private:
    GRPCChannelInterface(const GRPCChannelInterface&) = delete;
    GRPCChannelInterface& operator=(const GRPCChannelInterface&) = delete;     
};

#endif // __GRPC_CHANNEL_INTERFACE_H__