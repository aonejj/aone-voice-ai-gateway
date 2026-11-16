#include <memory>
#include <string>

#include "RTCSignalingSessionStatusObserver.h"
#include "RTCSignalingSessionInterface.h"

class RTCSignalingSessionCtrl {
public:
    static std::unique_ptr<RTCSignalingSessionCtrl> Create(RTCSignalingSessionStatusObserver *observer);
    ~RTCSignalingSessionCtrl();

public:
    uint32_t Open(std::string &host, int port, std::string &path);
    uint32_t Close();

    RTCSignalingSessionInterface* GetInterface();

private:
    RTCSignalingSessionCtrl(RTCSignalingSessionStatusObserver *observer);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
    
private:
	RTCSignalingSessionCtrl(const RTCSignalingSessionCtrl &) = delete;
	RTCSignalingSessionCtrl &operator=(const RTCSignalingSessionCtrl &) = delete;    
    
};