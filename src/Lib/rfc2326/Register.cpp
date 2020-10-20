#include "CorePch.h"
#include <sstream>
#include <rtp++/rfc2326/Register.h>

namespace rtp_plus_plus
{
namespace rfc2326_ext
{

const std::string REUSE_CONNECTION = "reuse_connection";
const std::string PREFERRED_DELIVERY_PROTOCOL = "preferred_delivery_protocol";
const std::string PROXY_URL_SUFFIX = "proxy_url_suffix";

TransportParameter::TransportParameter(bool bReuseConnection, DeliveryProtocol eDeliveryProtocol, const std::string& sProxyUrlSuffix)
  :m_bReuseConnection(bReuseConnection),
    m_eDeliveryProtocol(eDeliveryProtocol),
    m_sProxyUrlSuffix(sProxyUrlSuffix)
{

}

std::string TransportParameter::toString() const
{
  std::ostringstream ostr;
  ostr << REUSE_CONNECTION << "=";
  ostr << (m_bReuseConnection ? "1" : "0") << ";";
  switch (m_eDeliveryProtocol)
  {
    case TRANSPORT_UDP:
    {
      ostr << PREFERRED_DELIVERY_PROTOCOL << "=udp;";
      break;
    }
    case TRANSPORT_INTERLEAVED:
    {
      ostr << PREFERRED_DELIVERY_PROTOCOL << "=interleaved;";
      break;
    }
    default:
    {
      break;
    }
  }
  if (!m_sProxyUrlSuffix.empty())
  {
    ostr << PROXY_URL_SUFFIX << "=" << m_sProxyUrlSuffix << ";";
  }
  return ostr.str();
}

} // rfc2326_ext
} // rtp_plus_plus

