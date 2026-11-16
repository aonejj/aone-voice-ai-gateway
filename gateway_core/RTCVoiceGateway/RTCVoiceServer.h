#ifndef __RTC_VOICE_SERVER_H__
#define __RTC_VOICE_SERVER_H__

#include <string>
#include <unordered_map>
#include <memory>

#include <utils/Errors.h>

#include "RTCIceInfo.h"
#include "../RTCClient/RTCClientListener.h"
#include "../RTCClient/RTCClientInterface.h" 

#include "RTCSignalingSessionListener.h"
#include "RTCSignalingSessionInterface.h"
#include "../RTCLooper/RTCHandler.h"


class RTCAudioTickScheduler;
class RTCVoiceClient;

class RTCVoiceServer : public RTCHandler,
                       public RTCClientListener,
                       public RTCSignalingSessionListener {
public:
    virtual ~RTCVoiceServer();

public:
    static std::shared_ptr<RTCVoiceServer> Create(RTCSignalingSessionInterface* signaling_interface);
    status_t Start();
    void Stop();

private:
    typedef enum {
        RTC_VOICE_SERVER_UNINIT,
        RTC_VOICE_SERVER_INIT,
        RTC_VOICE_SERVER_START,
        RTC_VOICE_SERVER_STOP,
    }RTCVoiceServerState;

private:
    RTCVoiceServer(RTCSignalingSessionInterface* signaling_interface);                    

private:
    // RTCClientListener method
	status_t OnLocalDescription(std::string &id, std::string &sdp) override;
	status_t OnLocalCandidate(std::string &id, int32_t mline_idx, 
								 std::string &mid, std::string &candidate) override;
	status_t OnConnectionState(std::string &id, RTCIceState ice_state) override;
	status_t OnSetRemoteDescription(std::string &id) override;
	status_t OnClientError(std::string &id, std::string& err) override;        

    // RTCSignalingSessionListener method
    uint32_t OnConnectedPeer(std::string& id, std::string& agentId) override;
    uint32_t OnDisconnectedPeer(std::string& id) override;
    void OnAgentEndPoint(std::vector<AgentEndpoint>& endpoints) override;
    void OnAgentRemoved(std::string& id) override;    
    uint32_t OnPeerOffer(std::string& id, std::string& sdp) override;
    uint32_t OnPeerCandidate(std::string& id, int32_t mline_idx, 
                        std::string& mid, std::string& candidate) override;

private:
    status_t _init();                    

private:
    RTCSignalingSessionInterface* _signaling_interface;
    std::unordered_map<std::string, std::shared_ptr<RTCVoiceClient>> _clients;
    std::unordered_map<std::string, std::vector<RTCCandidateInfo>> _pending_ice;
    std::unordered_map<std::string, std::string> _agents;
    std::vector<RTCIceServerInfo> _ice_servers;

    RTCVoiceServerState _state;

    std::unique_ptr<RTCAudioTickScheduler> _audio_sender_scheduler;
private:
    RTCVoiceServer(const RTCVoiceServer&) = delete;
    RTCVoiceServer& operator=(const RTCVoiceServer&) = delete;                          
};

#endif // __RTC_VOICE_SERVER_H__
