//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_CLIENT_FACTORY_H__
#define __RTC_CLIENT_FACTORY_H__

#include <string>
#include <vector>
#include <memory>

#include "RTCClientListener.h"
#include "RTCClientInterface.h"
#include "../RTCIceInfo/RTCIceInfo.h"

class RTCAudioSenderSchedulerInterface;	

class RTCClientFactory {
public:
	static std::shared_ptr<RTCClientInterface> CreateRTCClient(const std::string &id, std::vector<RTCIceServerInfo>& rtcIceInfos,
												  			 RTCClientListener* listener,
												  			 RTCAudioSenderSchedulerInterface* audio_sender_scheduler);
	static bool InitializeSSL();
	static bool CleanupSSL(); 
private:
	RTCClientFactory() = default;
	~RTCClientFactory() = default;

private:
    RTCClientFactory(const RTCClientFactory&) = delete;
    RTCClientFactory& operator=(const RTCClientFactory&) = delete;  
};


#endif // __RTC_CLIENT_FACTORY_H__
