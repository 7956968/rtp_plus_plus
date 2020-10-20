#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <rtp++/network/MediaSessionNetworkManager.h>
#include <rtp++/rfc3261/Registrar.h>
#include <rtp++/rfc3261/UserAgentBase.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @brief The StatelessProxy class uses the TransportLayer directly.
 */
class StatelessProxy : public UserAgentBase
{
public:
  /**
   * @brief Constructor
   */
  StatelessProxy(boost::asio::io_service& ioService, MediaSessionNetworkManager& mediaSessionNetworkManager, uint16_t uiSipPort, boost::system::error_code& ec);
public:

private:
  /**
   * @brief overridden from UserAgentBase
   */
  virtual void doHandleSipMessageFromTransportLayer(const SipMessage& sipMessage, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol);
  /**
   * @brief handles SIP REGISTER request.
   */
  void handleRegister(const SipMessage& sipMessage, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol);
  /**
   * @brief overridden from UserAgentBase
   */
  virtual boost::system::error_code doStart();
  /**
   * @brief overridden from UserAgentBase
   */
  virtual boost::system::error_code doStop();

private:

  /// registrar
  Registrar m_registrar;
};

} // rfc3261
} // rtp_plus_plus
