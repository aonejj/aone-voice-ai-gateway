#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "RTCSignalingSessionImpl"

#include <utils/Log.h>
#include <jansson.h>

#include "RTCWebSocketClient.h"
#include "RTCSignalingSessionImpl.h"

// attibute
const char kCommandAttr[] = "cmd";
const char kIdAttr[] = "id";
const char kResultAttr[] = "result";
const char kIceInfoAttr[] = "iceinfo";
const char kSdpAttr[] = "sdp";
const char kCandidateAttr[] = "candidate";
const char kMLineIndexAttr[] = "mline_index";
const char kMIdAttr[] = "mid";
const char kUrlAttr[] = "url";
const char kUserNameAttr[] = "username";
const char kCredentialAttr[] = "credential";
const char kIceStateAttr[] = "state";
const char kIceStateDescAttr[] = "desc";
const char kAgentsAttr[] = "agents";
const char kEndpointAttr[] = "endpoint";
const char kAgentIdAttr[] = "agentId";

// commands
const char kRegistMsg[] = "regist";
const char kRegistMsgRes[] = "regist_res";

const char kAddPeerMsg[] = "add_peer";
const char kAddPeerMsgRes[] = "add_peer_res";

const char kRemovePeerMsg[] = "remove_peer";
const char kRemovePeerMsgRes[] = "remove_peer_res";

const char kAgentAddMsg[] = "agent_add";
const char kAgentAddMsgRes[] = "agent_add_res";
const char kAgentRemoveMsg[] = "agent_remove";
const char kAgentRemoveMsgRes[] = "agent_remove_res";

const char kOfferMsg[] = "offer";
const char kAnswerMsg[] = "answer";
const char kCandidateMsg[] = "candidate";
const char kIceStateMsg[] = "iceState";

// result
const char kSuccess[] = "success";
const char kFail[] = "fail";

std::shared_ptr<RTCSignalingSessionImpl> RTCSignalingSessionImpl::Create(RTCSignalingSessionStatusObserver *observer) {
	auto session = std::shared_ptr<RTCSignalingSessionImpl>(new RTCSignalingSessionImpl(observer));
	if(session != nullptr) {
		if(session->_init() != OK) {
			return nullptr;
		}
	}

	return session;
}   

RTCSignalingSessionImpl::RTCSignalingSessionImpl(RTCSignalingSessionStatusObserver *observer) 
 : RTCHandler(std::make_shared<RTCLooper>("SignalingSessionLooper")), 
   _session_rtc_listener(nullptr),
   _session_status_observer(observer) {
	_state = RTC_SIGNAL_SESSION_UNINT;
}

RTCSignalingSessionImpl::~RTCSignalingSessionImpl() {
	_reset();
}

status_t RTCSignalingSessionImpl::Open(std::string &host, int port, std::string &path) {
	Post([this, host, port, path] {
		_open(host, port, path);
	});

	return OK;
}

status_t RTCSignalingSessionImpl::Close() {
	Post([this] {
		_close();
	});

	return OK;
}

// RTCSignalingSessionInterface
void RTCSignalingSessionImpl::SetListener(RTCSignalingSessionListener* listener) {
    _session_rtc_listener = listener;
}

std::vector<RTCIceServerInfo> RTCSignalingSessionImpl::GetIceServerInfos() {
    return _ice_infos;
}

std::vector<AgentEndpoint> RTCSignalingSessionImpl::GetAgentEndpoint() {
	return _agent_endpoints;
}

uint32_t RTCSignalingSessionImpl::SendAnswer(const std::string& id, const std::string& sdp) {
	Post([this, id, sdp] {
		if(_is_connected()) {
			_send_answer(id.c_str(), sdp.c_str());
		}
	});

	return 0;
}

uint32_t RTCSignalingSessionImpl::SendCandidate(const std::string& id, int32_t index_line,
                            const std::string& mid, const std::string& candidate) {
	Post([this, id, index_line, mid, candidate] {
		if(_is_connected()) {
			_send_candidate(id.c_str(), index_line, mid.c_str(), candidate.c_str());
		}
	}); 								

	return 0;
}

uint32_t RTCSignalingSessionImpl::SendIceState(const std::string& id, int32_t state, 
                            const std::string& desc) {
	Post([this, id, state, desc] {
		if(_is_connected()) {
			_send_ice_state(id.c_str(), state, desc.c_str());
		}
	});								

	return OK;
}

status_t RTCSignalingSessionImpl::onWSConnected() {
	Post([this] {
		_state = RTC_SIGNAL_SESSION_CONNECTED;
		_regist_req();		
	});

	return OK;
}

status_t RTCSignalingSessionImpl::onWSDisconnected() {
	Post([this] {
		_state = RTC_SIGNAL_SESSION_DISCONNECTED;
		_session_status_observer->OnSignalingSessionClosed();
	});

	return OK;
}

status_t RTCSignalingSessionImpl::onWSError() {
	Post([this] {
		_state = RTC_SIGNAL_SESSION_ERROR;
		_session_status_observer->OnSignalingSessionError();
	});

	return OK;
}

