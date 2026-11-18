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
 *  pc/video_rtp_track_source.cc.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include "RTCVideoRtpTrackSource.h"

namespace webrtc {

RTCVideoRtpTrackSource::RTCVideoRtpTrackSource(Callback* callback)
	: RTCVideoTrackSource(true /* remote */), callback_(callback) {
	worker_sequence_checker_.Detach();
}

void RTCVideoRtpTrackSource::ClearCallback() {
	RTC_DCHECK_RUN_ON(&worker_sequence_checker_);
	callback_ = nullptr;
}


bool RTCVideoRtpTrackSource::SupportsEncodedOutput() const {
	return true;
}

void RTCVideoRtpTrackSource::GenerateKeyFrame() {
	RTC_DCHECK_RUN_ON(&worker_sequence_checker_);
	if (callback_) {
		callback_->OnGenerateKeyFrame();
	}
}

}  // namespace webrtc

