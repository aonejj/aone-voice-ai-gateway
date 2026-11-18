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
 *  pc/video_track_source.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#ifndef __RTC_PC_VIDEO_TRACK_SOURCE_H__
#define __RTC_PC_VIDEO_TRACK_SOURCE_H__

#include "api/notifier.h"
#include "rtc_base/system/rtc_export.h"
#include "rtc_base/thread_checker.h"

#include "../../src_update/api/_media_stream_interface.h"
#include "../../src_update/media/base/_media_channel.h"

namespace webrtc {

class RTC_EXPORT RTCVideoTrackSource : public Notifier<VideoTrackSourceInterface> {
public:
	explicit RTCVideoTrackSource(bool remote);
	void SetState(SourceState new_state);

	SourceState state() const override { return state_; }
	bool remote() const override { return remote_; }
	
	bool GetStats(Stats* stats) override { return false; }
	bool SupportsEncodedOutput() const override { return false; }
	void GenerateKeyFrame() override {}

private:
	rtc::ThreadChecker worker_thread_checker_;
	SourceState state_;
	const bool remote_;
};

}  // namespace webrtc


#endif	// __RTC_PC_VIDEO_TRACK_SOURCE_H__