#include "CorePch.h"
#include <rtp++/rfc2326/Transport.h>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/mprtp/MpRtp.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

Transport::Transport()
  :m_bValid(false),
    m_bUnicast(false),
    m_bInterleaved(false),
  #ifdef RTSP_1_0
    m_uiClientStartPort(0),
    m_uiServerStartPort(0),
  #endif
    m_uiInterleavedStart(0),
    m_bRtcpMux(false)
{
}

Transport::Transport(const std::string& sTransport)
  :m_sTransportHeader(sTransport),
    m_bValid(true),
    m_bUnicast(false),
    m_bInterleaved(false),
  #ifdef RTSP_1_0
    m_uiClientStartPort(0),
    m_uiServerStartPort(0),
  #endif
    m_uiInterleavedStart(0),
    m_bRtcpMux(false)
{
  VLOG(10) << "Parsing transport: " << sTransport;
  parseTransportString(sTransport);
}

Transport::~Transport()
{

}

std::string Transport::toString() const
{
  std::ostringstream oss;
#ifdef RTSP_1_0
  //oss << "RTP/AVP;";

  oss << getSpecifier();

  if (m_bUnicast)
    oss << ";unicast";
  else
    oss << ";multicast";

  if (!m_bInterleaved)
  {
    if (m_uiClientStartPort != 0)
    {
      uint16_t uiClientEndPort = m_bRtcpMux ? m_uiClientStartPort : m_uiClientStartPort + 1;
      oss << ";client_port=" << m_uiClientStartPort << "-" << uiClientEndPort;
    }
  }
  else
  {
    oss << ";interleaved=" << m_uiInterleavedStart << "-" << m_uiInterleavedStart + 1;
  }

  if (m_uiServerStartPort != 0)
  {
    uint16_t uiServerEndPort = m_bRtcpMux ? m_uiServerStartPort : m_uiServerStartPort + 1;
    oss << ";server_port=" << m_uiServerStartPort << "-" << uiServerEndPort;
  }

  if (m_bRtcpMux)
    oss << ";RTCP-mux";

#else
  // TODO: modify to use m_SrcAddresses & m_vDestAddresses
  oss << "RTP/AVP;unicast;dest_addr=\":" << m_uiClientStartPort << "\"/\":" << m_uiClientStartPort + 1 << "\"";
#endif

  if (!m_sSrcMpRtpAddr.empty())
  {
    oss << ";src_mprtp_addr=\"" << m_sSrcMpRtpAddr << "\"";
  }

  if (!m_sDestMpRtpAddr.empty())
  {
    oss << ";dest_mprtp_addr=\"" << m_sDestMpRtpAddr << "\"";
  }

  if (!m_mExtmaps.empty())
  {
    oss << ";extmap=\"";
    for (auto mapEntry : m_mExtmaps)
    {
      oss << mapEntry.first << " " << mapEntry.second << ";";
    }
    oss << "\"";
  }
  return oss.str();
}

