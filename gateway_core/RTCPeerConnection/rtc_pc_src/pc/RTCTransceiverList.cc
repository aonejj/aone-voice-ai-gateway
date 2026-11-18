/*
*  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
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
 *  pc/transceiver_list.cc.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include "rtc_base/checks.h"

#include "RTCTransceiverList.h"

namespace webrtc {

void RTCTransceiverStableState::set_newly_created() {
	RTC_DCHECK(!has_m_section_);
	newly_created_ = true;
}

void RTCTransceiverStableState::SetMSectionIfUnset(
						absl::optional<std::string> mid,
						absl::optional<size_t> mline_index) {
	if (!has_m_section_) {
		mid_ = mid;
		mline_index_ = mline_index;
		has_m_section_ = true;
	}
}

void RTCTransceiverStableState::SetRemoteStreamIdsIfUnset(
							const std::vector<std::string>& ids) {
	if (!remote_stream_ids_.has_value()) {
		remote_stream_ids_ = ids;
	}
}

RTCRtpTransceiverProxyRefPtr RTCTransceiverList::FindBySender(
				rtc::scoped_refptr<RTCRtpSenderInterface> sender) const {
	for (auto transceiver : transceivers_) {
		if (transceiver->sender() == sender) {
			return transceiver;
		}
	}
	return nullptr;
}

RTCRtpTransceiverProxyRefPtr RTCTransceiverList::FindByMid(
									const std::string& mid) const {
	for (auto transceiver : transceivers_) {
		if (transceiver->mid() == mid) {
			return transceiver;
		}
	}
	return nullptr;
}

RTCRtpTransceiverProxyRefPtr RTCTransceiverList::FindByMLineIndex(
												size_t mline_index) const {
	for (auto transceiver : transceivers_) {
		if (transceiver->internal()->mline_index() == mline_index) {
			return transceiver;
		}
	}
	return nullptr;
}

}  // namespace webrtc
