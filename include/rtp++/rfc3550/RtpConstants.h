#pragma once
#include <cstdint>

static const uint8_t PT_RTCP_AVB              = 208;
static const uint8_t PT_RTCP_RSI              = 209;
static const uint8_t PT_RTCP_PORT_MAPPING     = 210;

/**
 * @summary The ntp soffset in seconds from 1970 to 1900
 */
static const uint32_t NTP_SECONDS_OFFSET = 2208988800U;

/**
 * @summary Recommended by Perkins: participants are removed from the session database once this timeout has expired
 */
static const uint32_t BYE_TIMEOUT_SECONDS = 2;

/**
 * @summary RFC3550 6.3.7 if there are more participants, bye reconsideration should be used
 */
static const uint32_t RFC3550_IMMEDIATE_BYE_LIMIT = 50;
