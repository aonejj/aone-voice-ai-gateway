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
 *  api/rtp_transceiver_interface.h
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#ifndef __RTC_RTP_TRANSCEIVER_INTERFACE_H__
#define __RTC_RTP_TRANSCEIVER_INTERFACE_H__

#include <string>
#include <vector>

#include "absl/types/optional.h"

#include "api/array_view.h"
#include "api/media_types.h"
#include "api/rtp_parameters.h"
#include "api/rtc_error.h"
#include "api/rtp_transceiver_direction.h"
#include "api/scoped_refptr.h"
#include "rtc_base/ref_count.h"
#include "rtc_base/system/rtc_export.h"
#include "rtc_base/deprecation.h"

#include "RTCRtpSenderInterface.h"
#include "RTCRtpReceiverInterface.h"


namespace webrtc {
	
struct RTC_EXPORT RTCRtpTransceiverInit final {
	RTCRtpTransceiverInit();
	RTCRtpTransceiverInit(const RTCRtpTransceiverInit&);
	~RTCRtpTransceiverInit();
	// Direction of the RtpTransceiver. See RtpTransceiverInterface::direction().
	RtpTransceiverDirection direction = RtpTransceiverDirection::kSendRecv;

	// The added RtpTransceiver will be added to these streams.
	std::vector<std::string> stream_ids;

	// TODO(bugs.webrtc.org/7600): Not implemented.
	std::vector<RtpEncodingParameters> send_encodings;
};


class RTC_EXPORT RTCRtpTransceiverInterface : public rtc::RefCountInterface {
public:
	virtual cricket::MediaType media_type() const = 0;
	virtual absl::optional<std::string> mid() const = 0;

	virtual rtc::scoped_refptr<RTCRtpSenderInterface> sender() const = 0;
	virtual rtc::scoped_refptr<RTCRtpReceiverInterface> receiver() const = 0;

	virtual uint32_t receiver_size() const = 0;
	virtual rtc::scoped_refptr<RTCRtpReceiverInterface> receiver(uint32_t idx) const = 0;

	virtual bool stopped() const = 0;
	virtual bool stopping() const;

	virtual RtpTransceiverDirection direction() const = 0;

	RTC_DEPRECATED virtual void SetDirection(
		RtpTransceiverDirection new_direction);
	virtual RTCError SetDirectionWithError(RtpTransceiverDirection new_direction);

	virtual absl::optional<RtpTransceiverDirection> current_direction() const = 0;

	virtual absl::optional<RtpTransceiverDirection> fired_direction() const;	

	virtual RTCError StopStandard();
	virtual void StopInternal();
	RTC_DEPRECATED virtual void Stop();

	virtual RTCError SetCodecPreferences(
		rtc::ArrayView<RtpCodecCapability> codecs);
	virtual std::vector<RtpCodecCapability> codec_preferences() const;

	virtual std::vector<RtpHeaderExtensionCapability> HeaderExtensionsToOffer()
		const;
	virtual std::vector<RtpHeaderExtensionCapability> HeaderExtensionsNegotiated()
		const;
	virtual webrtc::RTCError SetOfferedRtpHeaderExtensions(
		rtc::ArrayView<const RtpHeaderExtensionCapability>
		header_extensions_to_offer);

protected:
	~RTCRtpTransceiverInterface() override = default;
};

}	// namespace webrtc


#endif // __RTC_RTP_TRANSCEIVER_INTERFACE_H__