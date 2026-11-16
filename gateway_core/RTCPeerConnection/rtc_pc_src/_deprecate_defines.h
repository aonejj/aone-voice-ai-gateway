//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __DEPRECATE_DEFINES_H__
#define __DEPRECATE_DEFINES_H__

#include <string>
#include <vector>
#include "rtc_base/time_utils.h"
#include "rtc_base/logging.h"

typedef struct _packet_trace_info {
    std::string _trace_info;
    int64_t _time_us = 0;
} PACKET_TRACE_INFO;    

void _dump_packet_trace_info(const char* log_prefix, const std::vector<PACKET_TRACE_INFO>& trace_vector);

int64_t ThreadCpuTimeUs();

#define __ENABLE_DEBUGING_PACKET_PATH_TRACE__

const int RTC_kDefaultMinBitrateBps = 100 * 1000;
const int RTC_kDefualMaxBitrateBps = 3000 * 1000;

#endif // __DEPRECATE_DEFINES_H__