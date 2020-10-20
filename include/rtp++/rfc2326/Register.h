#pragma once
#include <string>

namespace rtp_plus_plus
{
namespace rfc2326_ext
{

enum DeliveryProtocol
{
  TRANSPORT_NOT_SET = 0,
  TRANSPORT_UDP = 1,
  TRANSPORT_INTERLEAVED = 2
};

// optional transport parameters
extern const std::string REUSE_CONNECTION;
extern const std::string PREFERRED_DELIVERY_PROTOCOL;
extern const std::string PROXY_URL_SUFFIX;

class TransportParameter
{
public:
  TransportParameter(bool bReuseConnection = false, DeliveryProtocol eDeliveryProtocol = TRANSPORT_NOT_SET, const std::string& sProxyUrlSuffix = "");

  std::string toString() const;
private:
  bool m_bReuseConnection;
  DeliveryProtocol m_eDeliveryProtocol;
  std::string m_sProxyUrlSuffix;
};

} // rfc2326_ext
} // rtp_plus_plus
