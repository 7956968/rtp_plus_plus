#include "CorePch.h"
#include <rtp++/mprtp/MpRtp.h>
#include <sstream>
#include <tuple>

namespace rtp_plus_plus
{
namespace mprtp
{

const std::string EXTENSION_NAME_MPRTP ="urn:ietf:params:rtp-hdrext:mprtp";

const std::string SETUP_MPRTP = "setup.mprtp";

const std::string DEST_MPRTP_ADDR = "dest_mprtp_addr";
const std::string SRC_MPRTP_ADDR = "src_mprtp_addr";

const unsigned MAX_MISORDER = 500;

const uint8_t SUBFLOW_SPECIFIC_REPORT      = 0;
const uint8_t INTERFACE_ADVERTISEMENT_IPV4 = 1;
const uint8_t INTERFACE_ADVERTISEMENT_IPV6 = 2;
const uint8_t INTERFACE_ADVERTISEMENT_DNS  = 3;

const std::string MPRTP = "mprtp";

bool isMpRtpSession(const RtpSessionParameters& rtpParameters)
{
  return (rtpParameters.hasAttribute(MPRTP));
}

bool isMpRtpSession(const rfc4566::SessionDescription& sessionDescription)
{
  for (size_t i = 0; i < sessionDescription.getMediaDescriptionCount(); ++i)
  {
    const rfc4566::MediaDescription& desc = sessionDescription.getMediaDescription(i);
    if (desc.hasAttribute(MPRTP))
    {
      return true;
    }
  }
  return false;
}

std::string getMpRtpRtspTransportString(const MediaInterfaceDescriptor_t& mediaInterfaceDescriptor)
{
  std::ostringstream ostr;
  for (size_t i = 0; i < mediaInterfaceDescriptor.size(); ++i)
  {
    const AddressTuple_t& addresses = mediaInterfaceDescriptor[i];
    const AddressDescriptor& rtp = std::get<0>(addresses);
    if (i > 0) ostr << ";";
    ostr << (i + 1) << " " << rtp.getAddress() << " " << rtp.getPort();
  }
  return ostr.str();
}

MediaInterfaceDescriptor_t getMpRtpInterfaceDescriptorFromRtspTransportString(const std::string& sMpRtpAddr, bool bRtcpMux)
{
  MediaInterfaceDescriptor_t mediaInterface;
  std::vector<std::string> vSrcMpRtpAddrInterfaces = StringTokenizer::tokenize(sMpRtpAddr, ";", true, true);
  if (vSrcMpRtpAddrInterfaces.empty())
  {
    LOG(WARNING) << "Invalid src/dest_mprtp_addr header: " << sMpRtpAddr;
    return mediaInterface;
  }
  else
  {
    // This is a string of form "<counter> <address> <port>"
    for (const std::string& sInterface : vSrcMpRtpAddrInterfaces)
    {
      std::vector<std::string> vInterfaceInfo = StringTokenizer::tokenize(sInterface, " ", true, true);
      if (vInterfaceInfo.size() != 3)
      {
        LOG(WARNING) << "Invalid interface in src_mprtp_addr header: " << sInterface;
        return MediaInterfaceDescriptor_t();
      }
      else
      {
        VLOG(2) << "src/dest_mprtp_addr Interface validated: " << sInterface;
        // now try to bind to port
        bool bSuccess = true;
        uint16_t uiRtpPort = convert<uint16_t>(vInterfaceInfo[2], bSuccess);
        if (!bSuccess)
        {
          LOG(WARNING) << "Invalid port in interface: " << sInterface;
          return MediaInterfaceDescriptor_t();
        }
        else
        {
          uint16_t uiRtcpPort = bRtcpMux ? uiRtpPort : uiRtpPort + 1;
          AddressDescriptor rtp(vInterfaceInfo[1], uiRtpPort);
          AddressDescriptor rtcp(vInterfaceInfo[1], uiRtcpPort);
          mediaInterface.push_back(std::tie(rtp, rtcp, ""));
        }
      }
    }
  }
  return mediaInterface;
}

} // mprtp
} // rtp_plus_plus
