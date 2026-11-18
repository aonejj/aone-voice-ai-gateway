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
 *  api/media_stream_interface.cc.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include "api/media_types.h"

#include "_media_stream_interface.h"

namespace webrtc {

const char* const MediaStreamTrackInterface::kVideoKind =
    cricket::kMediaTypeVideo;
const char* const MediaStreamTrackInterface::kAudioKind =
    cricket::kMediaTypeAudio;


bool AudioTrackInterface::GetSignalLevel(int* level) {
  return false;
}

const cricket::AudioOptions AudioSourceInterface::options() const {
  return {};
}

}  // namespace webrtc
