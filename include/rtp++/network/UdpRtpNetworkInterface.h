#pragma once
#include <deque>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <rtp++/RtpPacket.h>
#include <rtp++/network/RtpNetworkInterface.h>
#include <rtp++/network/UdpSocketWrapper.h>

/// @def DEBUG_RTP Logs first RTP and RTCP packet
// #define DEBUG_RTP

// -DDEBUG_RTP_TS
// #define DEBUG_RTP_TS

/// -DRTP_DEBUG: writes RTP and RTCP packets to file
//#define RTP_DEBUG

#ifdef RTP_DEBUG
  #include <rtp++/RtpDump.h>
#endif

namespace rtp_plus_plus
{

/**
 * @brief This class is the RTP interface between the library and the socket wrapper
 * RTP/RTCP goes in via method calls and comes out via the callback mechanism
 */
class UdpRtpNetworkInterface : public RtpNetworkInterface
{
public:

  /**
   * @brief Constructor
   */
  UdpRtpNetworkInterface( boost::asio::io_service& rIoService, const EndPoint& rtpEp, const EndPoint& rtcpEp, boost::system::error_code& ec );
  /**
  * @brief Constructor
  *
  * This constructor takes previously constructed RTP and RTCP sockets
  */
  UdpRtpNetworkInterface(boost::asio::io_service& rIoService, const EndPoint& rtpEp, const EndPoint& rtcpEp, 
    std::unique_ptr<boost::asio::ip::udp::socket> pRtpSocket, std::unique_ptr<boost::asio::ip::udp::socket> pRtcpSocket);
  /**
   * @brief Destructor
   */
  virtual ~UdpRtpNetworkInterface();
  /**
   * @reset This method should initalise the component and setup all required
   */
  virtual void reset();
  /**
   * @brief If this method is called the component will begin the shutdown sequence
   * \li This will generate an RTCP BYE
   * \li No further sends or receives will occur once shutdown has ben called
   */
  virtual void shutdown();
  /**
   * @brief This method should make the sockets ready for receiving data
   */
  virtual bool recv();

protected:

  void handleRtpPacket(const boost::system::error_code& ec, UdpSocketWrapper::ptr pSource, NetworkPacket networkPacket, const EndPoint& ep);
  void handleRtcpPacket(const boost::system::error_code& ec, UdpSocketWrapper::ptr pSource, NetworkPacket networkPacket, const EndPoint& ep);
  void handleSentRtpPacket(const boost::system::error_code& ec, UdpSocketWrapper::ptr pSource, Buffer buffer, const EndPoint& ep);
  void handleSentRtcpPacket(const boost::system::error_code& ec, UdpSocketWrapper::ptr pSource, Buffer buffer, const EndPoint& ep);

  virtual bool doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp);
  virtual bool doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp);

private:

  /**
   * @brief initialises RTP and RTCP sockets
   */
  void initialise(boost::system::error_code& ec);
  /**
   * @brief initialises previously constructed RTP and RTCP sockets
   */
  void initialiseExistingSockets(std::unique_ptr<boost::asio::ip::udp::socket> pRtpSocket, std::unique_ptr<boost::asio::ip::udp::socket> pRtcpSocket);

private:
  //! io service
  boost::asio::io_service& m_rIoService;
  //! bool to keep track if UDP sockets have been bound
  bool m_bInitialised;
  //! shutdown flag
  bool m_bShuttingDown;
  //! RTP endpoint info
  EndPoint m_rtpEp;
  //! RTCP endpoint info
  EndPoint m_rtcpEp;
  //! RTP Socket
  UdpSocketWrapper::ptr m_pRtpSocket;
  //! RTCP socket
  UdpSocketWrapper::ptr m_pRtcpSocket;

#ifdef RTP_DEBUG
  //! TODO RTP dump class
  RtpDump m_rtpDump;
#endif
};

} // rtp_plus_plus
