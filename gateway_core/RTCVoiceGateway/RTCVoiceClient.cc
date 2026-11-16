#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "RTCVoiceServer"
#include <utils/Log.h>

#include <utils/Errors.h>
#include "../RTCClient/RTCClientInterface.h"
#include "../RTCAgentBridge/AgentClientInterface.h"
#include "../RTCClient/RTCAudioOuterSetInterface.h"
#include "RTCVoiceClient.h"

RTCVoiceClient::RTCVoiceClient(const std::string& id, std::shared_ptr<RTCClientInterface> peer, 
                                std::shared_ptr<AgentClientInterface> agent)
 : _id(id),
   _peer(peer),
   _agent(agent) {

}

RTCVoiceClient::~RTCVoiceClient() {

}

RTCClientInterface* RTCVoiceClient::GetPeerPtr() {
    return _peer.get();
}

AgentClientInterface* RTCVoiceClient::GetAgentPtr() {
    return _agent.get();
}

status_t RTCVoiceClient::Open() {
    RTCAudioIngressSinkSetInterface* ingress = _peer->GetAudioIngressSetInterface();
    RTCAudioEgressSourceSetInterface* egress = _peer->GetAudioEgressSetInterface();
    if(ingress == nullptr || egress == nullptr) {
        return INVALID_OPERATION;
    }

    RTCIngressAudioPacketSinkInterface* aingress = _agent->GetIngressPtr();
    RTCEgressAudioSourceInterface* aegress = _agent->GetEgressPtr();

    status_t sret = _agent->Open();
    if(sret != OK) {
        return sret;
    }
    ingress->setIngressSink(aingress);
    egress->SetEgressAudioSource(aegress);

    return OK;
}

status_t RTCVoiceClient::Close() {
    ALOGV("Close+++");
    RTCAudioIngressSinkSetInterface* ingress = _peer->GetAudioIngressSetInterface();
    RTCAudioEgressSourceSetInterface* egress = _peer->GetAudioEgressSetInterface();
    if(ingress != nullptr) {
        ingress->setIngressSink(nullptr);
    }

    if(egress != nullptr) {
        egress->SetEgressAudioSource(nullptr);
    }

    _agent->Close();    
    _peer->Close();
    ALOGV("Close---");
    return OK;
}