#include "_deprecate_defines.h"
#include "rtc_base/strings/string_builder.h"
#include <time.h>

void _dump_packet_trace_info(const char* log_prefix, const std::vector<PACKET_TRACE_INFO>& trace_vector) {
    if(trace_vector.empty()) {
        return;
    }

    int64_t now_us = rtc::TimeMicros();
    size_t trace_size = trace_vector.size();

    char buf[4096];
	rtc::SimpleStringBuilder ss(buf);

     ss << log_prefix <<" ";
    if(trace_size > 1) {
        for (size_t i = 0; i < trace_size -1; i++) {
            const PACKET_TRACE_INFO& info = trace_vector[i];
            const PACKET_TRACE_INFO& next_info = trace_vector[i + 1];

            ss << info._trace_info << "~" << next_info._trace_info << " " << (next_info._time_us - info._time_us) <<" "; 
        }
        ss << trace_vector[0]._trace_info << "~" << trace_vector[trace_size - 1]._trace_info << " " << (trace_vector[trace_size - 1]._time_us - trace_vector[0]._time_us);
    } 

    RTC_LOG(LS_INFO) << ss.str();
}    
    
int64_t ThreadCpuTimeUs() {
  struct timespec ts;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
  return static_cast<int64_t>(ts.tv_sec) * 1000000LL +
         ts.tv_nsec / 1000;
}
