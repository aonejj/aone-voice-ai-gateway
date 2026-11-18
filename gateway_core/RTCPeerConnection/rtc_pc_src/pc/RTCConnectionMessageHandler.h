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
 *  pc/peer_connection_message_handler.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#ifndef __RTC_CONNECTION_MESSAGE_HANDLER_H__
#define __RTC_CONNECTION_MESSAGE_HANDLER_H__

#include <functional>

#include "api/jsep.h"
#include "api/rtc_error.h"
#include "rtc_base/message_handler.h"
#include "rtc_base/thread.h"
#include "rtc_base/thread_message.h"

namespace webrtc {

class RTCConnectionMessageHandler : public rtc::MessageHandler {
public:
	explicit RTCConnectionMessageHandler(rtc::Thread* signaling_thread)
		: signaling_thread_(signaling_thread) {}
	~RTCConnectionMessageHandler();

	// Implements MessageHandler.
	void OnMessage(rtc::Message* msg) override;
	void PostSetSessionDescriptionSuccess(
		SetSessionDescriptionObserver* observer);
	void PostSetSessionDescriptionFailure(SetSessionDescriptionObserver* observer,
		RTCError&& error);
	void PostCreateSessionDescriptionFailure(
		CreateSessionDescriptionObserver* observer,
		RTCError error);

private:
	rtc::Thread* signaling_thread() const { return signaling_thread_; }

	rtc::Thread* const signaling_thread_;
};

}




#endif // __RTC_CONNECTION_MESSAGE_HANDLER_H__