status_t RTCSignalingSessionImpl::onWSMessage(std::string &ws_msg) {
	Post([this, ws_msg] {
		_received_msg(ws_msg.c_str());
	});

	return OK;
}

status_t RTCSignalingSessionImpl::_init() {
	_state = RTC_SIGNAL_SESSION_INT;
	return OK;
}

void RTCSignalingSessionImpl::_reset() {
	_ws_client.reset();
}

status_t RTCSignalingSessionImpl::_open(const std::string &host, int port, const std::string &path){
	_ws_client = std::make_unique<RTCWebSocketClient>(this);
	if (_ws_client == nullptr) {
		Post([this] {
			_session_status_observer->OnSignalingSessionError();
		});

		return INVALID_OPERATION;
	}

	status_t ret = _ws_client->Open(host.c_str(), port, path.c_str());
	if (ret != OK) {
		_ws_client.reset();
		_ws_client = nullptr;

		Post([this] {
			_session_status_observer->OnSignalingSessionError();
		});
		
		return INVALID_OPERATION;
	}

	return OK;    
}

void RTCSignalingSessionImpl::_close() {
	_state = RTC_SIGNAL_SESSION_DISCONNECTING;
	if (_ws_client != nullptr) {
		_ws_client->Close();
        _ws_client.reset();
	}
}

status_t RTCSignalingSessionImpl::_regist_req() {
	json_t *jroot = json_object();

	json_object_set_new(jroot, kCommandAttr, json_string(kRegistMsg));
	std::string msg = json_dumps(jroot, 0);
	json_decref(jroot);

	_ws_client->Send(msg);

	return OK;
}

status_t RTCSignalingSessionImpl::_received_msg(const std::string pmsg) {
	json_error_t jerror;
	json_t *jroot = json_loads(pmsg.c_str(), pmsg.size(), &jerror);
	if (!jroot) {
		return INVALID_OPERATION;
	}

	ALOGV("%s", pmsg.c_str());

	status_t sret = OK;
	json_t *jcmd = json_object_get(jroot, kCommandAttr);
	std::string cmd = json_string_value(jcmd);

	if (cmd == kRegistMsgRes) {
		json_t *jresult = json_object_get(jroot, kResultAttr);
		std::string result = json_string_value(jresult);
		if (result != kSuccess) {
			onWSError();
			sret = INVALID_OPERATION;
		} else {
			json_t *jnode_id = json_object_get(jroot, kIdAttr);
			_id = json_string_value(jnode_id);
			json_t *jiceinfo_array = json_object_get(jroot, kIceInfoAttr);
			if (jiceinfo_array && json_is_array(jiceinfo_array)) {
				size_t index;
				json_t* jice_entry;

				json_array_foreach(jiceinfo_array, index, jice_entry) {
					RTCIceServerInfo ice_info;
					json_t* jurl = json_object_get(jice_entry, kUrlAttr);
					ice_info._uri = json_string_value(jurl);

					json_t* jusername = json_object_get(jice_entry, kUserNameAttr);
					if (jusername) {
						ice_info._is_stun = false;
						ice_info._user_name = json_string_value(jusername);
					}

					json_t* jcredential = json_object_get(jice_entry, kCredentialAttr);
					if (jcredential) {
						ice_info._credential = json_string_value(jcredential);
					}
					_ice_infos.push_back(ice_info);
				}
			}

			json_t *jendpoint_array = json_object_get(jroot, kAgentsAttr);
			if(jendpoint_array && json_is_array(jendpoint_array)) {
				json_t* jendpoint_entry;
				size_t index;

				json_array_foreach(jendpoint_array, index, jendpoint_entry) {
					AgentEndpoint endpoint;
					json_t* jid = json_object_get(jendpoint_entry, kIdAttr);
					json_t* jendpoint = json_object_get(jendpoint_entry, kEndpointAttr);
					endpoint._id = json_string_value(jid);					
					endpoint._endpoint = json_string_value(jendpoint);
					_agent_endpoints.push_back(endpoint);
				}
			}

			_session_status_observer->OnSignalingSessionOpened();
		}
	} else if (cmd == kAddPeerMsg) {
		json_t *jpeer_id = json_object_get(jroot, kIdAttr);
		json_t *jagent_id = json_object_get(jroot, kAgentIdAttr);

		std::string peer_id = json_string_value(jpeer_id);
		std::string agent_id = json_string_value(jagent_id);
		uint32_t sret = _session_rtc_listener->OnConnectedPeer(peer_id, agent_id);
        
        std::string cmd("add_peer_res");
        std::string result;
        if(sret != 0) {
            result = "fail";
        } else {
            result = "success";
        }

        _post_command_response(cmd, peer_id, result);

	} else if (cmd == kRemovePeerMsg) {
		json_t *jpeer_id = json_object_get(jroot, kIdAttr);
		std::string peer_id = json_string_value(jpeer_id);
		uint32_t sret = _session_rtc_listener->OnDisconnectedPeer(peer_id);
        
        std::string cmd("remove_peer_res");
        std::string result;
        if(sret != 0) {
            result = "fail";
        } else {
            result = "success";
        }

        _post_command_response(cmd, peer_id, result);  

	} else if (cmd == kOfferMsg) {

		json_t *jpeer_id = json_object_get(jroot, kIdAttr);
		std::string peer_id = json_string_value(jpeer_id);
		json_t *jsdp = json_object_get(jroot, kSdpAttr);
		std::string sdp = json_string_value(jsdp);

		_session_rtc_listener->OnPeerOffer(peer_id, sdp);

	} else if (cmd == kCandidateMsg){

		json_t *jpeer_id = json_object_get(jroot, kIdAttr);
		std::string peer_id = json_string_value(jpeer_id);
		json_t *jmline_idx = json_object_get(jroot, kMLineIndexAttr);
		int32_t mline_idx = json_integer_value(jmline_idx);
		json_t *jmid = json_object_get(jroot, kMIdAttr);
		std::string mid = json_string_value(jmid);
		json_t *jcandidate = json_object_get(jroot, kCandidateAttr);
		std::string candidate = json_string_value(jcandidate);

		_session_rtc_listener->OnPeerCandidate(peer_id, mline_idx, mid, candidate);

	} else if(cmd == kAgentAddMsg) {

		json_t *jid = json_object_get(jroot, kIdAttr);
		json_t *jendpoint = json_object_get(jroot, kEndpointAttr);
		std::vector<AgentEndpoint> endpoints;
		AgentEndpoint endpoint;
		endpoint._id = json_string_value(jid);
		endpoint._endpoint = json_string_value(jendpoint);
		endpoints.push_back(endpoint);
		
		_session_rtc_listener->OnAgentEndPoint(endpoints);

		std::string cmd(kAgentAddMsgRes);
        std::string result = "success";
		_post_command_response(cmd, endpoint._id, result); 
		
	} else if(cmd == kAgentRemoveMsg) {

		json_t *jid = json_object_get(jroot, kIdAttr);
		std::string agent_id = json_string_value(jid);

		_session_rtc_listener->OnAgentRemoved(agent_id);
		std::string cmd(kAgentRemoveMsgRes);
        std::string result = "success";
		_post_command_response(cmd, agent_id, result); 		
	}

	json_decref(jroot);

	return sret;    
}

