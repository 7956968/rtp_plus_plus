#pragma once

#include <stdint.h>
#include "ErrorCodes.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Interface to RTP 
void* createInstance();

void deleteInstance(void* instance);

int sendRtpPacket(void* instance, uint8_t uiPayloadType, uint8_t* pBuffer, uint32_t uiBufferSize, uint64_t uiPts, uint64_t uiDts);

#ifdef __cplusplus
}
#endif

