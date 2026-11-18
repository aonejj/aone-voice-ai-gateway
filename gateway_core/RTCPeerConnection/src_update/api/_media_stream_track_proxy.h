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
 *  api/media_stream_track_proxy.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#ifndef __RTC_UPDATE_API_MEDIA_STREAM_TRACK_PROXY_H__
#define __RTC_UPDATE_API_MEDIA_STREAM_TRACK_PROXY_H__

#include <string>

#include "api/proxy.h"
#include "api/make_ref_counted.h"
#include "_media_stream_interface.h"

namespace webrtc {

BEGIN_PRIMARY_PROXY_MAP(AudioTrack)
PROXY_PRIMARY_THREAD_DESTRUCTOR()
BYPASS_PROXY_CONSTMETHOD0(std::string, kind)
BYPASS_PROXY_CONSTMETHOD0(std::string, id)
PROXY_CONSTMETHOD0(TrackState, state)
PROXY_CONSTMETHOD0(bool, enabled)
PROXY_CONSTMETHOD0(AudioSourceInterface*, GetSource)
PROXY_METHOD1(void, AddSink, AudioTrackSinkInterface*)
PROXY_METHOD1(void, RemoveSink, AudioTrackSinkInterface*)
PROXY_METHOD1(bool, GetSignalLevel, int*)
PROXY_METHOD1(bool, set_enabled, bool)
PROXY_METHOD1(void, RegisterObserver, ObserverInterface*)
PROXY_METHOD1(void, UnregisterObserver, ObserverInterface*)
END_PROXY_MAP(AudioTrack)

BEGIN_PROXY_MAP(VideoTrack)
PROXY_PRIMARY_THREAD_DESTRUCTOR()
BYPASS_PROXY_CONSTMETHOD0(std::string, kind)
BYPASS_PROXY_CONSTMETHOD0(std::string, id)
PROXY_CONSTMETHOD0(TrackState, state)
PROXY_CONSTMETHOD0(bool, enabled)
PROXY_METHOD1(bool, set_enabled, bool)
PROXY_CONSTMETHOD0(VideoTrackSourceInterface*, GetSource)

PROXY_METHOD1(void, RegisterObserver, ObserverInterface*)
PROXY_METHOD1(void, UnregisterObserver, ObserverInterface*)
END_PROXY_MAP(VideoTrack)

}  // namespace webrtc

#endif  // __RTC_UPDATE_API_MEDIA_STREAM_TRACK_PROXY_H__
