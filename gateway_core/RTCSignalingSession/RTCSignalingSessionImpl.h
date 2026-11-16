#ifndef __RTC_SIGNALING_SESSION_IMPL_H__
#define __RTC_SIGNALING_SESSION_IMPL_H__

#include <string>
#include <vector>
#include <memory>

#include <utils/Errors.h>

#include "../RTCLooper/RTCHandler.h"
#include "RTCWebSocketClientListener.h"

#include "RTCSignalingSessionListener.h"
#include "RTCSignalingSessionInterface.h"
#include "RTCSignalingSessionStatusObserver.h"
#include "RTCIceInfo.h"

class RTCWebSocketClient;    

class RTCSignalingSessionImpl : public RTCHandler,
								public RTCWebSocketClientListener,
                            	public RTCSignalingSessionInterface {
public:
    virtual ~RTCSignalingSessionImpl();

public:
    static std::shared_ptr<RTCSignalingSessionImpl> Create(RTCSignalingSessionStatusObserver *observer);

public:
	status_t Open(std::string &host, int port, std::string &path);
	status_t Close();
    
    // RTCSignalingSessionInterface
    void SetListener(RTCSignalingSessionListener* listener) override;
    std::vector<RTCIceServerInfo> GetIceServerInfos() override;
    std::vector<AgentEndpoint> GetAgentEndpoint() override;
    uint32_t SendAnswer(const std::string& id, const std::string& sdp) override;
    uint32_t SendCandidate(const std::string& id, int32_t index_line,
         const std::string& mid, const std::string& candidate) override;
    uint32_t SendIceState(const std::string& id, int32_t state, 
        const std::string& desc) override;
        
private:
    RTCSignalingSessionImpl(RTCSignalingSessionStatusObserver *observer);    

private:
    // RTCWebSocketClientListener
	status_t onWSConnected() override;
	status_t onWSDisconnected() override;
	status_t onWSError() override;
	status_t onWSMessage(std::string &ws_msg) override;

private:
	typedef enum {
		RTC_SIGNAL_SESSION_UNINT,
		RTC_SIGNAL_SESSION_INT,
		RTC_SIGNAL_SESSION_CONNECTED,
        RTC_SIGNAL_SESSION_DISCONNECTING,
		RTC_SIGNAL_SESSION_DISCONNECTED,
		RTC_SIGNAL_SESSION_ERROR,
	} RTCSignalSessionImplState;    

private:
	status_t _init();
	void	 _reset();    

    status_t _open(const std::string &host, int port, const std::string &path);
    void _close();
    status_t _regist_req();
    status_t _received_msg(const std::string pmsg);
    status_t _send_answer(const char *id, const char *sdp);
    status_t _send_candidate(const char *id, int32_t mline_index, 
        const char* mid, const char* candidate);
    status_t _send_ice_state(const char *id, int32_t state, 
        const char* desc);
    void _post_command_response(const std::string &cmd, 
        const std::string &id, const std::string &result); 
    status_t _send_command_response(const char* cmd, const char* id, 
        const char* result);    
    bool _is_connected() const;                   

private:    
	std::unique_ptr<RTCWebSocketClient>  	_ws_client;
    RTCSignalingSessionListener*   	 		_session_rtc_listener;
	RTCSignalingSessionStatusObserver*		_session_status_observer;
	std::string                     		_id;
    std::vector<RTCIceServerInfo>   		_ice_infos;
    RTCSignalSessionImplState 				_state;
    
    std::vector<AgentEndpoint>              _agent_endpoints;

private:
    RTCSignalingSessionImpl(const RTCSignalingSessionImpl&) = delete;
    RTCSignalingSessionImpl& operator=(const RTCSignalingSessionImpl&) = delete;     
};

#endif // __RTC_SIGNALING_SESSION_IMPL_H__