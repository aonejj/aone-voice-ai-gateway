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
 *  pc/video_track_source.cc.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include "rtc_base/checks.h"

#include "RTCVideoTrackSource.h"

namespace webrtc {

RTCVideoTrackSource::RTCVideoTrackSource(bool remote)
	: state_(kInitializing), remote_(remote) {
	worker_thread_checker_.Detach();
}

void RTCVideoTrackSource::SetState(SourceState new_state) {
	if (state_ != new_state) {
		state_ = new_state;
		FireOnChanged();
	}
}

}