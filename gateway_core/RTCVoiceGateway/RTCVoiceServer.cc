#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "RTCVoiceServer"

#include <cassert>

#include <utils/Log.h>

#include "RTCClientFactory.h"
#include "RTCAudioTickScheduler.h"
#include "RTCVoiceClient.h"

#include "../RTCAgentBridge/AgentClientInterface.h"
#include "../RTCAgentBridge/AgentFactory.h"

#include "RTCVoiceServer.h"

#include "../RTCPeerConnection/rtc_pc_src/RTCFunctionTracer.h"

std::shared_ptr<RTCVoiceServer> RTCVoiceServer::Create(RTCSignalingSessionInterface* signaling_interface) {
    auto server = std::shared_ptr<RTCVoiceServer>(
        new RTCVoiceServer(signaling_interface)
    );

    if(server != nullptr) {
        if(server->_init() != OK) {
            return nullptr;
        }
    }

    return server;
}    
    
RTCVoiceServer::RTCVoiceServer(RTCSignalingSessionInterface* signaling_interface) 
 : RTCHandler(std::make_shared<RTCLooper>("VoiceServerLooper")), 
    _signaling_interface(signaling_interface) {
    RTC_CLASS_FUNCTION_TRACER("RTCVoiceServer::RTCVoiceServer", this);
    assert(_signaling_interface != nullptr);

    RTCClientFactory::InitializeSSL();    
    AgentFactory::Initialize();

    _signaling_interface->SetListener(this);
    _ice_servers = _signaling_interface->GetIceServerInfos();

    _state = RTC_VOICE_SERVER_UNINIT;  
}

RTCVoiceServer::~RTCVoiceServer() {
    RTCClientFactory::CleanupSSL();
}

status_t RTCVoiceServer::Start() {
    RTC_CLASS_FUNCTION_TRACER("RTCVoiceServer::Start", this); 
    Post([this] {
        _state = RTC_VOICE_SERVER_START;
        _audio_sender_scheduler = std::make_unique<RTCAudioTickScheduler>();
        if(_audio_sender_scheduler != nullptr) {
            _audio_sender_scheduler->Start(2);
        }
    });

    std::vector<AgentEndpoint> endpoints = _signaling_interface->GetAgentEndpoint();
    OnAgentEndPoint(endpoints);

    return OK;
}

void RTCVoiceServer::Stop() {
    RTC_CLASS_FUNCTION_TRACER("RTCVoiceServer::Stop", this); 

    PostAndWait([this] {
        if(_state == RTC_VOICE_SERVER_START) {
            for(auto& it : _clients) {
                it.second->Close();
            } 
            
            _clients.clear();

            if(!_pending_ice.empty()) {
                _pending_ice.clear();
            }

            if(_audio_sender_scheduler != nullptr) {
                _audio_sender_scheduler->Stop();
            }                

            _state = RTC_VOICE_SERVER_STOP;
        }
        return OK;
    });

    // hardcoding for debug
    {
        std::string endpoint = "0.0.0.0:50051";
        AgentFactory::RemoveAgentEndPoint(endpoint);
    }    
}

status_t RTCVoiceServer::OnLocalDescription(std::string &id, std::string &sdp) {
//    RTC_CLASS_FUNCTION_TRACER("RTCVoiceServer::OnLocalDescription", this); 

    Post([this, id, sdp] {
        if(_state != RTC_VOICE_SERVER_START) {
            return;
        }
        _signaling_interface->SendAnswer(id, sdp);

        auto it = _clients.find(id);
        if(it != _clients.end()) {
            it->second->Open();
        }        
    });

    return OK;
}

status_t RTCVoiceServer::OnLocalCandidate(std::string &id, int32_t mline_idx, 
                                std::string &mid, std::string &candidate) {
//    RTC_CLASS_FUNCTION_TRACER("RTCVoiceServer::OnLocalCandidate", this); 

    Post([this, id, mline_idx, mid, candidate] {
        if(_state != RTC_VOICE_SERVER_START) {
            return;
        }
         _signaling_interface->SendCandidate(id, mline_idx, mid, candidate);
    });

    return OK;    
}

status_t RTCVoiceServer::OnConnectionState(std::string &id, RTCIceState ice_state) {
    Post([this, id, ice_state] {
        std::string desc;
        if(ice_state == (int32_t)RTCClientListener::RTC_ICE_STATE_INIT) {
            return;
        } else if(ice_state == (int32_t)RTCClientListener::RTC_ICE_STATE_CONNECTING) {
            desc = "ice connecting";
        } else if(ice_state == (int32_t)RTCClientListener::RTC_ICE_STATE_CONNECTED) {
            desc = "ice connected";
        } else if(ice_state == (int32_t)RTCClientListener::RTC_ICE_STATE_DISCONNECTED) {
            desc = "ice disconnected";
        } else if(ice_state == (int32_t)RTCClientListener::RTC_ICE_STATE_CLOSED) {
            desc = "ice closed";
        } else if(ice_state == (int32_t)RTCClientListener::RTC_ICE_STATE_FAILED) {
            desc = "ice failed";
        }             

        _signaling_interface->SendIceState(id, (uint32_t)ice_state, desc);        
    });

    return OK;
}    

