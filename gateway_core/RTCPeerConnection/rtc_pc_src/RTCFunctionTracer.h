//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_FUNCTION_TRACER_H__
#define __RTC_FUNCTION_TRACER_H__

#include <string>
#include "rtc_base/logging.h"


#ifdef NDEBUG
#define RTC_CLASS_FUNCTION_TRACER(X, P)
#else
#define RTC_CLASS_FUNCTION_TRACER(X, P)										\
	RTC_LOG(LS_INFO) << "[" << P <<"] "<< X << "+++";						\
	struct FunctionScope {													\
		FunctionScope(std::string fname, void *p) : _fname(fname), _p(p) {} \
		~FunctionScope() {													\
			RTC_LOG(LS_INFO) << "[" << _p <<"] "<< _fname << "---";			\
		}																	\
		std::string _fname;													\
		void* _p;															\
	};																		\
	FunctionScope log_scope(X, (void*)P);
#endif	

#endif // __RTC_FUNCTION_LOGGER_H__