#include "GRPCCompleteQueue.h"
#include "GRPCCQTag.h"

GRPCCompleteQueue::GRPCCompleteQueue(size_t count) {
    if (!count) {
        count = 1;
    }
  
    for (size_t i = 0; i < count; ++i) { 
        _pollers.emplace_back([this] { Poll(); });
    }
}

GRPCCompleteQueue::~GRPCCompleteQueue() { 
    Shutdown(); 
}

void GRPCCompleteQueue::Poll() {
    void* tag = nullptr; 
    bool ok = false;
  
    while (_cq.Next(&tag, &ok)) {
        std::unique_ptr<GRPCCQTag> owned(static_cast<GRPCCQTag*>(tag));
        owned->Proceed(ok);
    }
}

void GRPCCompleteQueue::Shutdown() {
    if (_stopped.exchange(true)) {
        return;
    }
  
    _cq.Shutdown();
  
    for (auto& t : _pollers) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    _pollers.clear();
}