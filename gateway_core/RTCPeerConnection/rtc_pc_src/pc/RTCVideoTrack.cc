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
 *  pc/video_track.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include <string>
#include <vector>

#include "api/notifier.h"
#include "rtc_base/checks.h"
#include "rtc_base/location.h"
#include "rtc_base/ref_counted_object.h"

#include "RTCVideoTrack.h"

namespace webrtc {

RTCVideoTrack::RTCVideoTrack(const std::string& label,
							 VideoTrackSourceInterface* video_source,
							 rtc::Thread* worker_thread)
	: MediaStreamTrack<VideoTrackInterface>(label),
	worker_thread_(worker_thread),
	video_source_(video_source) {
	video_source_->RegisterObserver(this);
}

RTCVideoTrack::~RTCVideoTrack() {
	video_source_->UnregisterObserver(this);
}

std::string RTCVideoTrack::kind() const {
	return kVideoKind;
}


bool RTCVideoTrack::set_enabled(bool enable) {
	RTC_DCHECK(signaling_thread_checker_.IsCurrent());
	return MediaStreamTrack<VideoTrackInterface>::set_enabled(enable);
}

void RTCVideoTrack::OnChanged() {
	RTC_DCHECK(signaling_thread_checker_.IsCurrent());
	if (video_source_->state() == MediaSourceInterface::kEnded) {
		set_state(kEnded);
	}
	else {
		set_state(kLive);
	}
}

rtc::scoped_refptr<RTCVideoTrack> RTCVideoTrack::Create(
	const std::string& id,
	VideoTrackSourceInterface* source,
	rtc::Thread* worker_thread) {
	rtc::RefCountedObject<RTCVideoTrack>* track =
		new rtc::RefCountedObject<RTCVideoTrack>(id, source, worker_thread);
	return track;
}

}  // namespace webrtc
