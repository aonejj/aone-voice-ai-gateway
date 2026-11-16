//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "RTCClientFactory"
#include <utils/Log.h>

#include "RTCAudioSenderSchedulerInterface.h"
#include "RTCClientFactory.h"
#include "RTCClientImpl.h"

std::shared_ptr<RTCClientInterface> RTCClientFactory::CreateRTCClient(const std::string &id, 
												   	   std::vector<RTCIceServerInfo>& rtcIceInfos,
												       RTCClientListener* listener,
													   RTCAudioSenderSchedulerInterface* audio_sender_scheduler) {
	return RTCClientImpl::Create(id, rtcIceInfos, listener, audio_sender_scheduler);
}

bool RTCClientFactory::InitializeSSL() {
	return RTCClientImpl::InitializeSSL();
}

bool RTCClientFactory::CleanupSSL() {
	return RTCClientImpl::CleanupSSL();
}
