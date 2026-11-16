//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_CLIENT_RTP_RECEIVER_INTERFACE_H__
#define __RTC_CLIENT_RTP_RECEIVER_INTERFACE_H__

#include <vector>
#include <utils/Errors.h>

#include "absl/types/optional.h"

#include "api/rtp_parameters.h"
#include "modules/rtp_rtcp/source/rtp_header_extensions.h"

#include "../RTCPeerConnection/rtc_pc_src/api/RTCReceivedPacketSinks.h"

#include "RTCAudioOuterSetInterface.h"

class RTCClientInterface;

class RTCClientRtpReceiverInterface : public webrtc::RTCMediaReceivedPacketSink,
									  public RTCAudioIngressSinkSetInterface {
public:
	RTCClientRtpReceiverInterface() = default;
	~RTCClientRtpReceiverInterface() = default;

public:
	virtual std::vector<webrtc::RtpParameters> getRtpParameters() = 0;
	virtual absl::optional<webrtc::RtpHeaderExtensionMap> getRtpHeaderExtensionMap() = 0;
	virtual const RTCClientInterface* getRTCClientInterface() const = 0;
	virtual void close() = 0;

	virtual void GenerateKeyFrame() {}
	virtual void GenerateKeyFramePli(uint32_t ssrc) {}
};

#endif // __RTC_CLIENT_RTP_RECEIVER_INTERFACE_H__