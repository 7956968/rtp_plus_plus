#pragma once
#include <cstdint>
#include <deque>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/function.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/utility.hpp>
#include <cpputil/Buffer.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

class SipServerTcpConnection;
typedef boost::shared_ptr<SipServerTcpConnection> SipServerTcpConnectionPtr;

/**
 * @brief The SipServerTcpConnection manages TCP connection management for the SIP server.
 */
class SipServerTcpConnection : public boost::enable_shared_from_this<SipServerTcpConnection>,
    private boost::noncopyable
{
public:
  typedef boost::shared_ptr<SipServerTcpConnection> ptr;
  typedef boost::shared_array< char > MessageBuffer_t;
  typedef std::string SipMessage_t;
  typedef std::deque<SipMessage_t> SipMessageQueue_t;
  typedef boost::function<void (const boost::system::error_code&, const std::string&, SipServerTcpConnection::ptr)> ReadCompletionHandler;
  typedef boost::function<void (const boost::system::error_code&, const std::string&, SipServerTcpConnection::ptr)> WriteCompletionHandler;
  typedef boost::function<void (SipServerTcpConnection::ptr)> CloseHandler;

  static ptr create( boost::asio::io_service& io_service );
  SipServerTcpConnection( boost::asio::io_service& io_service );
  ~SipServerTcpConnection();

  boost::asio::ip::tcp::socket& socket() { return m_socket; }
  unsigned getId() const { return m_uiId; }
  std::string getClientIp() const { return m_sClientIp; }
  unsigned short getClientPort() const { return m_uiClientPort; }
  bool isClosed() const { return m_eState == CONNECTION_CLOSED; }

  void setReadCompletionHandler(ReadCompletionHandler val) { m_readCompletionHandler = val; }
  void setWriteCompletionHandler(WriteCompletionHandler val) { m_writeCompletionHandler = val; }
  void setCloseHandler(CloseHandler val) { m_closeHandler = val; }

  /**
   * @brief start should be called once the socket has been accepted
   */
  void start();

  void write(const std::string& sSipMessage);

  void close() { m_socket.close(); m_eState = CONNECTION_CLOSED; }

protected:
  void read();
  void writeIfRequestQueued();

  void handleRead( const boost::system::error_code& error, std::size_t bytesTransferred );
  void handleReadBody(const boost::system::error_code& error, std::size_t bytesTransferred);
  void handleWrite(const boost::system::error_code& error, std::size_t bytes_transferred);
  void handleClose();

  /// Socket
  boost::asio::ip::tcp::socket m_socket;

  /// State of socket
  enum State
  {
    CONNECTION_READY,      // initial state
    CONNECTION_CONNECTING, // state only used when server initiates connection
    CONNECTION_ACTIVE,     // state after successful accept or connect
    CONNECTION_CLOSED,     // state after close
    CONNECTION_ERROR       // error state
  };
  State m_eState;

  /// ID of instance
  unsigned m_uiId;
  /// static id for id generation
  static unsigned m_UniqueId;

  /// Client info
  std::string m_sClientIp;
  unsigned short m_uiClientPort;

  /// Storage for message during async write
  std::string m_sMessage;
  /// Buffer for async reads
  boost::asio::streambuf m_streamBuffer;

  Buffer m_incomingMessageBuffer;
  uint32_t m_uiMessageLength;
  uint32_t m_uiMessageBodyLength;

  /// Queue for Sip requests
  SipMessageQueue_t m_qSipRequests;

  /// Handlers
  ReadCompletionHandler m_readCompletionHandler;
  WriteCompletionHandler m_writeCompletionHandler;
  CloseHandler m_closeHandler;
};

} // rfc3261
} // rtp_plus_plus
