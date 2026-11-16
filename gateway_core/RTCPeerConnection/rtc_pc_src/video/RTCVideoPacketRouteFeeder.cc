//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////
#include "../rtc_pc_src/_deprecate_defines.h"

#include "rtc_base/location.h"
#include "rtc_base/logging.h"
#include "rtc_base/time_utils.h"
#include "modules/rtp_rtcp/source/rtp_header_extensions.h"
#include "RTCVideoPacketRouteFeeder.h"

#include <pthread.h>

#include "../rtc_pc_src/RTCFunctionTracer.h"

namespace webrtc {

RTCVideoPacketRouteFeeder::RTCVideoPacketRouteFeeder(
	Clock* clock)
	: clock_(clock),
	  source_(nullptr),
	  sink_(nullptr), 
	  is_run_(false) {

	RTC_CLASS_FUNCTION_TRACER("RTCVideoPacketRouteFeeder::RTCVideoPacketRouteFeeder", this);
} 

RTCVideoPacketRouteFeeder::~RTCVideoPacketRouteFeeder() {
	RTC_CLASS_FUNCTION_TRACER("RTCVideoPacketRouteFeeder::~RTCVideoPacketRouteFeeder", this);
}

void RTCVideoPacketRouteFeeder::SetSource(
	rtc::RTCVideoPacketRouteSourceInterface<std::unique_ptr<RtpPacketReceived>>* source) {
	RTC_CLASS_FUNCTION_TRACER("RTCVideoPacketRouteFeeder::SetSource", this);
	rtc::RTCVideoPacketRouteSourceInterface<std::unique_ptr<RtpPacketReceived>>* old_source = source_;

	MutexLock lock(&_source_lock);

	source_ = source;

	if (old_source != source && old_source) {
		old_source->RemovePacketRoutingSink(this);
	}

	if (!source)
		return;

	source->AddPacketRoutingSink(this);
}

void RTCVideoPacketRouteFeeder::SetSink(RTCVideoPacketRoutingSink *sink) {
	RTC_CLASS_FUNCTION_TRACER("RTCVideoPacketRouteFeeder::SetSink", this);
	MutexLock lock(&_sink_lock);
	sink_ = sink;
}

void RTCVideoPacketRouteFeeder::Start() {
	RTC_CLASS_FUNCTION_TRACER("RTCVideoPacketRouteFeeder::Start", this);
	MutexLock lock(&_run_lock);
	is_run_ = true;
}

void RTCVideoPacketRouteFeeder::Stop() {
	RTC_CLASS_FUNCTION_TRACER("RTCVideoPacketRouteFeeder::Stop", this);
	MutexLock lock(&_run_lock);
	is_run_ = false;
}

void RTCVideoPacketRouteFeeder::GenerateKeyFrame() {
	{
		MutexLock lock(&_run_lock);
		if (!is_run_) {
			return;
		}
	}

	MutexLock lock(&_source_lock);
	if(source_ != nullptr) {
		source_->GenerateKeyFrame();
	}
}

void RTCVideoPacketRouteFeeder::OnFeedPacketPtr(std::unique_ptr<RtpPacketReceived> packet) {
	RTC_LOG(LS_INFO) << "kimi_thread_check_trace RTCVideoPacketRouteFeeder tid " << (unsigned int)gettid();
//	RTC_LOG(LS_INFO) <<"idx_trace packet->_debug_packet_trace_idx "<<packet->_debug_packet_trace_idx;
#ifdef __ENABLE_DEBUGING_PACKET_PATH_TRACE__
	PACKET_TRACE_INFO dinfo;
  	dinfo._trace_info = "v_route_feeder_on_feed_rtp_packet";
  	dinfo._time_us = rtc::TimeMicros();
  	packet->_debug_packet_trace_vector.push_back(dinfo);	
#endif

	{
		MutexLock lock(&_run_lock);
		if (!is_run_) {
			return;
		}
	}

	MutexLock lock(&_sink_lock);
	if (sink_ != nullptr) {
		sink_->OnRoutingPacket(std::move(packet));
	}
}

}	// namespace webrtc