#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <rtp++/rfc3261/TransportLayer.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @brief Base class for UserAgent and Stateless/statefull proxy
 */
class UserAgentBase
{
  friend class TransportLayer;
public:
  /**
   * @brief Destructor
   */
  virtual ~UserAgentBase();
  /**
   * @brief Getter for transport layer
   */
  TransportLayer& getTransportLayer() { return m_transportLayer; }
  /**
   * @brief starts the User Agent
   */
  boost::system::error_code start();
  /**
   * @brief stops the User Agent
   */
  boost::system::error_code stop();

protected:
  UserAgentBase(boost::asio::io_service& ioService, const std::string& sFQDN, uint16_t uiSipPort);

private:
  /**
   * @brief handles incoming SIP messages from the TransportLayer
   */
  void handleSipMessageFromTransportLayer(const SipMessage& sipMessage, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol);
  /**
   * @brief Subclass must implement behaviour
   */
  virtual void doHandleSipMessageFromTransportLayer(const SipMessage& sipMessage, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol) = 0;
  /**
   * @brief Allows subclasses to perform initialisation functions
   */
  virtual boost::system::error_code doStart() = 0;
  /**
   * @brief Allows subclasses to perform termination functions
   */
  virtual boost::system::error_code doStop() = 0;

protected:

  /// IO service
  boost::asio::io_service& m_ioService;
  /// transport layer
  TransportLayer m_transportLayer;

};

} // rfc3261
} // rtp_plus_plus
