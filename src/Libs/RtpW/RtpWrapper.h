#pragma once

#include <cstdint>
#include <memory>

#include <boost/thread.hpp>
#include "ErrorCodes.h"
#include <cpputil/Buffer.h>

#include <rtp++/media/MediaStreamParser.h>

// fwd declaration
namespace rtp_plus_plus
{
class SimpleMediaSession;
}

class RtpWrapper
{
public:
  RtpWrapper();
  ~RtpWrapper();

  int start();
  int stop();

  int sendRtpPacket(uint8_t uiPayloadType, uint8_t* pBuffer, uint32_t uiBufferSize, uint64_t uiPts, uint64_t uiDts);

private:
  boost::shared_ptr<rtp_plus_plus::SimpleMediaSession> m_pMediaSession;
  std::unique_ptr<boost::thread> m_pThread;
  std::unique_ptr<rtp_plus_plus::media::MediaStreamParser> m_pMediaStreamParser;
  Buffer m_buffer;
  uint32_t m_uiCurrentPos; /// pointer for new reads
};

