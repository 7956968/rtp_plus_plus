#pragma once
#include <iostream>
#include <fstream>
#include <cpputil/Buffer.h>
#include <rtp++/RtpTime.h>

namespace rtp_plus_plus
{

/// FORMAT: | NTP TS (UTC) | size |
class RtpDump
{
public:

  RtpDump(const std::string& sFile)
    :m_out(sFile.c_str(), std::ofstream::out | std::ofstream::binary)
  {
  }

  ~RtpDump()
  {
    m_out.close();
  }

  void dumpRtp(Buffer buffer)
  {
    uint32_t uiSize = buffer.getSize();
    uint64_t uiNtpTime = getNTPTimeStamp();
    m_out.write((char*)&uiNtpTime, sizeof(uint64_t));
    m_out.write((char*)&uiSize, sizeof(uiSize));
    m_out.write((char *)buffer.data(), buffer.getSize());
  }

  void dumpRtcp(Buffer buffer)
  {
    LOG(WARNING) << "TODO: implement";
  }

private:

  std::ofstream m_out;
};

}
