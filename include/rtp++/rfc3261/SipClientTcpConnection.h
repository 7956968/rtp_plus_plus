#pragma once
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/function.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/utility.hpp>
#include <cpputil/Buffer.h>
#include <rtp++/network/EndPoint.h>
#include <rtp++/network/NetworkPacket.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

class SipClientTcpConnection;
typedef boost::shared_ptr<SipClientTcpConnection> SipClientTcpConnectionPtr;

/**
 * @brief The SipClientTcpConnection class is used by the UAC to send SIP requests to a UAS.
 */
class SipClientTcpConnection : public boost::enable_shared_from_this<SipClientTcpConnection>,
    private boost::noncopyable
{
public:

  typedef boost::shared_ptr<SipClientTcpConnection> ptr;
  typedef boost::shared_array< char > MessageBuffer_t;
  typedef std::deque<std::string> SipMessageQueue_t;
  typedef boost::function<void (const boost::system::error_code&, const std::string&, SipClientTcpConnection::ptr)> ReadCompletionHandler;
  typedef boost::function<void (const boost::system::error_code&, const std::string&, SipClientTcpConnection::ptr)> WriteCompletionHandler;
  typedef boost::function<void (SipClientTcpConnection::ptr)> CloseHandler;
  typedef boost::function<void (const boost::system::error_code&, SipClientTcpConnection::ptr)> ErrorHandler;

  static ptr create( boost::asio::io_service& io_service, const std::string& sIp, uint16_t uiPort );
  SipClientTcpConnection( boost::asio::io_service& io_service, const std::string& sIp, uint16_t uiPort );
  ~SipClientTcpConnection();

  bool isConnected() const { return m_eState == CONNECTION_CONNECTED; }
  bool eof() const { return m_eState == CONNECTION_CLOSED || m_eState == CONNECTION_ERROR;  }
  boost::asio::ip::tcp::socket& socket() { return m_socket; }
  std::string getIpOrHost() const { return m_sIpOrHostName; }
  unsigned short getPort() const { return m_uiPort; }
  std::string getServerIp() const { return m_socket.remote_endpoint().address().to_string(); }
  void setReadCompletionHandler(ReadCompletionHandler val) { m_readCompletionHandler = val; }
  void setWriteCompletionHandler(WriteCompletionHandler val) { m_writeCompletionHandler = val; }
  void setCloseHandler(CloseHandler val) { m_closeHandler = val; }
  void setConnectErrorHandler(ErrorHandler val) { m_connectErrorHandler = val; }

  /**
   * @brief starts the asynchronous read operation
   */ 
  boost::system::error_code connect();

  /**
   * @brief write sends SIP message to the SIP UAS
   * @param sSipMessage The SIP message to be sent
   */
  void write(const std::string& sSipMessage);

  /**
   * @brief close closes the SIP TCP connection
   */
  void close();

private:

  void handleConnect( const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator endpointIterator );
  void handleRead( const boost::system::error_code& error, std::size_t bytesTransferred );
  void handleReadBody(const boost::system::error_code& error, std::size_t bytesTransferred);
  void handleWrite(const boost::system::error_code& error, std::size_t bytes_transferred);
  void handleClose();
  void doClose();

  /// io service
  boost::asio::io_service& m_ioService;
  /// Socket
  boost::asio::ip::tcp::socket m_socket;

  /// State of instance
  enum State
  {
    CONNECTION_READY,
    CONNECTION_CONNECTING,
    CONNECTION_CONNECTED,
    CONNECTION_CLOSED,
    CONNECTION_ERROR
  };
  State m_eState;
  /// Client info
  std::string m_sIpOrHostName;
  unsigned short m_uiPort;

  /// Storage for message during async write
  std::string m_sMessage;
  /// Buffer for async reads
  boost::asio::streambuf m_streamBuffer;

  Buffer m_incomingMessageBuffer;
  uint32_t m_uiMessageLength;
  uint32_t m_uiMessageBodyLength;

  /// Queue for SIP requests
  SipMessageQueue_t m_qSipRequests;
  /// lock for queue
  boost::mutex m_lock;

  /// Handlers
  ReadCompletionHandler m_readCompletionHandler;
  WriteCompletionHandler m_writeCompletionHandler;

  CloseHandler m_closeHandler;
  ErrorHandler m_connectErrorHandler;
};

} // rfc3261
} // rtp_plus_plus