void Transport::parseTransportString(const std::string& sTransport)
{
  m_sTransportHeader = sTransport;

#if 0
#if 0
  boost::char_separator<char> sep(";");
  boost::tokenizer<boost::char_separator<char>> tokens(sTransport, sep);
  BOOST_FOREACH(std::string sParam, tokens)
  {
    m_mParams.insert(sParam);
  }
#else
  std::vector<std::string> vTokens = StringTokenizer::tokenize(sTransport, ";", true, true);
  for (auto& sParam: vTokens)
  {
    m_mParams.insert(sParam);
  }
#endif
#else
  // the src_mprtp_addr and dest_mprtp_addr have complicated the parsing
  std::string sCopy = sTransport;
  size_t posQuote = sCopy.find("\"");
  std::map<std::string, std::string> mTempMap;
  int iToken = 0;
  while (posQuote != std::string::npos)
  {
    size_t posQuote2 = sCopy.find("\"", posQuote + 1);
    assert(posQuote2 != std::string::npos);
    std::string sKey = "<<" + ::toString(iToken++) + ">>";
    std::string sTemp = sCopy.substr(0, posQuote) + sKey + sCopy.substr(posQuote2 + 1);
    mTempMap[sKey] = sCopy.substr(posQuote, posQuote2 - posQuote + 1);
    sCopy = sTemp;
    posQuote = sCopy.find("\"");
  }
  
  // now split
  m_vParams = StringTokenizer::tokenize(sCopy, ";", true, true);

  for (std::string& sParam : m_vParams)
  {
    for (auto& pair : mTempMap)
    {
      if (sParam.find(pair.first) != std::string::npos)
      {
        // replace
        boost::algorithm::replace_all(sParam, pair.first, pair.second);
      }
    }
  }

#endif
  for (std::string& val : m_vParams)
  {
    VLOG(2) << "Transport parameter: " << val;
    if (val == "unicast")
    {
      m_bUnicast = true;
    }
    else if (val == "multicast")
    {
      m_bUnicast = false;
    }
    else if (val == "RTCP-mux")
    {
      m_bRtcpMux = true;
    }
    // We only accept RTP as a specifier for now
    if (boost::algorithm::contains(val, "RTP"))
    {
      if (!setTransportSpecifier(val))
      {
        m_bValid = false;
      }
    }
#ifdef RTSP_1_0
    else if (boost::algorithm::contains(val, "client_port="))
    {
      std::string sPorts = val.substr(12);
      size_t pos = sPorts.find("-");

      std::string sRtpPort = sPorts.substr(0, pos);
      bool bDummy;
      m_uiClientStartPort = convert<unsigned short>(sRtpPort, bDummy);
      m_vDestAddresses.push_back(std::make_pair("", m_uiClientStartPort));
      std::string sRtcpPort = sPorts.substr(pos + 1);
      m_vDestAddresses.push_back(std::make_pair("", convert<unsigned short>(sRtcpPort, bDummy)));
    }
    else if (boost::algorithm::contains(val, "server_port="))
    {
      std::string sPorts = val.substr(12);
      size_t pos = sPorts.find("-");

      bool bDummy;
      std::string sRtpPort = sPorts.substr(0, pos);
      m_uiServerStartPort = convert<uint16_t>(sRtpPort, bDummy);
      m_vDestAddresses.push_back(std::make_pair("", m_uiServerStartPort));
      std::string sRtcpPort = sPorts.substr(pos + 1);
      m_vDestAddresses.push_back(std::make_pair("", convert<unsigned short>(sRtcpPort, bDummy)));
    }
#endif
#ifdef RTSP_2_0
    else if (boost::algorithm::starts_with(val, "dest_addr="))
    {
      if (!parseAddresses(val.substr(10), m_vDestAddresses))
      {
        m_bValid = false;
      }
    }
    else if ( boost::algorithm::starts_with(val, "src_addr="))
    {
      if (!parseAddresses(val.substr(9), m_SrcAddresses))
      {
        m_bValid = false;
      }
    }
#endif
    else if (boost::algorithm::contains(val, "interleaved="))
    {
      m_bInterleaved = true;
      std::string sChannels = val.substr(12);
      size_t pos = sChannels.find("-");
      assert(pos != std::string::npos);
      bool bDummy;
      m_uiInterleavedStart = convert<uint32_t>(sChannels.substr(0, pos), bDummy);
  }
    else if (boost::algorithm::contains(val, "source="))
    {
      m_sSource = val.substr(7);
    }
    else if (boost::algorithm::contains(val, "destination="))
    {
      m_sDestination = val.substr(12);
    }
    else if (boost::algorithm::contains(val, "src_mprtp_addr=\""))
    {
      m_sSrcMpRtpAddr = val.substr(16);
      // rem last quote
      m_sSrcMpRtpAddr = m_sSrcMpRtpAddr.substr(0, m_sSrcMpRtpAddr.length() - 1);
    }
    else if (boost::algorithm::contains(val, "dest_mprtp_addr=\""))
    {
      m_sDestMpRtpAddr = val.substr(17);
      // rem last quote
      m_sDestMpRtpAddr = m_sDestMpRtpAddr.substr(0, m_sDestMpRtpAddr.length() - 1);
    }
    else if (boost::algorithm::contains(val, "extmap=\""))
    {
      std::string sExtmaps = val.substr(8);
      // rem last quote
      sExtmaps = sExtmaps.substr(0, sExtmaps.length() - 1);
      std::vector<std::string> vExtmaps = StringTokenizer::tokenize(sExtmaps, ";", true, true);
      // now split each of these according to the space
      for (std::string& sExtmap : vExtmaps)
      {
        std::vector<std::string> vValues = StringTokenizer::tokenize(sExtmap, " ", true, true);
        assert(vValues.size() == 2);
        bool bSuccess = false;
        uint32_t uiId = convert<uint32_t>(vValues[0], bSuccess);
        assert(bSuccess);
        std::string sValue = vValues[1];
        m_mExtmaps[uiId] = sValue;
      }
    }
    else
    {
      // TODO:
    }
  }


  // layers
  // dest_addr
  // src_addr
  // mode
  // interleaved

  // multicast
  //  ttl

  // RTP
  //  ssrc

  // TCP
  //  RTCP-mux
  //  - interleave
  //    setup
  //    connection
  m_bValid = true;
}

