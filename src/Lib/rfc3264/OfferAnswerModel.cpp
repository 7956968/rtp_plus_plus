#include "CorePch.h"
#include <rtp++/rfc3264/OfferAnswerModel.h>
#include <cpputil/Conversion.h>
#include <rtp++/experimental/RtcpHeaderExtension.h>
#include <rtp++/mprtp/MpRtp.h>
#include <rtp++/RtpTime.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc4145/Rfc4145.h>
#include <rtp++/rfc4566/Rfc4566.h>
#include <rtp++/rfc5285/Rfc5285.h>
#include <rtp++/rfc5506/Rfc5506.h>
#include <rtp++/rfc5761/Rfc5761.h>
#include <rtp++/rfc6051/Rfc6051.h>
#include <rtp++/rfc6184/Rfc6184.h>

#define COMPONENT_LOG_LEVEL 2

#define MIN_DYNAMIC_PAYLOAD 96
#define DEFAULT_AUDIO_BW_KBPS 64
#define DEFAULT_VIDEO_BW_KBPS 500

namespace rtp_plus_plus
{
namespace rfc3264
{

// TOREMOVE: replace with FQDN at outer layer
// when defining this we use the 0.0.0.0 IP address for port allocation instead of the primary interface
// #define OVERRIDE_PRIMARY_INTERFACE_WITH_0_0_0_0

OfferAnswerModel::OfferAnswerModel(rfc3261::SipContext& sipContext,
                                   MediaSessionNetworkManager& mediaSessionNetworkManager,
                                   bool bEnableMpRtp, bool bRtcpMux, bool bRapidSync, bool bRtcpRs, bool bRtcpRtpHeaderExt)
  :m_sipContext(sipContext),
  m_mediaSessionNetworkManager(mediaSessionNetworkManager),
  m_sRtpProfile(rfc3550::RTP_AVP),
  m_sProtocol(rfc3550::UDP),
  m_bEnableMpRtp(bEnableMpRtp),
  m_bRtcpMux(bRtcpMux),
  m_bRapidSync(bRapidSync),
  m_bRtcpRs(bRtcpRs),
  m_bRtcpRtpHeaderExt(bRtcpRtpHeaderExt),
  m_uiNextDynamicPayload(MIN_DYNAMIC_PAYLOAD),
  m_uiMaxVideoBandwidthKbps(1000), // default
  m_uiMaxAudioBandwidthKbps(100), // default
  m_uiPreferredStartingPort(49170),
  m_bBindPorts(true)
{
  VLOG(COMPONENT_LOG_LEVEL) << "Profile: " << m_sRtpProfile
                            << " Protocol: " << m_sProtocol
                            << " MPRTP: " << bEnableMpRtp
                            << " RTCP-mux: " << bRtcpMux
                            << " rapid sync: " << bRapidSync
                            << " reduced-size: " << bRtcpRs;
  m_vSupportedRtpProfiles.insert(m_sRtpProfile);
}

OfferAnswerModel::OfferAnswerModel(rfc3261::SipContext& sipContext,
                                   MediaSessionNetworkManager& mediaSessionNetworkManager,
                                   const std::string& sRtpProfile, bool bEnableMpRtp, bool bRtcpMux, bool bRapidSync, bool bRtcpRs, bool bRtcpRtpHeaderExt)
  :m_sipContext(sipContext),
  m_mediaSessionNetworkManager(mediaSessionNetworkManager),
  m_bEnableMpRtp(bEnableMpRtp),
  m_sRtpProfile(sRtpProfile),
  m_sProtocol(rfc3550::UDP),
  m_bRtcpMux(bRtcpMux),
  m_bRapidSync(bRapidSync),
  m_bRtcpRs(bRtcpRs),
  m_bRtcpRtpHeaderExt(bRtcpRtpHeaderExt),
  m_uiNextDynamicPayload(MIN_DYNAMIC_PAYLOAD),
  m_uiMaxVideoBandwidthKbps(1000), // default
  m_uiMaxAudioBandwidthKbps(100), // default
  m_bBindPorts(true)
{
  VLOG(COMPONENT_LOG_LEVEL) << "Profile: " << m_sRtpProfile
                            << " Protocol: " << m_sProtocol
                            << " MPRTP: " << bEnableMpRtp
                            << " RTCP-mux: " << bRtcpMux
                            << " rapid sync: " << bRapidSync
                            << " reduced-size: " << bRtcpRs;
  m_vSupportedRtpProfiles.insert(sRtpProfile);
}


OfferAnswerModel::~OfferAnswerModel()
{
}

bool OfferAnswerModel::addFormat(const std::string& sMediaType,
                                 const std::string& sFormat,
                                 uint32_t uiBandwidthKbps,
                                 const std::vector<std::string>& vRtcpFb,
                                 const std::vector<std::string>& vRtcpXr)
{
  // TODO: add uiBandwidthKbps
  if (sMediaType == rfc4566::AUDIO)
  {
    m_vAudioPayloadFormats.push_back(PayloadFormat(sFormat, vRtcpFb, vRtcpXr));
    if (uiBandwidthKbps > m_uiMaxAudioBandwidthKbps)
      m_uiMaxAudioBandwidthKbps = uiBandwidthKbps;
    return true;
  }
  else if (sMediaType == rfc4566::VIDEO)
  {
    m_vVideoPayloadFormats.push_back(PayloadFormat(sFormat, vRtcpFb, vRtcpXr));
    if (uiBandwidthKbps > m_uiMaxVideoBandwidthKbps)
      m_uiMaxVideoBandwidthKbps = uiBandwidthKbps;
    return true;
  }
  else
    return false;
}

bool OfferAnswerModel::addFormat(std::string& sFormat,
                                 const std::string& sMediaType,
                                 boost::optional<rfc4566::RtpMapping> rtpMap,
                                 boost::optional<rfc4566::FormatDescription> fmtp,
                                 uint32_t uiBandwidthKbps,
                                 const std::vector<std::string>& vRtcpFb,
                                 const std::vector<std::string>& vRtcpXr)
{  
  assert(m_uiNextDynamicPayload >= MIN_DYNAMIC_PAYLOAD && m_uiNextDynamicPayload <= 128);
  if (sMediaType == rfc4566::AUDIO)
  {
    sFormat = ::toString(m_uiNextDynamicPayload++);
    m_vAudioPayloadFormats.push_back(PayloadFormat(sFormat, rtpMap, fmtp, vRtcpFb, vRtcpXr));
    if (uiBandwidthKbps > m_uiMaxAudioBandwidthKbps)
      m_uiMaxAudioBandwidthKbps = uiBandwidthKbps;
    return true;
  }
  else if (sMediaType == rfc4566::VIDEO)
  {
    sFormat = ::toString(m_uiNextDynamicPayload++);
    m_vVideoPayloadFormats.push_back(PayloadFormat(sFormat, rtpMap, fmtp, vRtcpFb, vRtcpXr));
    if (uiBandwidthKbps > m_uiMaxVideoBandwidthKbps)
      m_uiMaxVideoBandwidthKbps = uiBandwidthKbps;
    return true;
  }
  else
    return false;
}

void OfferAnswerModel::configurePorts(const std::string& sHost, rfc4566::MediaDescription& media, const MediaInterfaceDescriptor_t& mediaInterface)
{
  assert(!mediaInterface.empty());
  if (m_bEnableMpRtp)
  {
    if (m_bRtcpMux)
    {
      media.addAttribute(rfc5761::RTCP_MUX, "");

      for (size_t i = 0; i < mediaInterface.size(); ++i)
      {
        const AddressTuple_t& addresses = mediaInterface[i];
        const AddressDescriptor& rtpAddress = std::get<0>(addresses);

        std::string localInterface = rtpAddress.getAddress();
        uint16_t uiPort = rtpAddress.getPort();
        // RG: TODO: don't bind when using this overload as we're generating SDP for remote host?
#if 0
        if (m_bBindPorts)
          m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPort(localInterface, uiPort, false);
#endif
        // set primary interface port in m-line
        if (i == 0)
        {
#if 1
          // c-line
          rfc4566::ConnectionData connection;
#ifdef OVERRIDE_PRIMARY_INTERFACE_WITH_0_0_0_0
          connection.ConnectionAddress = m_sipContext.FQDN;
#else
          connection.ConnectionAddress = sHost;
#endif
          media.setConnection(connection);
#endif
          media.setPort(uiPort);
        }
        std::ostringstream mprtpInterfaceAttribute;
        mprtpInterfaceAttribute << "interface:" << (i + 1) << " " << localInterface << ":" << uiPort;
        std::string sMpRtpInterface = mprtpInterfaceAttribute.str();
        VLOG(2) << "Adding local MPRTP interface to SDP: " << sMpRtpInterface;
        media.addAttribute(mprtp::MPRTP, sMpRtpInterface);
      }
    }
    else
    {
      for (size_t i = 0; i < mediaInterface.size(); ++i)
      {
        const AddressTuple_t& addresses = mediaInterface[i];
        const AddressDescriptor& rtpAddress = std::get<0>(addresses);

        std::string localInterface = rtpAddress.getAddress();
        uint16_t uiPort = rtpAddress.getPort();

        // RG: TODO: don't bind when using this overload as we're generating SDP for remote host?
#if 0
        if (m_bBindPorts)
          m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPortsForRtpRtcp(localInterface, uiPort, false);
#endif
        // set primary interface port in m-line
        if (i == 0)
        {
#if 1
          // c-line
          rfc4566::ConnectionData connection;
#ifdef OVERRIDE_PRIMARY_INTERFACE_WITH_0_0_0_0
          connection.ConnectionAddress = m_sipContext.FQDN;
#else
          connection.ConnectionAddress = sHost;
#endif
          media.setConnection(connection);
#endif
          media.setPort(uiPort);
        }

        std::ostringstream mprtpInterfaceAttribute;
        mprtpInterfaceAttribute << "interface:" << (i + 1) << " " << localInterface << ":" << uiPort;
        std::string sMpRtpInterface = mprtpInterfaceAttribute.str();
        VLOG(2) << "Adding local MPRTP interface to SDP: " << sMpRtpInterface;
        media.addAttribute(mprtp::MPRTP, sMpRtpInterface);
      }
    }
    // add extmap: hard-coding to 1 for now
    const uint32_t MPRTP_EXT_MAP_ID = 1;
    std::string sExtmapValue = ::toString(MPRTP_EXT_MAP_ID) + " " + mprtp::EXTENSION_NAME_MPRTP;
    media.addAttribute(rfc5285::EXTMAP, sExtmapValue);
  }
  else
  {
    const AddressTuple_t& addresses = mediaInterface[0];
    const AddressDescriptor& rtpAddress = std::get<0>(addresses);
    std::string localInterface = rtpAddress.getAddress();
    uint16_t uiPort = rtpAddress.getPort();

    if (m_bRtcpMux)
    {
      media.addAttribute(rfc5761::RTCP_MUX, "");
      // RG: TODO: don't bind when using this overload as we're generating SDP for remote host?
#if 0
      if (m_bBindPorts)
        m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPort(localInterface, uiPort, false);
    }
    else
    {
      if (m_bBindPorts)
        m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPortsForRtpRtcp(localInterface, uiPort, false);
#endif
    }
    media.setPort(uiPort);
#if 1
    // c-line
    rfc4566::ConnectionData connection;
#ifdef OVERRIDE_PRIMARY_INTERFACE_WITH_0_0_0_0
    connection.ConnectionAddress = m_sipContext.FQDN;
#else
    connection.ConnectionAddress = sHost;
#endif
    media.setConnection(connection);
#endif
  }
}

void OfferAnswerModel::configurePorts(rfc4566::MediaDescription& media, uint16_t uiStartingPort)
{
  uint16_t uiPort = uiStartingPort;
  if (m_bEnableMpRtp)
  {
    if (m_bRtcpMux)
    {
      media.addAttribute(rfc5761::RTCP_MUX, "");

      const std::vector<std::string>& vInterfaces = m_mediaSessionNetworkManager.getLocalInterfaces();
      for (size_t i = 0; i < vInterfaces.size(); ++i)
      {
        const std::string& localInterface = vInterfaces[i];
        if (m_bBindPorts)
          m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPort(localInterface, uiPort, false);
        // set primary interface port in m-line
        if (i == 0)
        {
#if 1
          // c-line
          rfc4566::ConnectionData connection;
#ifdef OVERRIDE_PRIMARY_INTERFACE_WITH_0_0_0_0
          connection.ConnectionAddress = m_sipContext.FQDN;
#else
          connection.ConnectionAddress = m_mediaSessionNetworkManager.getPrimaryInterface();
#endif
          media.setConnection(connection);
#endif
          media.setPort(uiPort);
        }
        std::ostringstream mprtpInterfaceAttribute;
        mprtpInterfaceAttribute << "interface:" << (i + 1) << " " << localInterface << ":" << uiPort;
        std::string sMpRtpInterface = mprtpInterfaceAttribute.str();
        VLOG(2) << "Adding local MPRTP interface to SDP: " << sMpRtpInterface;
        media.addAttribute(mprtp::MPRTP, sMpRtpInterface);
      }
    }
    else
    {
      const std::vector<std::string>& vInterfaces = m_mediaSessionNetworkManager.getLocalInterfaces();
      for (size_t i = 0; i < vInterfaces.size(); ++i)
      {
        const std::string& localInterface = vInterfaces[i];
        if (m_bBindPorts)
          m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPortsForRtpRtcp(localInterface, uiPort, false);
        // set primary interface port in m-line
        if (i == 0)
        {
#if 1
          // c-line
          rfc4566::ConnectionData connection;
#ifdef OVERRIDE_PRIMARY_INTERFACE_WITH_0_0_0_0
          connection.ConnectionAddress = m_sipContext.FQDN;
#else
          connection.ConnectionAddress = m_mediaSessionNetworkManager.getPrimaryInterface();
#endif
          media.setConnection(connection);
#endif
          media.setPort(uiPort);
        }

        std::ostringstream mprtpInterfaceAttribute;
        mprtpInterfaceAttribute << "interface:" << (i + 1) << " " << localInterface << ":" << uiPort;
        std::string sMpRtpInterface = mprtpInterfaceAttribute.str();
        VLOG(2) << "Adding local MPRTP interface to SDP: " << sMpRtpInterface;
        media.addAttribute(mprtp::MPRTP, sMpRtpInterface);
      }
    }
    // add extmap: hard-coding to 1 for now
    const uint32_t MPRTP_EXT_MAP_ID = 1;
    std::string sExtmapValue = ::toString(MPRTP_EXT_MAP_ID) + " " + mprtp::EXTENSION_NAME_MPRTP;
    media.addAttribute(rfc5285::EXTMAP, sExtmapValue);
  }
  else
  {
#ifdef OVERRIDE_PRIMARY_INTERFACE_WITH_0_0_0_0
    const std::string sAddress("0.0.0.0");
#else
    const std::string sAddress(m_mediaSessionNetworkManager.getPrimaryInterface());
#endif
    if (m_bRtcpMux)
    {
      media.addAttribute(rfc5761::RTCP_MUX, "");
      if (m_bBindPorts)
        m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPort(sAddress, uiPort, false);
    }
    else
    {
      if (m_bBindPorts)
        m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPortsForRtpRtcp(sAddress, uiPort, false);
    }
    media.setPort(uiPort);
#if 1
    // c-line
    rfc4566::ConnectionData connection;
#ifdef OVERRIDE_PRIMARY_INTERFACE_WITH_0_0_0_0
    connection.ConnectionAddress = m_sipContext.FQDN;
#else
    connection.ConnectionAddress = m_mediaSessionNetworkManager.getPrimaryInterface();
#endif
    media.setConnection(connection);
#endif
  }
}

rfc4566::SessionDescription OfferAnswerModel::createSessionDescription(const std::string &sConnection)
{
  rfc4566::SessionDescription offer;
  // o-line
  offer.setOrigin(rfc4566::origin(m_sipContext.User, m_sipContext.FQDN));
  // s-line
  offer.setSessionName("rtp++ SIP Session");
  // t-line: unbounded should be fine for now
  offer.addTiming("0 0");
  // media-level c-line causes offer/answer with linphone to fail for some reason
#if 1
  // c-line
  rfc4566::ConnectionData connection;
#ifdef OVERRIDE_PRIMARY_INTERFACE_WITH_0_0_0_0
  connection.ConnectionAddress = m_sipContext.FQDN;
#else
  connection.ConnectionAddress = sConnection;
#endif

#if 0
  connection.ConnectionAddress = m_mediaSessionNetworkManager.getPrimaryInterface();
#else

#endif
  offer.setConnection(connection);
#endif
  return offer;
}

boost::optional<rfc4566::SessionDescription> OfferAnswerModel::generateOffer(const std::string& sHost, const InterfaceDescriptions_t &interfaces)
{
  rfc4566::SessionDescription offer = createSessionDescription(sHost);

  uint32_t mediaInterfaceIndex = 0;
  // add audio
  if (!m_vAudioPayloadFormats.empty())
  {
    rfc4566::MediaDescription media;
    media.setMediaType(rfc4566::AUDIO);
    media.setProtocol(m_sRtpProfile, m_sProtocol);
    // TODO: if protocol is TCP or SCTP, then set connection setup parameters
    VLOG(2) << "Protocol: " << m_sProtocol;
    if (m_sProtocol == "SCTP" || m_sProtocol == "TCP")
    {
      // offerer will always call: set to active
      media.addAttribute(rfc4145::SETUP, rfc4145::ACTIVE);
      media.addAttribute(rfc4145::CONNECTION, rfc4145::NEW);
    }
    // get ports
    configurePorts(sHost, media, interfaces[mediaInterfaceIndex++]);
    for (auto& format : m_vAudioPayloadFormats)
    {
      if (!format.Dynamic)
      {
        // add static payload type info
        media.addFormat(format.Format);
        if (!format.RtcpFb.empty())
        {
          media.setFeedback(format.Format, format.RtcpFb);
#if 0
          media.addAttribute(rfc4585::RTCP_FB, ostr.str());
#endif
        }
      }
      else
      {
        // add dynamic payload type info
        // TODO: how to handle cases that have rtpmap but no fmtp
        media.addFormat(format.Format, *format.rtpMap, *format.fmtp);
        // assert(uiDynamicPayload >= MIN_DYNAMIC_PAYLOAD && uiDynamicPayload <= 128);
        if (!format.RtcpFb.empty())
        {
          media.setFeedback(format.Format, format.RtcpFb);
#if 0
          media.addAttribute(rfc4585::RTCP_FB, ostr.str());
#endif
        }
      }
    }
    uint32_t uiAudioBandwidthKbps = (m_uiMaxAudioBandwidthKbps > 0) ? m_uiMaxAudioBandwidthKbps : DEFAULT_AUDIO_BW_KBPS;
    media.setSessionBandwidth(uiAudioBandwidthKbps);

    // rapid sync
    // add extmap: hard-coding to 2 for now
    if (m_bRapidSync)
    {
      const uint32_t RAPID_SYNC_RTP_EXT_MAP_ID = 2;
      std::string sExtmapValue = ::toString(RAPID_SYNC_RTP_EXT_MAP_ID) + " " + rfc6051::EXTENSION_NAME_RTP_NTP_64;
      media.addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }

    if (m_bRtcpRs)
    {
      media.addAttribute(rfc5506::RTCP_RSIZE, "");
    }

    if (m_bRtcpRtpHeaderExt)
    {
      media.addAttribute(experimental::RTCP_RTP_HEADER_EXT, "");
      const uint32_t RTCP_RTP_EXT_MAP_ID = 3;
      std::string sExtmapValue = ::toString(RTCP_RTP_EXT_MAP_ID) + " " + experimental::EXTENSION_NAME_RTCP_HEADER_EXT;
      media.addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }

    offer.addMediaDescription(media);
  }

  // add video
  if (!m_vVideoPayloadFormats.empty())
  {
    rfc4566::MediaDescription media;
    media.setMediaType(rfc4566::VIDEO);
    media.setProtocol(m_sRtpProfile, m_sProtocol);
    VLOG(2) << "Protocol: " << m_sProtocol;
    // TODO: if protocol is TCP or SCTP, then set connection setup parameters
    if (m_sProtocol == "SCTP" || m_sProtocol == "TCP")
    {
      // offerer will always call: set to active
      media.addAttribute(rfc4145::SETUP, rfc4145::ACTIVE);
      media.addAttribute(rfc4145::CONNECTION, rfc4145::NEW);
    }
    // get ports
    configurePorts(sHost, media, interfaces[mediaInterfaceIndex++]);
    for (auto& format : m_vVideoPayloadFormats)
    {
      if (!format.Dynamic)
      {
        // add static payload type info
        media.addFormat(format.Format);
        if (!format.RtcpFb.empty())
        {
          media.setFeedback(format.Format, format.RtcpFb);
#if 0
          media.addAttribute(rfc4585::RTCP_FB, ostr.str());
#endif
        }
      }
      else
      {
        // add dynamic payload type info
        // TODO: how to handle cases that have rtpmap but no fmtp
        media.addFormat(format.Format, *format.rtpMap, *format.fmtp);
        if (!format.RtcpFb.empty())
        {
          media.setFeedback(format.Format, format.RtcpFb);
#if 0
          media.addAttribute(rfc4585::RTCP_FB, ostr.str());
#endif
        }
      }
    }
    uint32_t uiVideoBandwidthKbps = (m_uiMaxVideoBandwidthKbps > 0) ? m_uiMaxVideoBandwidthKbps : DEFAULT_VIDEO_BW_KBPS;
    media.setSessionBandwidth(uiVideoBandwidthKbps);

    // rapid sync
    // add extmap: hard-coding to 2 for now
    if (m_bRapidSync)
    {
      const uint32_t RAPID_SYNC_RTP_EXT_MAP_ID = 2;
      std::string sExtmapValue = ::toString(RAPID_SYNC_RTP_EXT_MAP_ID) + " " + rfc6051::EXTENSION_NAME_RTP_NTP_64;
      media.addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }

    if (m_bRtcpRs)
    {
      media.addAttribute(rfc5506::RTCP_RSIZE, "");
    }

    if (m_bRtcpRtpHeaderExt)
    {
      media.addAttribute(experimental::RTCP_RTP_HEADER_EXT, "");
      const uint32_t RTCP_RTP_EXT_MAP_ID = 3;
      std::string sExtmapValue = ::toString(RTCP_RTP_EXT_MAP_ID) + " " + experimental::EXTENSION_NAME_RTCP_HEADER_EXT;
      media.addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }

    offer.addMediaDescription(media);
  }

  return boost::optional<rfc4566::SessionDescription>(offer);
}

boost::optional<rfc4566::SessionDescription> OfferAnswerModel::generateOffer()
{  
  rfc4566::SessionDescription offer = createSessionDescription(m_mediaSessionNetworkManager.getPrimaryInterface());

  // add audio
  if (!m_vAudioPayloadFormats.empty())
  {
    rfc4566::MediaDescription media;
    media.setMediaType(rfc4566::AUDIO);
    media.setProtocol(m_sRtpProfile, m_sProtocol);
    // TODO: if protocol is TCP or SCTP, then set connection setup parameters
    VLOG(2) << "Protocol: " << m_sProtocol;
    if (m_sProtocol == "SCTP" || m_sProtocol == "TCP")
    {
      // offerer will always call: set to active
      media.addAttribute(rfc4145::SETUP, rfc4145::ACTIVE);
      media.addAttribute(rfc4145::CONNECTION, rfc4145::NEW);
    }
    // get ports
    configurePorts(media, m_uiPreferredStartingPort);
    for (auto& format : m_vAudioPayloadFormats)
    {
      if (!format.Dynamic)
      {
        // add static payload type info
        media.addFormat(format.Format);
        if (!format.RtcpFb.empty())
        {
          media.setFeedback(format.Format, format.RtcpFb);
#if 0
          media.addAttribute(rfc4585::RTCP_FB, ostr.str());
#endif
        }
      }
      else
      {
        // add dynamic payload type info
        // TODO: how to handle cases that have rtpmap but no fmtp
        media.addFormat(format.Format, *format.rtpMap, *format.fmtp);
        // assert(uiDynamicPayload >= MIN_DYNAMIC_PAYLOAD && uiDynamicPayload <= 128);
        if (!format.RtcpFb.empty())
        {
          media.setFeedback(format.Format, format.RtcpFb);
#if 0
          media.addAttribute(rfc4585::RTCP_FB, ostr.str());
#endif
        }
      }
    }
    uint32_t uiAudioBandwidthKbps = (m_uiMaxAudioBandwidthKbps > 0) ? m_uiMaxAudioBandwidthKbps : DEFAULT_AUDIO_BW_KBPS;
    media.setSessionBandwidth(uiAudioBandwidthKbps);

    // rapid sync
    // add extmap: hard-coding to 2 for now    
    if (m_bRapidSync)
    {
      const uint32_t RAPID_SYNC_RTP_EXT_MAP_ID = 2;
      std::string sExtmapValue = ::toString(RAPID_SYNC_RTP_EXT_MAP_ID) + " " + rfc6051::EXTENSION_NAME_RTP_NTP_64;
      media.addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }

    if (m_bRtcpRs)
    {
      media.addAttribute(rfc5506::RTCP_RSIZE, "");
    }

    if (m_bRtcpRtpHeaderExt)
    {
      media.addAttribute(experimental::RTCP_RTP_HEADER_EXT, "");
      const uint32_t RTCP_RTP_EXT_MAP_ID = 3;
      std::string sExtmapValue = ::toString(RTCP_RTP_EXT_MAP_ID) + " " + experimental::EXTENSION_NAME_RTCP_HEADER_EXT;
      media.addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }

    offer.addMediaDescription(media);
  }

  // add video
  if (!m_vVideoPayloadFormats.empty())
  {
    rfc4566::MediaDescription media;
    media.setMediaType(rfc4566::VIDEO);
    media.setProtocol(m_sRtpProfile, m_sProtocol);
    VLOG(2) << "Protocol: " << m_sProtocol;
    // TODO: if protocol is TCP or SCTP, then set connection setup parameters
    if (m_sProtocol == "SCTP" || m_sProtocol == "TCP")
    {
      // offerer will always call: set to active
      media.addAttribute(rfc4145::SETUP, rfc4145::ACTIVE);
      media.addAttribute(rfc4145::CONNECTION, rfc4145::NEW);
    }
    // get ports
    configurePorts(media, m_uiPreferredStartingPort);
    for (auto& format : m_vVideoPayloadFormats)
    {
      if (!format.Dynamic)
      {
        // add static payload type info
        media.addFormat(format.Format);
        if (!format.RtcpFb.empty())
        {
          media.setFeedback(format.Format, format.RtcpFb);
#if 0
          media.addAttribute(rfc4585::RTCP_FB, ostr.str());
#endif
        }
      }
      else
      {
        // add dynamic payload type info
        // TODO: how to handle cases that have rtpmap but no fmtp
        media.addFormat(format.Format, *format.rtpMap, *format.fmtp);
        if (!format.RtcpFb.empty())
        {
          media.setFeedback(format.Format, format.RtcpFb);
#if 0
          media.addAttribute(rfc4585::RTCP_FB, ostr.str());
#endif
        }
      }
    }
    uint32_t uiVideoBandwidthKbps = (m_uiMaxVideoBandwidthKbps > 0) ? m_uiMaxVideoBandwidthKbps : DEFAULT_VIDEO_BW_KBPS;
    media.setSessionBandwidth(uiVideoBandwidthKbps);

    // rapid sync
    // add extmap: hard-coding to 2 for now
    if (m_bRapidSync)
    {
      const uint32_t RAPID_SYNC_RTP_EXT_MAP_ID = 2;
      std::string sExtmapValue = ::toString(RAPID_SYNC_RTP_EXT_MAP_ID) + " " + rfc6051::EXTENSION_NAME_RTP_NTP_64;
      media.addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }

    if (m_bRtcpRs)
    {
      media.addAttribute(rfc5506::RTCP_RSIZE, "");
    }

    if (m_bRtcpRtpHeaderExt)
    {
      media.addAttribute(experimental::RTCP_RTP_HEADER_EXT, "");
      const uint32_t RTCP_RTP_EXT_MAP_ID = 3;
      std::string sExtmapValue = ::toString(RTCP_RTP_EXT_MAP_ID) + " " + experimental::EXTENSION_NAME_RTCP_HEADER_EXT;
      media.addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }

    offer.addMediaDescription(media);
  }

  return boost::optional<rfc4566::SessionDescription>(offer);
}

boost::optional<rfc4566::SessionDescription> OfferAnswerModel::generateAnswer(const rfc4566::SessionDescription& offer)
{
  rfc4566::SessionDescription answer;
  // o-line
  answer.setOrigin(rfc4566::origin(m_sipContext.User, m_sipContext.FQDN));
  // s-line
  answer.setSessionName(offer.getSessionName());
  // t-line
  answer.setTiming(offer.getTiming());
  // media-level c-line causes offer/answer with linphone to fail for some reason
#if 0
  // c-line
  rfc4566::ConnectionData connection;
  connection.ConnectionAddress = m_sUnicastAddress;
  answer.setConnection(connection);
#endif
  // now select per media line
  for (size_t i = 0; i < offer.getMediaDescriptionCount(); ++i)
  {
    answer.addMediaDescription(getMediaPreference(offer.getMediaDescription(i)));
  }

  VLOG(2) << "Offer: " << offer.toString();
  VLOG(2) << "Answer: " << answer.toString();
  return boost::optional<rfc4566::SessionDescription>(answer);
}

rfc4566::MediaDescription OfferAnswerModel::getMediaPreference(const rfc4566::MediaDescription& media)
{
  rfc4566::MediaDescription preference;
  rfc4566::ConnectionData connection;
  // TOOD: should this use m_sipContext.FQDN instead?
#ifdef OVERRIDE_PRIMARY_INTERFACE_WITH_0_0_0_0
  connection.ConnectionAddress = m_sipContext.FQDN;
#else
  connection.ConnectionAddress = m_mediaSessionNetworkManager.getPrimaryInterface();
#endif

  preference.setConnection(connection);
  preference.setMediaTitle(media.getMediaTitle());
  preference.setMediaType(media.getMediaType());
  preference.setMid(media.getMid());
  VLOG(2) << "Offered protocol: " << media.getProtocol();
  VLOG(6) << "TODO: do we need to check first whether we support the profile at all?";
  preference.setProtocol(media.getProtocol());
  if (boost::algorithm::starts_with(media.getProtocol(), "SCTP") ||
      boost::algorithm::starts_with(media.getProtocol(), "TCP"))
  {
    // offerer will always call: set to passive
    preference.addAttribute(rfc4145::SETUP, rfc4145::PASSIVE);
    preference.addAttribute(rfc4145::CONNECTION, rfc4145::NEW);
  }

  uint32_t uiMaxSessionBandwidthKbps = media.getMediaType() == rfc4566::AUDIO ? m_uiMaxAudioBandwidthKbps : m_uiMaxVideoBandwidthKbps;
  preference.setSessionBandwidth(uiMaxSessionBandwidthKbps);

  const std::vector<std::string>& vFormats = media.getFormats();

  VLOG(2) << "Gathering acceptable media formats";
  uint32_t uiAcceptableFormats = 0;
  for (auto& sFormat : vFormats)
  {
    // check RTP MAP
    boost::optional<rfc4566::RtpMapping> rtpMap = media.getRtpMap(sFormat);
    boost::optional<rfc4566::FormatDescription> fmtp = media.getFmtp(sFormat);
    const std::map<std::string, rfc4566::FeedbackDescription>& fbMap = media.getFeedbackMap();

    // HACK for now: we only deal with dynamic payload types
    bool bDummy;
    uint32_t uiFormat = convert<uint32_t>(sFormat, bDummy);
    if (isFormatAcceptable(media.getMediaType(), sFormat, rtpMap, fmtp))
    {
      ++uiAcceptableFormats;
      if (uiFormat >= MIN_DYNAMIC_PAYLOAD)
        preference.addFormat(sFormat, *rtpMap, *fmtp);
      else
        preference.addFormat(sFormat);

      std::vector<std::string> vCommonFb = getCommonFeedbackTypes(sFormat, media);
      if (!vCommonFb.empty())
      {
#if 0
        for (auto& sFb : vCommonFb)
        {
          preference.setFeedback(sFormat, sFb);
        }
#else
        preference.setFeedback(sFormat, vCommonFb);
#endif
      }
      else
      {
        VLOG(2) << "No common feedback types";
      }
    }
  }
  VLOG(2) << "Gathering acceptable media formats- DONE";

  bool bRtcpMux = media.hasAttribute(rfc5761::RTCP_MUX);
  bool bMpRtp = media.hasAttribute(mprtp::MPRTP);
  bool bReducedSize = media.hasAttribute(rfc5506::RTCP_RSIZE);
  bool bRtcpRtpHeaderExt = media.hasAttribute(experimental::RTCP_RTP_HEADER_EXT);

  if (uiAcceptableFormats > 0)
  {
    VLOG(2) << "Using preferred start port: " << m_uiPreferredStartingPort;
    uint16_t uiLocalRtpPort = m_uiPreferredStartingPort;
    if (bMpRtp)
    {
      // if the offer is mprtp-capable, add all local interfaces to the SDP answer
      const std::vector<std::string>& vInterfaces = m_mediaSessionNetworkManager.getLocalInterfaces();
      for (size_t i = 0; i < vInterfaces.size(); ++i)
      {
        if (bRtcpMux)
        {
          if (m_bBindPorts)
          {
            boost::system::error_code ec = m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPort(vInterfaces[i], uiLocalRtpPort, false);
            assert(!ec);
          }
          // only add mux once
          if (i == 0)
            preference.addAttribute(rfc5761::RTCP_MUX, "");
        }
        else
        {
          if (m_bBindPorts)
          {
            boost::system::error_code ec = m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPortsForRtpRtcp(vInterfaces[i], uiLocalRtpPort, false);
            assert(!ec);
          }
        }
        // set port for primary interface
        if (i == 0)
          preference.setPort(uiLocalRtpPort);
        std::ostringstream mprtpInterfaceAttribute;
        mprtpInterfaceAttribute << "interface:" << (i + 1) << " " << vInterfaces[i] << ":" << uiLocalRtpPort;
        std::string sMpRtpInterface = mprtpInterfaceAttribute.str();
        VLOG(2) << "Adding local MPRTP interface to SDP: " << sMpRtpInterface;
        preference.addAttribute(mprtp::MPRTP, sMpRtpInterface);
      }

      if (bReducedSize && m_bRtcpRs)
      {
        preference.addAttribute(rfc5506::RTCP_RSIZE, "");
      }

      if (bRtcpRtpHeaderExt)
      {
        preference.addAttribute(experimental::RTCP_RTP_HEADER_EXT, "");
        const uint32_t RTCP_RTP_EXT_MAP_ID = 3;
        std::string sExtmapValue = ::toString(RTCP_RTP_EXT_MAP_ID) + " " + experimental::EXTENSION_NAME_RTCP_HEADER_EXT;
        preference.addAttribute(rfc5285::EXTMAP, sExtmapValue);
      }

      VLOG(6) << "TODO: get mprtp map id from offer and use same id. Hard-coding to 1 for now";
      // add extmap
      const uint32_t MPRTP_EXT_MAP_ID = 1;
      std::string sExtmapValue = ::toString(MPRTP_EXT_MAP_ID) + " " + mprtp::EXTENSION_NAME_MPRTP;
      preference.addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }
    else
    {
#ifdef OVERRIDE_PRIMARY_INTERFACE_WITH_0_0_0_0
      const std::string sAddress("0.0.0.0");
#else
      const std::string sAddress(m_mediaSessionNetworkManager.getPrimaryInterface());
#endif
      if (bRtcpMux)
      {
        if (m_bBindPorts)
        {
          boost::system::error_code ec = m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPort(sAddress, uiLocalRtpPort, false);
          assert(!ec);
        }
        preference.addAttribute(rfc5761::RTCP_MUX, "");
      }
      else
      {
        if (m_bBindPorts)
        {
          boost::system::error_code ec = m_mediaSessionNetworkManager.getPortAllocationManager().allocateUdpPortsForRtpRtcp(sAddress, uiLocalRtpPort, false);
          assert(!ec);
        }
      }
      VLOG(2) << "Setting local interface to SDP: " << sAddress << ":" << uiLocalRtpPort;
      preference.setPort(uiLocalRtpPort);
    }

    if (bReducedSize && m_bRtcpRs)
    {
      preference.addAttribute(rfc5506::RTCP_RSIZE, "");
    }

    if (bRtcpRtpHeaderExt)
    {
      preference.addAttribute(experimental::RTCP_RTP_HEADER_EXT, "");
      const uint32_t RTCP_RTP_EXT_MAP_ID = 3;
      std::string sExtmapValue = ::toString(RTCP_RTP_EXT_MAP_ID) + " " + experimental::EXTENSION_NAME_RTCP_HEADER_EXT;
      preference.addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }

    // check for rapid sync
    uint32_t uiExtMapId;
    if (media.lookupExtmap(rfc6051::EXTENSION_NAME_RTP_NTP_64, uiExtMapId))
    {
      VLOG(5) << "Found 64-bit rapid sync extension header handler.";
      const uint32_t RAPID_SYNC_RTP_EXT_MAP_ID = 2;
      std::string sExtmapValue = ::toString(RAPID_SYNC_RTP_EXT_MAP_ID) + " " + rfc6051::EXTENSION_NAME_RTP_NTP_64;
      preference.addAttribute(rfc5285::EXTMAP, sExtmapValue);
    }
  }
  else
  {
    VLOG(2) << "Rejecting " << media.getMediaType() << " media line";
    // reject media line
    // set port to 0
    preference.setPort(0);
    // the m line must still have a payload format so use the first one
    preference.addFormat(vFormats.at(0));
  }
  // preserve order
  preference.setIndexInSdp(media.getIndexInSdp());
  return preference;
}

bool OfferAnswerModel::isFormatAcceptable(const std::string& sMediaType, const std::string& sFormat, boost::optional<rfc4566::RtpMapping> rtpMap, boost::optional<rfc4566::FormatDescription> fmtp) const
{
  if (!rtpMap)
  {
    LOG(WARNING) << "NULL rtpMap in format " << sFormat;
  }
  if (!fmtp)
  {
    LOG(WARNING) << "NULL fmtp in format " << sFormat;
  }
  //assert(rtpMap);
  // assert(fmtp);

  if (sMediaType == rfc4566::AUDIO)
  {
    // check in static payload formats
    bool bDummy;
    uint32_t uiFormat = convert<uint32_t>(sFormat, bDummy);
    if (uiFormat < MIN_DYNAMIC_PAYLOAD)
    {
      auto it = std::find_if(m_vAudioPayloadFormats.begin(), m_vAudioPayloadFormats.end(), [sFormat](const PayloadFormat& format)
      {
        return format.Format == sFormat;
      });
      if (it != m_vAudioPayloadFormats.end())
      {
        VLOG(2) << "Supported static audio payload type: " << sFormat;
        return true;
      }
      else
      {
        VLOG(2) << "Unsupported static audio payload type: " << sFormat;
        return false;
      }
    }
    // check in dynamic payload formats
    else
    {
      // for now just compare codec name
      VLOG(6) << "TODO: should we also compare clock rates here?";
      auto it = std::find_if(m_vAudioPayloadFormats.begin(), m_vAudioPayloadFormats.end(), [rtpMap](const PayloadFormat& format)
      {
        // NOTE: there are also static payload types in the SDP
        if (format.rtpMap)
          return format.rtpMap->EncodingName == rtpMap->EncodingName;
        else
          return false;
      });
      if (it != m_vAudioPayloadFormats.end())
      {
        VLOG(2) << "Supported dynamic audio payload type: " << rtpMap->EncodingName;
        return true;
      }
      else
      {
        VLOG(2) << "Unsupported dynamic audio payload type: " << rtpMap->EncodingName;
        return false;
      }
    }
  }
  else if (sMediaType == rfc4566::VIDEO)
  {
    // check in static payload formats
    bool bDummy;
    uint32_t uiFormat = convert<uint32_t>(sFormat, bDummy);
    if (uiFormat < MIN_DYNAMIC_PAYLOAD)
    {
      auto it = std::find_if(m_vVideoPayloadFormats.begin(), m_vVideoPayloadFormats.end(), [sFormat](const PayloadFormat& format)
      {
        return format.Format == sFormat;
      });
      if (it != m_vVideoPayloadFormats.end())
      {
        VLOG(2) << "Supported static video payload type: " << sFormat;
        return true;
      }
      else
      {
        VLOG(2) << "Unsupported static video payload type: " << sFormat;
        return false;
      }
    }
    // check in dynamic payload formats
    else
    {
      // for now just compare codec name
      VLOG(6) << "TODO: should we also compare clock rates here?";
      auto it = std::find_if(m_vVideoPayloadFormats.begin(), m_vVideoPayloadFormats.end(), [rtpMap](const PayloadFormat& format)
      {
        // NOTE: there are also static payload types in the SDP
        if (format.rtpMap)
          return format.rtpMap->EncodingName == rtpMap->EncodingName;
        else
          return false;
      });
      if (it != m_vVideoPayloadFormats.end())
      {
        VLOG(2) << "Supported dynamic video payload type: " << rtpMap->EncodingName;
        return true;
      }
      else
      {
        // FIXME: HACK RG: not sure where the right place is for this
        if (rtpMap->EncodingName == "rtx")
        {
          LOG(WARNING) << "FIXME: RTX in offer";
          return true;
        }
        VLOG(2) << "Unsupported dynamic video payload type: " << rtpMap->EncodingName;
        return false;
      }
    }
  }
  else
  {
    LOG(WARNING) << "Unsupported media type: " << sMediaType;
    return false;
  }
}

std::vector<std::string> OfferAnswerModel::getCommonFeedbackTypes(const std::string& sFormat, const rfc4566::MediaDescription& media) const
{
  std::vector<std::string> commonFeedbackTypes;

  // look through local feedback map to see which feedback messages are understood
  assert (media.getMediaType() == rfc4566::AUDIO || media.getMediaType() == rfc4566::VIDEO);

  const std::vector<PayloadFormat>& vSupportedPayloadFormats = (media.getMediaType() == rfc4566::AUDIO) ? m_vAudioPayloadFormats : m_vVideoPayloadFormats;
  boost::optional<rfc4566::RtpMapping> rtpMap = media.getRtpMap(sFormat);
  boost::optional<rfc4566::FormatDescription> fmtp = media.getFmtp(sFormat);
  const std::map<std::string, rfc4566::FeedbackDescription>& fbMap = media.getFeedbackMap();
  bool bDummy;
  uint32_t uiFormat = convert<uint32_t>(sFormat, bDummy);
  // check if payload type is dynamic or static
  if (uiFormat < MIN_DYNAMIC_PAYLOAD)
  {
    // static pt
    assert(false); // not implementing for now
  }
  else
  {
    // see if we can find matching codec
    auto it = std::find_if(vSupportedPayloadFormats.begin(), vSupportedPayloadFormats.end(), [rtpMap](const PayloadFormat& format)
    {
      // NOTE: there are also static payload types in the SDP
      if (format.rtpMap)
        return format.rtpMap->EncodingName == rtpMap->EncodingName;
      else
        return false;
    });
    if (it != vSupportedPayloadFormats.end())
    {
      VLOG(2) << "Checking format: " << sFormat;
      auto it2 = fbMap.find(sFormat);
      if (it2 != fbMap.end())
      {
        // check if we have the same feedback messages
        const rfc4566::FeedbackDescription fb = it2->second;
        for (auto sFb : fb.FeedbackTypes)
        {
          VLOG(2) << "Checking format in media line: " << sFormat << " " << sFb;
          for (auto sFb2 : it->RtcpFb)
          {
            VLOG(2) << "Checking against feedback types: format: " << sFormat << " media feedback:" << sFb << " Supported format feedback: " << sFb2;
            if (sFb == sFb2)
              commonFeedbackTypes.push_back(sFb);
          }
        }
      }
      VLOG(2) << "Supported dynamic payload type: " << rtpMap->EncodingName;
    }
    else
    {
      VLOG(2) << "Unsupported dynamic payload type: " << rtpMap->EncodingName;
    }
  }
  return commonFeedbackTypes;
}

} // media
} // rtp_plus_plus