status_t RTCVoiceServer::OnSetRemoteDescription(std::string &id) {
    Post([this, id] {
        auto it = _clients.find(id);
        if(it != _clients.end()) {
            it->second->GetPeerPtr()->CreateAnswer();
        } else {
            assert(0);
        }
    });
  
    return OK;
}

status_t RTCVoiceServer::OnClientError(std::string &id, std::string& err) {
    Post([this, id, err] {
        // TODO
    });

    return OK;
}

uint32_t RTCVoiceServer::OnConnectedPeer(std::string& id, std::string& agentId) {
    RTC_CLASS_FUNCTION_TRACER("RTCVoiceServer::OnConnectedPeer", this);

    status_t ret = PostAndWait([this, id, agentId] {
        if(_state != RTC_VOICE_SERVER_START) {
            return INVALID_OPERATION;
        }
        
        std::shared_ptr<RTCClientInterface> peer = RTCClientFactory::CreateRTCClient(id, _ice_servers, this, 
                _audio_sender_scheduler != nullptr ? _audio_sender_scheduler.get() : nullptr);
        if(peer == nullptr) {
            return INVALID_OPERATION;
        }

        if(peer->CreatePeerConnection() != OK) {
            return INVALID_OPERATION;
        }

        auto it = _agents.find(agentId);
        if(it == _agents.end()) {
            return INVALID_OPERATION;
        }

        std::shared_ptr<AgentClientInterface> agent = AgentFactory::CreateAgentClient(it->second, agentId);
        if(agent == nullptr) {
            return INVALID_OPERATION;
        }

        std::shared_ptr<RTCVoiceClient> client = std::make_shared<RTCVoiceClient>(id, peer, agent);
        _clients[id] = client;

        return OK; 
    });

    if(ret != OK) {
        return -1;
    }
    return 0;
}

uint32_t RTCVoiceServer::OnDisconnectedPeer(std::string& id) {
    RTC_CLASS_FUNCTION_TRACER("RTCVoiceServer::OnDisconnectedPeer", this);

    status_t ret = PostAndWait([this, id] {
        if(_state != RTC_VOICE_SERVER_START) {
            return INVALID_OPERATION;
        }

        auto it = _clients.find(id);
        if(it == _clients.end()) {
            return INVALID_OPERATION;
        }
        
        std::shared_ptr<RTCVoiceClient> client = it->second;
        _clients.erase(it);
        client->Close();

        return OK;         
    });

    if(ret != OK) {
        return -1;
    }
    return 0;
}

void RTCVoiceServer::OnAgentEndPoint(std::vector<AgentEndpoint>& endpoints) {
    ALOGV("OnAgentEndPoint");
    Post([this, endpoints] {
        for(auto it : endpoints) {
            AgentFactory::AddAgentEndPoint(it._endpoint);
            _agents[it._id] = it._endpoint;
        }
    });
}

void RTCVoiceServer::OnAgentRemoved(std::string& id) {
    Post([this, id] {
        // TODO...
    });
}

uint32_t RTCVoiceServer::OnPeerOffer(std::string& id, std::string& sdp) {
//    RTC_CLASS_FUNCTION_TRACER("RTCVoiceServer::OnPeerOffer", this);
    Post([this, id, sdp] {
        if(_state != RTC_VOICE_SERVER_START) {
            return;
        }

        auto it = _clients.find(id);
        if(it == _clients.end()) {
            ALOGE("not found client");
            return;
        }

        it->second->GetPeerPtr()->SetRemoteDescription(sdp);

        auto cit = _pending_ice.find(id);
        if(cit != _pending_ice.end()) {
            std::vector<RTCCandidateInfo> vice = cit->second;
            _pending_ice.erase(cit);

            for(auto tmpit : vice) {
                it->second->GetPeerPtr()->SetRemoteCandidate(tmpit._sdp_index_line, tmpit._sdp_mid, tmpit._candidate);
            }
        }                   
    });

    return 0;    
}

uint32_t RTCVoiceServer::OnPeerCandidate(std::string& id, int32_t mline_idx, 
                    std::string& mid, std::string& candidate) {
//    RTC_CLASS_FUNCTION_TRACER("RTCVoiceServer::OnPeerCandidate", this);  

    Post([this, id, mline_idx, mid, candidate] {
        if(_state != RTC_VOICE_SERVER_START) {
            return;
        }

        auto it = _clients.find(id);
        if(it == _clients.end()) {
            RTCCandidateInfo cinfo;
            cinfo._sdp_mid = mid;
            cinfo._sdp_index_line = mline_idx;
            cinfo._candidate = candidate;

            auto cit = _pending_ice.find(id);
            if(cit == _pending_ice.end()) {
                std::vector<RTCCandidateInfo> vice;
                vice.push_back(cinfo);
                _pending_ice[id] = vice;
            } else {
                cit->second.push_back(cinfo);
            }
        } else {
            it->second->GetPeerPtr()->SetRemoteCandidate(mline_idx, mid, candidate);
        }                    
    });

    return 0;
}

status_t RTCVoiceServer::_init() {
	_state = RTC_VOICE_SERVER_INIT;
	return OK;
}
