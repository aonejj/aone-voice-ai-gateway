//////////////////////////////////////////////////////////////////////////
//
// author : kimi
// desc   : CreateSessionDescriptionObserver Impl class
//
//////////////////////////////////////////////////////////////////////////

#include "rtc_base/logging.h"

#include "RTCCreateSdpObserver.h"
#include "../rtc_pc_src/RTCFunctionTracer.h"

namespace webrtc {

RTCCreateSdpObserver::RTCCreateSdpObserver(RTCSdpObserver *observer) 
	:_observer(observer) {

}

RTCCreateSdpObserver::~RTCCreateSdpObserver() = default;

void RTCCreateSdpObserver::OnSuccess(SessionDescriptionInterface* desc) {
	RTC_CLASS_FUNCTION_TRACER("RTCCreateSdpObserver::OnSuccess", this);
	if (_observer) {
		_observer->OnCreateSuccess(desc);
	}
}

void RTCCreateSdpObserver::OnFailure(RTCError error) {
	RTC_CLASS_FUNCTION_TRACER("RTCCreateSdpObserver::OnFailure", this);
	if (_observer) {
		_observer->OnCreateFailure(error);
	}
}

RTCSetSdpObserver::RTCSetSdpObserver(RTCSdpObserver *observer, bool is_local, std::unique_ptr<SessionDescriptionInterface> desc)
	:_observer(observer),
	 _is_local(is_local),
	 _desc(std::move(desc)) {

}

RTCSetSdpObserver::~RTCSetSdpObserver() = default;

void RTCSetSdpObserver::OnSuccess() {
	RTC_CLASS_FUNCTION_TRACER("RTCSetSdpObserver::OnSuccess", this);
	if (_observer) {
		_observer->OnSetSuccess(_is_local, _desc.get());
	}
}

void RTCSetSdpObserver::OnFailure(RTCError error) {
	RTC_CLASS_FUNCTION_TRACER("RTCSetSdpObserver::OnFailure", this);
	if (_observer) {
		_observer->OnSetFailure(error, _is_local);
	}
}

}
