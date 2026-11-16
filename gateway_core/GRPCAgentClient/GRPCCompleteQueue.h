#ifndef __GRPC_COMPLETE_QUEUE_H__
#define __GRPC_COMPLETE_QUEUE_H__

#include <memory>
#include <grpcpp/grpcpp.h>
#include <atomic>
#include <thread>
#include <vector>

class GRPCCompleteQueue {
public:
    explicit GRPCCompleteQueue(size_t poller_count = 1);
    ~GRPCCompleteQueue();
    grpc::CompletionQueue* nativeCQPtr() { return &_cq; }
    void Shutdown();

private:
    void Poll();

private:    
    grpc::CompletionQueue _cq;
    std::vector<std::thread> _pollers;
    std::atomic<bool> _stopped{false};
};

#endif // __GRPC_COMPLETE_QUEUE_H__