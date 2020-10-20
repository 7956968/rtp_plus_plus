#pragma once

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @brief Supported protocols for SIP exchanges
 */
enum TransactionProtocol
{
  TP_TCP,
  TP_UDP,
  TP_SCTP
};

/**
 * @brief checks if transport is unreliable
 */
static bool isTransportUnreliable(TransactionProtocol eProtocol)
{
  switch (eProtocol)
  {
  case TP_TCP:
  {
    return false;
  }
  case TP_UDP:
  {
    return true;
  }
    // TODO: depends on PR mode?
  case TP_SCTP:
  {
    return true;
  }
  default:
  {
    assert(false);
    return true;
  }
  }
}

} // rfc3261
} // rtp_plus_plus
