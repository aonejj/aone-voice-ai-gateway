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
 *  media/engine/webrtc_media_engine.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */


#ifndef __RTC_UPDATE_MEDIA_ENGINE_WEBRTC_MEDIA_ENGINE_H__
#define __RTC_UPDATE_MEDIA_ENGINE_WEBRTC_MEDIA_ENGINE_H__


#include "media/base/codec.h"
#include "rtc_base/system/rtc_export.h"
#include "../base/_media_engine.h"

namespace cricket {

RTC_EXPORT std::unique_ptr<MediaEngineInterface> CreateMediaEngine(
	std::vector<AudioCodec> audio_decoder_codecs,
	std::vector<AudioCodec> audio_encoder_codecs,
	std::vector<VideoCodec> video_decoder_codecs,
	std::vector<VideoCodec> video_encoder_codecs);

}


#endif // __RTC_UPDATE_MEDIA_ENGINE_WEBRTC_MEDIA_ENGINE_H__