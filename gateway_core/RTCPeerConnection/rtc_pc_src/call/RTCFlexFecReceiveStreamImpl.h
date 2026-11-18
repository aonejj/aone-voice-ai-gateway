/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 *  Modified by kimi.
 *  This file is a modified replacement for
 *  call/flex_fec_receive_stream_impl.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#ifndef __RTC_FLEX_FEC_RECEIVE_STREAM_IMPL_H__
#define __RTC_FLEX_FEC_RECEIVE_STREAM_IMPL_H__

#include <memory>

#include "call/flexfec_receive_stream.h"
#include "call/rtp_packet_sink_interface.h"
#include "system_wrappers/include/clock.h"

#include "../modules/rtp_rtcp/source/RTCRtpRtcpRecvImpl.h"


namespace webrtc {

class FlexfecReceiver;
class ReceiveStatistics;
class RecoveredPacketReceiver;
class RtcpRttStats;
class RtpPacketReceived;
class RtpRtcp;
class RtpStreamReceiverControllerInterface;
class RtpStreamReceiverInterface;

class RTCFlexFecReceiveStreamImpl : public FlexfecReceiveStream {
public:
	RTCFlexFecReceiveStreamImpl(
		Clock* clock,
		RtpStreamReceiverControllerInterface* receiver_controller,
		const Config& config,
		RecoveredPacketReceiver* recovered_packet_receiver,
		RtcpRttStats* rtt_stats);
	~RTCFlexFecReceiveStreamImpl() override;

	// RtpPacketSinkInterface.
	void OnRtpPacket(const RtpPacketReceived& packet) override;

	Stats GetStats() const override;
	const Config& GetConfig() const override;

private:
	// Config.
	const Config config_;

	// Erasure code interfacing.
	const std::unique_ptr<FlexfecReceiver> receiver_;

	// RTCP reporting.
	const std::unique_ptr<ReceiveStatistics> rtp_receive_statistics_;
	const std::unique_ptr<RTCRtpRtcpRecvImpl> rtp_rtcp_;

	std::unique_ptr<RtpStreamReceiverInterface> rtp_stream_receiver_;
};

}

#endif	// __RTC_FLEX_FEC_RECEIVE_STREAM_IMPL_H__