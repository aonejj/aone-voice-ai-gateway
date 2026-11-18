/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
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
 *  call/audio_receiver_stream.cc.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include "RTCAudioReceiveStream.h"


namespace webrtc {

RTCAudioReceiveStream::Config::Config() = default;
RTCAudioReceiveStream::Config::~Config() = default;

RTCAudioReceiveStream::Config::Rtp::Rtp() = default;
RTCAudioReceiveStream::Config::Rtp::~Rtp() = default;

}  // namespace webrtc