#include "RTCVADWebrtcLegacy.h"
#include "RTCVADFactory.h"

std::unique_ptr<RTCVADInterface> RTCVADFactory::Create() {
    return std::make_unique<RTCVADWebrtcLegacy>();
}