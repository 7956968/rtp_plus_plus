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
namespace rfc2326
{

class RtspServerConnection;
typedef boost::shared_ptr<RtspServerConnection> RtspServerConnectionPtr;

class RtspServerConnection : public boost::enable_shared_from_this<RtspServerConnection>,
    private boost::noncopyable
{
public:
  typedef boost::shared_ptr<RtspServerConnection> ptr;
  typedef boost::shared_array< char > MessageBuffer_t;
  typedef std::pair<uint32_t, std::string> RtspMessage_t;
  typedef std::deque<RtspMessage_t> RtspMessageQueue_t;
  typedef boost::function<void (const boost::system::error_code&, const std::string&, RtspServerConnection::ptr)> ReadCompletionHandler;
  typedef boost::function<void (const boost::system::error_code&, uint32_t, const std::string&, RtspServerConnection::ptr)> WriteCompletionHandler;
  typedef boost::function<void (RtspServerConnection::ptr)> CloseHandler;

  static ptr create( boost::asio::io_service& io_service );
  RtspServerConnection( boost::asio::io_service& io_service );
  ~RtspServerConnection();

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

  void write(uint32_t uiMessageId, const std::string& sRtspMessage);

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

  /// Queue for RTSP requests
  RtspMessageQueue_t m_qRtspRequests;

  /// Handlers
  ReadCompletionHandler m_readCompletionHandler;
  WriteCompletionHandler m_writeCompletionHandler;
  CloseHandler m_closeHandler;
};

} // rfc2326
} // rtp_plus_plus
