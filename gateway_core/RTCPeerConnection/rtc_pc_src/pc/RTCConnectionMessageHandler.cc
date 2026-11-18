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

#include "api/scoped_refptr.h"
#include "rtc_base/checks.h"
#include "rtc_base/location.h"
#include "rtc_base/synchronization/sequence_checker.h"

#include "RTCConnectionMessageHandler.h"

#include "../rtc_pc_src/RTCFunctionTracer.h"

namespace webrtc {

enum {
	MSG_SET_SESSIONDESCRIPTION_SUCCESS = 0,
	MSG_SET_SESSIONDESCRIPTION_FAILED,
	MSG_CREATE_SESSIONDESCRIPTION_FAILED,
	MSG_GETSTATS,
	MSG_REPORT_USAGE_PATTERN,
};

struct SetSessionDescriptionMsg : public rtc::MessageData {
	explicit SetSessionDescriptionMsg(
		webrtc::SetSessionDescriptionObserver* observer)
		: observer(observer) {}

	rtc::scoped_refptr<webrtc::SetSessionDescriptionObserver> observer;
	RTCError error;
};

struct CreateSessionDescriptionMsg : public rtc::MessageData {
	explicit CreateSessionDescriptionMsg(
		webrtc::CreateSessionDescriptionObserver* observer)
		: observer(observer) {}

	rtc::scoped_refptr<webrtc::CreateSessionDescriptionObserver> observer;
	RTCError error;
};

struct RequestUsagePatternMsg : public rtc::MessageData {
	explicit RequestUsagePatternMsg(std::function<void()> func)
		: function(func) {}
	std::function<void()> function;
};

RTCConnectionMessageHandler::~RTCConnectionMessageHandler() {
	// Process all pending notifications in the message queue. If we don't do
	// this, requests will linger and not know they succeeded or failed.
	rtc::MessageList list;
	signaling_thread()->Clear(this, rtc::MQID_ANY, &list);
	for (auto& msg : list) {
		if (msg.message_id == MSG_CREATE_SESSIONDESCRIPTION_FAILED) {
			// Processing CreateOffer() and CreateAnswer() messages ensures their
			// observers are invoked even if the PeerConnection is destroyed early.
			OnMessage(&msg);
		}
		else {
			// TODO(hbos): Consider processing all pending messages. This would mean
			// that SetLocalDescription() and SetRemoteDescription() observers are
			// informed of successes and failures; this is currently NOT the case.
			delete msg.pdata;
		}
	}
}

void RTCConnectionMessageHandler::OnMessage(rtc::Message* msg) {
	RTC_DCHECK_RUN_ON(signaling_thread());
	switch (msg->message_id) {
	case MSG_SET_SESSIONDESCRIPTION_SUCCESS: {
		RTC_CLASS_FUNCTION_TRACER("RTCConnectionMessageHandler::OnMessage MSG_SET_SESSIONDESCRIPTION_SUCCESS", this);
		SetSessionDescriptionMsg* param =
			static_cast<SetSessionDescriptionMsg*>(msg->pdata);
		param->observer->OnSuccess();
		delete param;
		break;
	}
	case MSG_SET_SESSIONDESCRIPTION_FAILED: {
		RTC_CLASS_FUNCTION_TRACER("RTCConnectionMessageHandler::OnMessage MSG_SET_SESSIONDESCRIPTION_FAILED", this);
		SetSessionDescriptionMsg* param =
			static_cast<SetSessionDescriptionMsg*>(msg->pdata);
		param->observer->OnFailure(std::move(param->error));
		delete param;
		break;
	}
	case MSG_CREATE_SESSIONDESCRIPTION_FAILED: {
		RTC_CLASS_FUNCTION_TRACER("RTCConnectionMessageHandler::OnMessage MSG_CREATE_SESSIONDESCRIPTION_FAILED", this);
		CreateSessionDescriptionMsg* param =
			static_cast<CreateSessionDescriptionMsg*>(msg->pdata);
		param->observer->OnFailure(std::move(param->error));
		delete param;
		break;
	}
	case MSG_GETSTATS: {
		break;
	}
	case MSG_REPORT_USAGE_PATTERN: {
		break;
	}
	default:
		RTC_NOTREACHED() << "Not implemented";
		break;
	}
}

void RTCConnectionMessageHandler::PostSetSessionDescriptionSuccess(
	SetSessionDescriptionObserver* observer) {
	RTC_CLASS_FUNCTION_TRACER("RTCConnectionMessageHandler::PostSetSessionDescriptionSuccess", this);
	SetSessionDescriptionMsg* msg = new SetSessionDescriptionMsg(observer);
	signaling_thread()->Post(RTC_FROM_HERE, this,
		MSG_SET_SESSIONDESCRIPTION_SUCCESS, msg);
}

void RTCConnectionMessageHandler::PostSetSessionDescriptionFailure(
	SetSessionDescriptionObserver* observer,
	RTCError&& error) {
	RTC_CLASS_FUNCTION_TRACER("RTCConnectionMessageHandler::PostSetSessionDescriptionFailure", this);
	RTC_DCHECK(!error.ok());
	SetSessionDescriptionMsg* msg = new SetSessionDescriptionMsg(observer);
	msg->error = std::move(error);
	signaling_thread()->Post(RTC_FROM_HERE, this,
		MSG_SET_SESSIONDESCRIPTION_FAILED, msg);
}

void RTCConnectionMessageHandler::PostCreateSessionDescriptionFailure(
	CreateSessionDescriptionObserver* observer,
	RTCError error) {
	RTC_CLASS_FUNCTION_TRACER("RTCConnectionMessageHandler::PostCreateSessionDescriptionFailure", this);
	RTC_DCHECK(!error.ok());
	CreateSessionDescriptionMsg* msg = new CreateSessionDescriptionMsg(observer);
	msg->error = std::move(error);
	signaling_thread()->Post(RTC_FROM_HERE, this,
		MSG_CREATE_SESSIONDESCRIPTION_FAILED, msg);
}

}