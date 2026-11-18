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

#ifndef __RTC_PC_VIDEO_TRACK_H__
#define __RTC_PC_VIDEO_TRACK_H__

#include <string>


#include "api/media_stream_track.h"
#include "api/scoped_refptr.h"

#include "rtc_base/thread.h"
#include "rtc_base/thread_annotations.h"
#include "rtc_base/thread_checker.h"

#include "../../src_update/api/_media_stream_interface.h"

namespace webrtc {

class RTCVideoTrack : public MediaStreamTrack<VideoTrackInterface>,
	public ObserverInterface {
public:
	static rtc::scoped_refptr<RTCVideoTrack> Create(
		const std::string& label,
		VideoTrackSourceInterface* source,
		rtc::Thread* worker_thread);

	VideoTrackSourceInterface* GetSource() const override {
		return video_source_.get();
	}
	bool set_enabled(bool enable) override;
	std::string kind() const override;

protected:
	RTCVideoTrack(const std::string& id,
		VideoTrackSourceInterface* video_source,
		rtc::Thread* worker_thread);
	~RTCVideoTrack();

private:
	void OnChanged() override;

	rtc::Thread* const worker_thread_;
	rtc::ThreadChecker signaling_thread_checker_;
	rtc::scoped_refptr<VideoTrackSourceInterface> video_source_;
};

}  // namespace webrtc

#endif // __RTC_PC_VIDEO_TRACK_H__