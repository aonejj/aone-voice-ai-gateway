//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_ICE_SERVER_INFO_H__
#define __RTC_ICE_SERVER_INFO_H__

#include <string>

struct RTCIceServerInfo {
	bool			_is_stun = true;
	std::string		_uri;
	std::string		_user_name;
	std::string		_credential;
};

struct RTCCandidateInfo {
	std::string _sdp_mid;
	uint32_t	_sdp_index_line;
	std::string	_candidate;
};

#endif //__RTC_ICE_SERVER_INFO_H__