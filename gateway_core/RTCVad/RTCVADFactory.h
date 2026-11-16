#ifndef __RTC_VAD_FACTORY_H__
#define __RTC_VAD_FACTORY_H__

#include <memory>
#include "RTCVADInterface.h"

class RTCVADFactory {
public:
    static std::unique_ptr<RTCVADInterface> Create();
};

#endif // __RTC_VAD_FACTORY_H__