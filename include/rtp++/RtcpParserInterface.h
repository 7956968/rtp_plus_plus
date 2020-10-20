#pragma once
#include <cstdint>
#include <cpputil/IBitStream.h>
#include <rtp++/RtcpPacketBase.h>

namespace rtp_plus_plus
{

class RtcpParserInterface
{
public:
  typedef std::unique_ptr<RtcpParserInterface> ptr;

  virtual ~RtcpParserInterface()
  {
  }

  /**
    * This method returns if the parser can handle the passed in RTCP type
    */
  virtual bool canHandleRtcpType(uint32_t uiType) const = 0;

  /**
    * Parsing method where the calling code has already partially parsed the RTCP header and passes in that information
    */
  virtual RtcpPacketBase::ptr parseRtcpPacket(IBitStream& ib, uint32_t uiVersion, uint32_t uiPadding, uint32_t uiTypeSpecific, uint32_t uiPayload, uint32_t uiLengthInWords) = 0;
};

}
