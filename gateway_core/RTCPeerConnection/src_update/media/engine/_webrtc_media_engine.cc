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
 *  media/engine/webrtc_media_engine.cc.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include "../../../rtc_pc_src/media/engine/RTCWebRTCVideoEngine.h"
#include "../../../rtc_pc_src/media/engine/RTCWebRTCVoiceEngine.h"

#include "_webrtc_media_engine.h"


namespace cricket {

std::unique_ptr<MediaEngineInterface> CreateMediaEngine(
	std::vector<AudioCodec> audio_decoder_codecs,
	std::vector<AudioCodec> audio_encoder_codecs,
	std::vector<VideoCodec> video_decoder_codecs,
	std::vector<VideoCodec> video_encoder_codecs) {
	auto video_engine = std::make_unique<RTCWebRtcVideoEngine>(
		video_decoder_codecs,
		video_encoder_codecs);

		RTC_LOG(LS_INFO) << "codec_trace CreateMediaEngine audio_encoder_codecs size "<<audio_encoder_codecs.size();
	auto audio_engine = std::make_unique<RTCWebRtcVoiceEngine>(
		audio_decoder_codecs,
		audio_encoder_codecs
		);

	return std::make_unique<CompositeMediaEngine>(
		std::move(audio_engine),
		std::move(video_engine));
}

}