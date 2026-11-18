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
 *  api/rtp_receiver_interface.cc
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include "RTCRtpReceiverInterface.h"


namespace webrtc {

std::vector<std::string> RTCRtpReceiverInterface::stream_ids() const {
	return{};
}

std::vector<rtc::scoped_refptr<MediaStreamInterface>>
RTCRtpReceiverInterface::streams() const {
	return{};
}

std::vector<RtpSource> RTCRtpReceiverInterface::GetSources() const {
	return{};
}

rtc::scoped_refptr<DtlsTransportInterface>
RTCRtpReceiverInterface::dtls_transport() const {
	return nullptr;
}

}  // namespace webrtc