bool Transport::setTransportSpecifier(const std::string &sTransportSpecifier)
{
  std::vector<std::string> vTransport = StringTokenizer::tokenize(sTransportSpecifier, "/", true, true);
  if (vTransport.size() < 2)
  {
    return false;
  }
  m_sTransport = vTransport[0];
  m_sProfile = vTransport[1];
  if (vTransport.size() == 3)
  {
    m_sLowerTransport = vTransport[2];
  }
  return true;
}

bool Transport::parseAddresses(std::string& sTransport, std::vector<Address_t>& vAddresses)
{
  boost::algorithm::erase_all(sTransport, "\"");
  size_t pos = sTransport.find("/");
  if (pos == std::string::npos)
  {
    return extractAddress(sTransport, vAddresses);
  }
  else
  {
    return extractAddress(sTransport.substr(0, pos), vAddresses) &&
        extractAddress(sTransport.substr(pos + 1), vAddresses);
  }
}

bool Transport::extractAddress( const std::string &sTransport, std::vector<Address_t> &vAddresses )
{
  size_t posColon = sTransport.find(":");
  if (posColon == std::string::npos)
  {
    return false;
  }
  // parse single address
  bool bDummy;
  std::string sIp = sTransport.substr(0, posColon);
  std::string sPort = sTransport.substr(posColon + 1);
  vAddresses.push_back(std::make_pair(sIp, convert<unsigned short>(sPort, bDummy)));
  return true;
}

bool Transport::setSrcMpRtpAddr(const std::string& sSrcMpRtpAddr)
{
  bool bDummy = true;
  MediaInterfaceDescriptor_t interfaces = mprtp::getMpRtpInterfaceDescriptorFromRtspTransportString(sSrcMpRtpAddr, bDummy);
  if (interfaces.empty()) return false;
  else
  {
    m_sSrcMpRtpAddr = sSrcMpRtpAddr;
    return true;
  }
}

bool Transport::setDestMpRtpAddr(const std::string& sDestMpRtpAddr)
{
  bool bDummy = true;
  MediaInterfaceDescriptor_t interfaces = mprtp::getMpRtpInterfaceDescriptorFromRtspTransportString(sDestMpRtpAddr, bDummy);
  if (interfaces.empty()) return false;
  else
  {
    m_sDestMpRtpAddr = sDestMpRtpAddr;
    return true;
  }
}


} // rfc2326
} // rtp_plus_plus

