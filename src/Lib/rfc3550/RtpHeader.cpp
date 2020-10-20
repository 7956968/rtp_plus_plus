#include "CorePch.h"
#include <rtp++/rfc3550/RtpHeader.h>

namespace rtp_plus_plus
{
namespace rfc3550
{

std::ostream& operator<<( std::ostream& ostr, const RtpHeader& rtpHeader )
{
  ostr << "RTP header:";
  ostr << " V: "    << (int)rtpHeader.m_uiVersion;
  ostr << " P: "    << rtpHeader.m_bPadding;
  ostr << " X: "    << rtpHeader.m_bExtension;
  ostr << " CC: "   << (int)rtpHeader.m_uiCC;
  ostr << " M: "    << rtpHeader.m_bMarker;
  ostr << " PT: "   << (int)rtpHeader.m_uiPT;
  ostr << " SN: "   << rtpHeader.m_uiSN;
  ostr << " TS: "   << rtpHeader.m_uiRtpTimestamp;
  ostr << " SSRC: " << rtpHeader.m_uiSSRC;
  for (size_t i = 0; i < rtpHeader.m_vCCs.size(); ++i)
  {
    ostr << " CSRC: " << rtpHeader.m_vCCs[i];
  }
  return ostr;
}

OBitStream& operator<<( OBitStream& ob, const RtpHeader& rtpHeader )
{
  ob.write( rtpHeader.getVersion(),         2);
  ob.write( rtpHeader.hasPadding(),         1);
  ob.write( rtpHeader.hasExtension(),       1);
  ob.write( rtpHeader.getCC(),              4);
  ob.write( rtpHeader.isMarkerSet(),        1);
  ob.write( (uint32_t) rtpHeader.getPayloadType(),    7);
  ob.write( rtpHeader.getSequenceNumber(), 16);
  ob.write( rtpHeader.getRtpTimestamp(),   32);
  ob.write( rtpHeader.getSSRC(),           32);

  std::vector<uint32_t> vCSRCs = rtpHeader.getCSRCs();
  std::for_each(vCSRCs.begin(), vCSRCs.end(), [&ob](uint32_t uiCSRC)
  {
    ob.write(uiCSRC,     32);
  } 
  );

  // write extension header
  rfc5285::RtpHeaderExtension extension = rtpHeader.getHeaderExtension();
  if (extension.containsExtensions())
  {
    extension.writeExtensionData(ob);
  }

  return ob;
}

} // rfc3550
} // rtp_plus_plus
