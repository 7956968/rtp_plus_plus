#pragma once
#include <cstdint>
#include <sstream>
#include <string>

namespace rtp_plus_plus
{
namespace rfc2326
{

// RTP-Info: url=rtsp://foo.com/bar.avi/streamid=0;seq=45102,
// url=rtsp://foo.com/bar.avi/streamid=1;seq=30211

class RtpInfo
{
public:
  RtpInfo(const std::string& sRtspUri)
    :m_sRtspUri(sRtspUri),
      m_uiStartSN(0),
      m_uiRtpTs(0)
  {

  }

  RtpInfo(const std::string& sRtspUri, uint16_t uiSN, uint16_t uiRtpTs)
    :m_sRtspUri(sRtspUri),
      m_uiStartSN(uiSN),
      m_uiRtpTs(uiRtpTs)
  {

  }

  std::string getRtspUri() const { return m_sRtspUri; }
  void setRtspUri(const std::string& sRtspUri) { m_sRtspUri = sRtspUri ; }

  uint16_t getStartSN() const { return m_uiStartSN; }
  void setStartSN(uint16_t uiSN ) { m_uiStartSN = uiSN; }

  uint32_t getRtpTs() const { return m_uiRtpTs; }
  void setRtpTs(uint32_t uiRtpTs ) { m_uiRtpTs = uiRtpTs; }


  std::string toString() const
  {
    std::ostringstream rtpInfo;
    rtpInfo << "url=" << m_sRtspUri << ";seq=" << m_uiStartSN << ";rtptime=" << m_uiRtpTs;
    return rtpInfo.str();
  }

private:
  std::string m_sRtspUri;
  uint16_t m_uiStartSN;
  uint32_t m_uiRtpTs;
};

} // rfc2326
} // rtp_plus_plus
