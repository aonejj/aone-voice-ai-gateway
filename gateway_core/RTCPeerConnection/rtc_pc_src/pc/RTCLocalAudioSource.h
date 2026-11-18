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
 *  pc/local_audio_source.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#ifndef __RTC_LOCAL_AUDIO_SOURCE_H__
#define __RTC_LOCAL_AUDIO_SOURCE_H__

#include "api/audio_options.h"
#include "api/notifier.h"
#include "api/scoped_refptr.h"

#include "../../src_update/api/_media_stream_interface.h"

namespace webrtc {

class RTCLocalAudioSource : public Notifier<AudioSourceInterface> {
public:
	static rtc::scoped_refptr<RTCLocalAudioSource> Create(
		const cricket::AudioOptions* audio_options);

	SourceState state() const override { return kLive; }
	bool remote() const override { return false; }

	const cricket::AudioOptions options() const override { return options_; }

	void AddSink(AudioTrackSinkInterface* sink) override {}
	void RemoveSink(AudioTrackSinkInterface* sink) override {}

protected:
	RTCLocalAudioSource() {}
	~RTCLocalAudioSource() override {}

private:
	void Initialize(const cricket::AudioOptions *audio_options);

private:
	cricket::AudioOptions options_;
};

}


#endif // __RTC_LOCAL_AUDIO_SOURCE_H__