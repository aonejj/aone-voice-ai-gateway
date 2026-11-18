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
 *  pc/video_rtp_track_source.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#ifndef __RTC_PC_VIDEO_RTC_TRACK_SOURCE_H__
#define __RTC_PC_VIDEO_RTC_TRACK_SOURCE_H__

#include <vector>

#include "rtc_base/callback.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/system/no_unique_address.h"

#include "RTCVideoTrackSource.h"

namespace webrtc {

// Video track source in use by VideoRtpReceiver
class RTCVideoRtpTrackSource : public RTCVideoTrackSource {
public:
	class Callback {
	public:
		virtual ~Callback() = default;

		// Called when a keyframe should be generated
		virtual void OnGenerateKeyFrame() = 0;
	};

	explicit RTCVideoRtpTrackSource(Callback* callback);

	// Call before the object implementing Callback finishes it's destructor. No
	// more callbacks will be fired after completion. Must be called on the
	// worker thread
	void ClearCallback();

	// Returns true. This method can be called on any thread.
	bool SupportsEncodedOutput() const override;

	// Generates a key frame. Must be called on the worker thread.
	void GenerateKeyFrame() override;

private:
	RTC_NO_UNIQUE_ADDRESS SequenceChecker worker_sequence_checker_;
	Callback* callback_ RTC_GUARDED_BY(worker_sequence_checker_);

	RTC_DISALLOW_COPY_AND_ASSIGN(RTCVideoRtpTrackSource);
};

}  // namespace webrtc


#endif // __RTC_PC_VIDEO_RTC_TRACK_SOURCE_H__