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
namespace rfc2326
{

class RtspClientConnection;
typedef boost::shared_ptr<RtspClientConnection> RtspClientConnectionPtr;

class RtspClientConnection : public boost::enable_shared_from_this<RtspClientConnection>,
    private boost::noncopyable
{
  enum MessageType
  {
    MT_RTSP,
    MT_RTP_RTCP
  };
public:

  typedef boost::shared_ptr<RtspClientConnection> ptr;
  typedef boost::shared_array< char > MessageBuffer_t;
  typedef std::tuple<uint32_t, std::string, MessageType> RtspMessage_t;
  typedef std::deque<RtspMessage_t> RtspMessageQueue_t;
  typedef boost::function<void (const boost::system::error_code&, const std::string&, RtspClientConnection::ptr)> ReadCompletionHandler;
  typedef boost::function<void (const boost::system::error_code&, uint32_t, const std::string&, RtspClientConnection::ptr)> WriteCompletionHandler;
  typedef boost::function<void (const boost::system::error_code&, NetworkPacket, const EndPoint&, RtspClientConnection::ptr)> RtpRtcpReadCompletionHandler;
  typedef boost::function<void (const boost::system::error_code&, const std::string&, RtspClientConnection::ptr)> RtpRtcpWriteCompletionHandler;
  typedef boost::function<void (RtspClientConnection::ptr)> CloseHandler;
  typedef boost::function<void (const boost::system::error_code&, RtspClientConnection::ptr)> ErrorHandler;

  static ptr create( boost::asio::io_service& io_service, const std::string& sIp, uint16_t uiPort );
  RtspClientConnection( boost::asio::io_service& io_service, const std::string& sIp, uint16_t uiPort );
  ~RtspClientConnection();

  bool isConnected() const { return m_eState == CONNECTION_CONNECTED; }
  bool eof() const { return m_eState == CONNECTION_CLOSED || m_eState == CONNECTION_ERROR;  }
  boost::asio::ip::tcp::socket& socket() { return m_socket; }
  std::string getIpOrHost() const { return m_sIpOrHostName; }
  unsigned short getPort() const { return m_uiPort; }
  std::string getServerIp() const { return m_socket.remote_endpoint().address().to_string(); }
  void setReadCompletionHandler(ReadCompletionHandler val) { m_readCompletionHandler = val; }
  void setWriteCompletionHandler(WriteCompletionHandler val) { m_writeCompletionHandler = val; }
  void registerRtpRtcpReadCompletionHandler(uint32_t uiChannel, RtpRtcpReadCompletionHandler val);
  void registerRtpRtcpWriteCompletionHandler(uint32_t uiChannel, RtpRtcpWriteCompletionHandler val);
  void setCloseHandler(CloseHandler val) { m_closeHandler = val; }
  void setConnectErrorHandler(ErrorHandler val) { m_connectErrorHandler = val; }

  // starts reading
  boost::system::error_code connect();

  /**
   * @brief write sends standard RTSP messages to the RTSP server
   * @param uiMessageId
   * @param sRtspMessage The RTSP message to be sent
   */
  void write(uint32_t uiMessageId, const std::string& sRtspMessage);

  /**
   * @brief writeInterleavedData sends RTP or RTCP packets to the server
   * @param uiChannel The channel that the message is to be sent over
   * @param sRtpRtcp The RTP or RTCP message that has been packetised into a string
   */
  void writeInterleavedData(uint32_t uiChannel, const Buffer data);

  /**
   * @brief close closes the RTSP TCP connection
   */
  void close();

private:

  void handleConnect( const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator endpointIterator );
  void handleRead( const boost::system::error_code& error, std::size_t bytesTransferred );
  void handleReadBody(const boost::system::error_code& error, std::size_t bytesTransferred);
  void handleReadRtpLength(const boost::system::error_code& error, std::size_t bytesTransferred);
  void handleReadRtpMessage(const boost::system::error_code& error, std::size_t bytesTransferred);
  void handleWrite(const boost::system::error_code& error, std::size_t bytes_transferred);
  void handleClose();
  void doClose();

  // reads message body from stream buffer
  void readRtpMessageBodyIntoBuffer(std::istream& is, size_t uiRtpRtcpSize);

  // returns the number of bytes processed from the stream
  int processRtpMessage(std::istream& is, size_t iBytesToBeProcessed);

  // calls the handler for the channel with the appropriate parameters
  void outputRtpRtcpPacket(uint32_t uiChannel, uint32_t uiLength);

  // io service
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

  enum ParseState
  {
    PARSING_RTSP,
    PARSING_RTP
  };
  ParseState m_eParseState;

  /// Storage for message during async write
  std::string m_sMessage;
  /// Buffer for async reads
  boost::asio::streambuf m_streamBuffer;

  Buffer m_incomingMessageBuffer;
  uint32_t m_uiMessageLength;
  uint32_t m_uiMessageBodyLength;

  /// Queue for RTSP requests
  RtspMessageQueue_t m_qRtspRequests;
  /// lock for queue
  boost::mutex m_lock;

  /// Handlers
  ReadCompletionHandler m_readCompletionHandler;
  WriteCompletionHandler m_writeCompletionHandler;

  // registered RTP/RTCP message handlers
  std::unordered_map<uint32_t, RtpRtcpReadCompletionHandler> m_mRtpRtcpReadCompletionHandlers;
  std::unordered_map<uint32_t, RtpRtcpWriteCompletionHandler> m_mRtpRtcpWriteCompletionHandlers;

  CloseHandler m_closeHandler;
  ErrorHandler m_connectErrorHandler;
};

} // rfc2326
} // rtp_plus_plus
