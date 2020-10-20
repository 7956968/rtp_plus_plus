#pragma once

#include <cstdint>
#include <cpputil/Buffer.h>

namespace rtp_plus_plus
{

/**
  * This class adds an NTP timestamp to the Buffer
  * Note that there is no virtual destructor in Buffer
  * since it is not intended to be allocated dynamically
  */
class NetworkPacket : public Buffer
{
public:

  NetworkPacket()
    :m_tNtpArrival(0)
  {}

  NetworkPacket(uint8_t* ptr, size_t size)
    :Buffer(ptr, size),
    m_tNtpArrival(0)
  {}

  NetworkPacket(uint64_t tArrival)
    :m_tNtpArrival(tArrival)
  {}

  NetworkPacket(uint8_t* ptr, size_t size, uint64_t tArrival)
    :Buffer(ptr, size),
    m_tNtpArrival(tArrival)
  {}

  uint64_t getNtpArrivalTime() const { return m_tNtpArrival; }
  void setNtpArrivalTime(uint64_t tArrival) { m_tNtpArrival = tArrival; }

private:
  uint64_t m_tNtpArrival;
};

}
