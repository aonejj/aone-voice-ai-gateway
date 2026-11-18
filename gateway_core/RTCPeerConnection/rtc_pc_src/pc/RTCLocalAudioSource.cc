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
 *  pc/local_audio_source.cc.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include "rtc_base/ref_counted_object.h"

#include "RTCLocalAudioSource.h"

namespace webrtc {

rtc::scoped_refptr<RTCLocalAudioSource> RTCLocalAudioSource::Create(
	const cricket::AudioOptions* audio_options) {
	rtc::scoped_refptr<RTCLocalAudioSource> source(
		new rtc::RefCountedObject<RTCLocalAudioSource>());
	source->Initialize(audio_options);
	return source;
}

void RTCLocalAudioSource::Initialize(const cricket::AudioOptions* audio_options) {
	if (!audio_options)
		return;

	options_ = *audio_options;
}

}