status_t RTCSignalingSessionImpl::_send_answer(const char *id, const char *sdp) {
	json_t *jroot = json_object();
	json_object_set_new(jroot, kCommandAttr, json_string(kAnswerMsg));
	json_object_set_new(jroot, kIdAttr, json_string(id));
	json_object_set_new(jroot, kSdpAttr, json_string(sdp));

	std::string msg = json_dumps(jroot, 0);
	json_decref(jroot);
	_ws_client->Send(msg);

	return OK;
}

status_t RTCSignalingSessionImpl::_send_candidate(const char *id, int32_t mline_index, 
    const char* mid, const char* candidate) {
	json_t *jroot = json_object();
	json_object_set_new(jroot, kCommandAttr, json_string(kCandidateMsg));
	json_object_set_new(jroot, kIdAttr, json_string(id));
	json_object_set_new(jroot, kMLineIndexAttr, json_integer(mline_index));
	json_object_set_new(jroot, kMIdAttr, json_string(mid));
	json_object_set_new(jroot, kCandidateAttr, json_string(candidate));

	std::string msg = json_dumps(jroot, 0);
	json_decref(jroot);
	_ws_client->Send(msg);

	return OK;
}

status_t RTCSignalingSessionImpl::_send_ice_state(const char *id, 
    int32_t state, const char* desc) {
	json_t *jroot = json_object();
	json_object_set_new(jroot, kCommandAttr, json_string(kIceStateMsg));
	json_object_set_new(jroot, kIdAttr, json_string(id));
	json_object_set_new(jroot, kIceStateAttr, json_integer(state));
	json_object_set_new(jroot, kIceStateDescAttr, json_string(desc));

	std::string msg = json_dumps(jroot, 0);
	json_decref(jroot);
	_ws_client->Send(msg);

	return OK;
}

void RTCSignalingSessionImpl::_post_command_response(const std::string &cmd, 
    const std::string &id, const std::string &result) {
	Post([this, cmd, id, result] {
		_send_command_response(cmd.c_str(), id.c_str(), result.c_str());
	});
}

status_t RTCSignalingSessionImpl::_send_command_response(const char* cmd, const char* id, const char* result) {
	json_t *jroot = json_object();
	json_object_set_new(jroot, kCommandAttr, json_string(cmd));
	json_object_set_new(jroot, kIdAttr, json_string(id));
	json_object_set_new(jroot, kResultAttr, json_string(result));

	std::string msg = json_dumps(jroot, 0);
	json_decref(jroot);
	_ws_client->Send(msg);
	return OK;
}

bool RTCSignalingSessionImpl::_is_connected() const {
	if(_state == RTC_SIGNAL_SESSION_CONNECTED) {
		return true;
	}
	return false;
}
