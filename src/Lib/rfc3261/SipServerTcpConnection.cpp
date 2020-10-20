#include "CorePch.h"
#include <rtp++/rfc3261/SipServerTcpConnection.h>
#include <boost/asio/error.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <rtp++/rfc2326/RtspUtil.h>

#define DEBUG_SIP

namespace rtp_plus_plus
{
namespace rfc3261
{

unsigned SipServerTcpConnection::m_UniqueId = 0;
static const unsigned DEFAULT_BUFFER_SIZE = 512;

SipServerTcpConnectionPtr SipServerTcpConnection::create( boost::asio::io_service& io_service )
{
  return boost::make_shared<SipServerTcpConnection>( boost::ref(io_service) );
}

SipServerTcpConnection::SipServerTcpConnection( boost::asio::io_service& io_service )
  :m_socket(io_service),
  m_eState(CONNECTION_READY),
  m_uiId(m_UniqueId++),
  m_incomingMessageBuffer(new unsigned char[DEFAULT_BUFFER_SIZE], DEFAULT_BUFFER_SIZE),
  m_uiMessageLength(0),
  m_uiMessageBodyLength(0)
{
  VLOG(15) << "[" << this << "] Constructor";
}

SipServerTcpConnection::~SipServerTcpConnection()
{
  VLOG(15) << "[" << this << "] Destructor";
  m_socket.close();
  m_eState = CONNECTION_CLOSED;
}

void SipServerTcpConnection::read()
{
  /// Create task for reading response
  boost::asio::async_read_until(m_socket,
    m_streamBuffer,
    "\r\n\r\n",
    boost::bind(&SipServerTcpConnection::handleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
    );
}

void SipServerTcpConnection::start()
{
  // This step assumed that the socket accept was successful
  m_eState = CONNECTION_ACTIVE;

  m_sClientIp = m_socket.remote_endpoint().address().to_string();
  m_uiClientPort = m_socket.remote_endpoint().port();

  VLOG(2) << "[" << this << "] Connection from IP: " << m_sClientIp << ":" << m_uiClientPort;

  read();
}

void SipServerTcpConnection::write(const std::string& sSipMessage)
{
  bool bBusyWriting = !m_qSipRequests.empty();
  m_qSipRequests.push_back(sSipMessage);

  if (m_eState != CONNECTION_ACTIVE)
  {
    // in this case only queue the packets.
    // this would be the case where write has been called before
    // a successful accept or connect
    return;
  }

  if (!bBusyWriting)
  {
    m_sMessage = m_qSipRequests.front();

#ifdef DEBUG_SIP
    VLOG(2) << "[" << this << "] [UAS->UAC]:\r\n" << m_sMessage;
#endif

    boost::asio::async_write(m_socket, boost::asio::buffer(m_sMessage),
      boost::bind(&SipServerTcpConnection::handleWrite, shared_from_this(),
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));
  }
}

void SipServerTcpConnection::handleClose()
{
#ifdef DEBUG_SIP
  VLOG(2)<< "[" << this << "]  closing connection";
#endif
  // Notify about closing of connection
  m_closeHandler( shared_from_this() );

  // Close socket
  close();
}

void SipServerTcpConnection::handleRead( const boost::system::error_code& error, std::size_t bytesTransferred )
{
  if (!error )
  {
    std::istream is(&m_streamBuffer);
    size_t size = m_streamBuffer.size();

    // check if buffer is big enough
    if (m_incomingMessageBuffer.getSize() < bytesTransferred + 1)
    {
      m_incomingMessageBuffer.setData(new uint8_t[bytesTransferred + 1], bytesTransferred + 1);
    }

    // store message length so we don't overwrite it later
    m_uiMessageLength = bytesTransferred;

    m_incomingMessageBuffer[bytesTransferred] = '\0';
    is.read((char*)&m_incomingMessageBuffer[0], m_uiMessageLength);

    // HACK: for now do scan f for Content-Length header
    // Content-Length: 376
    std::string sContent((char*)m_incomingMessageBuffer.data(), bytesTransferred);
    // TODO: refactor getContentLength
    if (rfc2326::getContentLength(sContent, m_uiMessageBodyLength) && m_uiMessageBodyLength > 0)
    {
      // DBG
      VLOG(2) << "[" << this << "] Content-length: " << m_uiMessageBodyLength << " Content read: " << sContent;
      uint32_t uiTotalMessageSize = m_uiMessageLength + m_uiMessageBodyLength + 1;
      if (m_incomingMessageBuffer.getSize() < uiTotalMessageSize)
      {
        uint8_t* pNewBuffer = new uint8_t[uiTotalMessageSize];
        memcpy((char*)pNewBuffer, &m_incomingMessageBuffer[0], m_uiMessageLength);
        m_incomingMessageBuffer.setData(pNewBuffer, uiTotalMessageSize);
      }

      // Note that there might be more data in the stream buffer than bytesTransferred
      size_t overflow = size - bytesTransferred;

      // Build up response string
      // TODO: create message big enough for SIP message + message body
//      m_messageBuffer = MessageBuffer_t( new char[uiContentLength] );
      size_t to_read = m_uiMessageBodyLength - overflow;
      VLOG(2) << "[" << this << "] Need to read message body " << m_uiMessageBodyLength
              << " Overflow: " << overflow
              << " TO be read: " << to_read;

      boost::asio::async_read(m_socket,
        m_streamBuffer, boost::asio::transfer_at_least(to_read),
        boost::bind(&SipServerTcpConnection::handleReadBody, shared_from_this(),
        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    else
    {
#ifdef DEBUG_SIP
      VLOG(2) << "[UAC->UAS]:\r\n" << m_incomingMessageBuffer.data();
#endif

      // No message body
      if (m_readCompletionHandler)
      {
        std::string sSipMessage((char*)&m_incomingMessageBuffer[0], bytesTransferred);
        m_readCompletionHandler(error, sSipMessage, shared_from_this());
      }

      /// Create task for reading next message
      boost::asio::async_read_until( m_socket,
        m_streamBuffer,
        "\r\n\r\n",
        boost::bind(&SipServerTcpConnection::handleRead, shared_from_this(),  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
        );
    }
  }
  else
  {
    // verbose logging: let calling class handle error io
    if (error != boost::asio::error::eof)
      VLOG(10) << "[" << this << "] Error in read: " << error.message();

    if (m_readCompletionHandler)
    {
      m_readCompletionHandler(error, "", shared_from_this());
    }
    // Check if connection has been closed
    handleClose();
  }
}

void SipServerTcpConnection::handleReadBody( const boost::system::error_code& error, std::size_t bytesTransferred )
{
  if (!error)
  {
    // TODO: check if Content-Length == bytesTransferred?
    std::istream is(&m_streamBuffer);
    size_t size = m_streamBuffer.size();
    assert(size >= m_uiMessageBodyLength);
    LOG(INFO) << "[" << this << "] TODO: Read body: bytesTransferred: " << bytesTransferred << " size: " << size << " m_uiMessageBodyLength: " << m_uiMessageBodyLength ;
    // TODO: what if size includes bytes from next request?
    // if (size > 0)
    {
      is.read((char*)&m_incomingMessageBuffer[m_uiMessageLength], m_uiMessageBodyLength);
    }

    uint32_t uiTotalMessageSize = m_uiMessageLength + m_uiMessageBodyLength;

    std::string sSipMessage((char*)&m_incomingMessageBuffer[0], uiTotalMessageSize);

#ifdef DEBUG_SIP
    VLOG(2) << "[" << this << "] [UAC->UAS MB] " << sSipMessage;
#endif

    m_readCompletionHandler(error, sSipMessage, shared_from_this());

    /// Create task for reading next message
    boost::asio::async_read_until( m_socket,
      m_streamBuffer,
      "\r\n\r\n",
      boost::bind(&SipServerTcpConnection::handleRead, shared_from_this(),  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
      );
  }
  else
  {
    LOG(WARNING) << "[" << this << "] Error in read body: " << error.message();

    if (m_readCompletionHandler)
    {
      m_readCompletionHandler(error, "", shared_from_this());
    }
    handleClose();
  }
}

void SipServerTcpConnection::writeIfRequestQueued()
{
  assert(m_eState == CONNECTION_ACTIVE);
  if (!m_qSipRequests.empty())
  {
    m_sMessage = m_qSipRequests.front();
#ifdef DEBUG_SIP
    VLOG(2) << "[" << this << "] [UAS->UAC] " << m_sMessage;
#endif

    boost::asio::async_write(m_socket, boost::asio::buffer(m_sMessage),
      boost::bind(&SipServerTcpConnection::handleWrite, shared_from_this(),
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));
  }
}

void SipServerTcpConnection::handleWrite( const boost::system::error_code& error, size_t bytes_transferred)
{
  if (!error)
  {
    // Remove request from queue
    std::string sSipRequest = m_qSipRequests.front();
    m_qSipRequests.pop_front();
    if (m_writeCompletionHandler)
    {
      m_writeCompletionHandler(error, sSipRequest, shared_from_this());
    }

    writeIfRequestQueued();
  }
  else
  {
    LOG(WARNING) << "[" << this << "] Error on write: " << error.message();
    if (m_writeCompletionHandler)
    {
      // Remove request from queue
      std::string sSipRequest = m_qSipRequests.front();
      m_qSipRequests.pop_front();
      m_writeCompletionHandler(error, sSipRequest, shared_from_this());
    }
    // Check if connection has been closed
    handleClose();
  }
}

} // rfc3261
} // rtp_plus_plus
