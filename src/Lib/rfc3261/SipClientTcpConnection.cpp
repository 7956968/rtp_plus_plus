#include "CorePch.h"
#include <rtp++/rfc3261/SipClientTcpConnection.h>
#include <boost/algorithm/string.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/OBitStream.h>
#include <rtp++/RtpTime.h>
#include <rtp++/rfc2326/RtspUtil.h>

#define DEBUG_SIP

namespace rtp_plus_plus
{
namespace rfc3261
{

using boost::asio::ip::tcp;

const unsigned DEFAULT_SIP_BUFFER_SIZE = 512;

SipClientTcpConnectionPtr SipClientTcpConnection::create( boost::asio::io_service& io_service, const std::string& sIp, uint16_t uiPort )
{
  return boost::make_shared<SipClientTcpConnection>( boost::ref(io_service), sIp, uiPort );
}

SipClientTcpConnection::SipClientTcpConnection( boost::asio::io_service& io_service, const std::string& sIp, uint16_t uiPort )
  :m_ioService(io_service),
    m_socket(io_service),
    m_eState(CONNECTION_READY),
    m_sIpOrHostName(sIp),
    m_uiPort(uiPort),
    m_incomingMessageBuffer(new unsigned char[DEFAULT_SIP_BUFFER_SIZE], DEFAULT_SIP_BUFFER_SIZE),
    m_uiMessageLength(0),
    m_uiMessageBodyLength(0)
{
  VLOG(10) << "SipClientTcpConnection " << sIp << ":" << uiPort;
}

SipClientTcpConnection::~SipClientTcpConnection()
{
  VLOG(10) << "~SipClientTcpConnection " << m_sIpOrHostName << ":" << m_uiPort;
  doClose();
}

boost::system::error_code SipClientTcpConnection::connect()
{
  switch (m_eState)
  {
    case CONNECTION_READY:
    {
      tcp::resolver resolver(m_socket.get_io_service());
      tcp::resolver::query query(m_sIpOrHostName, ::toString(m_uiPort));
      tcp::resolver::iterator iterator = resolver.resolve(query);

      tcp::endpoint endpoint = *iterator;
      m_socket.async_connect(endpoint, boost::bind(&SipClientTcpConnection::handleConnect, shared_from_this(), boost::asio::placeholders::error, ++iterator));
      m_eState = CONNECTION_CONNECTING;
      return boost::system::error_code();
    }
    default:
    {
      LOG(WARNING) << "Connection in invalid state for connect";
      return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
    }
  }
}

void SipClientTcpConnection::write(const std::string& sSipMessage)
{
  switch (m_eState)
  {
    case CONNECTION_CONNECTED:
    {
      boost::mutex::scoped_lock l(m_lock);
      bool bBusyWriting = !m_qSipRequests.empty();
      m_qSipRequests.push_back(sSipMessage);

      if (!bBusyWriting)
      {
        m_sMessage = m_qSipRequests.front();

    #ifdef DEBUG_SIP
        VLOG(2) << "[UAS->UAC]:\r\n" << m_sMessage;
    #endif

        boost::asio::async_write(m_socket, boost::asio::buffer(m_sMessage),
          boost::bind(&SipClientTcpConnection::handleWrite, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
      }
      break;
    }
    case CONNECTION_READY:
    {
      // store message for later delivery, but start connect
      {
        boost::mutex::scoped_lock l(m_lock);
        m_qSipRequests.push_back(sSipMessage);
      }
      boost::system::error_code ec = connect();
      if (ec)
      {
        LOG(WARNING) << "Failed to connect";
      }
      break;
    }
    case CONNECTION_CONNECTING:
    {
      boost::mutex::scoped_lock l(m_lock);
      m_qSipRequests.push_back(sSipMessage);
      break;
    }
    case CONNECTION_CLOSED:
    case CONNECTION_ERROR:
    default:
    {
      LOG(WARNING) << "Connection is in invalid state for write";
      break;
    }
  }
}

void SipClientTcpConnection::handleConnect( const boost::system::error_code& error, tcp::resolver::iterator endpointIterator )
{
  if (!error)
  {
    m_eState = CONNECTION_CONNECTED;
    boost::regex regex("\r\n\r\n");
    // Start read for server commands, responses and media data
    boost::asio::async_read_until( m_socket, m_streamBuffer, regex,
                                   boost::bind(&SipClientTcpConnection::handleRead,
                                               shared_from_this(),  boost::asio::placeholders::error,
                                               boost::asio::placeholders::bytes_transferred));

    if (!m_qSipRequests.empty())
    {
      m_sMessage = m_qSipRequests.front();

  #ifdef DEBUG_SIP
      VLOG(2) << "[UAS->UAC]:\r\n" << m_sMessage;
  #endif

      boost::asio::async_write(m_socket, boost::asio::buffer(m_sMessage),
        boost::bind(&SipClientTcpConnection::handleWrite, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
    }
  }
  else
  {
    if (error == boost::asio::error::invalid_argument)
    {
      if (endpointIterator == tcp::resolver::iterator())
      {
        // Failed to connect
        LOG(WARNING) << "Failed to connect using endpoint! No more endpoints.";
        m_eState = CONNECTION_ERROR;
        // call error handler?
        if (m_connectErrorHandler)
          m_connectErrorHandler(error, shared_from_this());
      }
      else
      {
        VLOG(2) << "Failed to connect using endpoint! Trying next endpoint.";
        tcp::endpoint endpoint = *endpointIterator;
        m_socket.async_connect(endpoint, boost::bind(&SipClientTcpConnection::handleConnect, shared_from_this(), boost::asio::placeholders::error, ++endpointIterator) );
      }
    }
    else
    {
      VLOG(5) << "Failed to connect: " << error.message();
      m_eState = CONNECTION_ERROR;
      if (m_connectErrorHandler)
        m_connectErrorHandler(error, shared_from_this());
      // can't connect to endpoint
      // handleClose();
    }
  }
}

void SipClientTcpConnection::close()
{
  m_ioService.post(boost::bind(&SipClientTcpConnection::doClose, shared_from_this()));
}

void SipClientTcpConnection::doClose()
{
  if (m_socket.is_open())
  {
    boost::system::error_code ec;
    VLOG(15) << this << " shutting down socket";
    m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    if (ec)
      VLOG(5) << this << " shutting down socket: " << ec.message();
    VLOG(15) << this << " closing socket";
    m_socket.close(ec);
    if (ec)
      VLOG(5) << this << " closed socket: " << ec.message();
    else
      VLOG(15) << this << " closed socket";
    m_eState = CONNECTION_CLOSED;
  }
  else
  {
    VLOG(15) << this << " socket closed";
  }
}

void SipClientTcpConnection::handleClose()
{
#ifdef DEBUG_SIP
  VLOG(2) << this << " closing connection";
#endif
  // Notify about closing of connection
  m_closeHandler( shared_from_this() );

  // Close socket
  close();
}

void SipClientTcpConnection::handleRead( const boost::system::error_code& error, std::size_t bytesTransferred )
{
  VLOG(15) << "handleRead " << bytesTransferred;

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

      boost::asio::async_read(m_socket,
                              m_streamBuffer, boost::asio::transfer_at_least(to_read),
                              boost::bind(&SipClientTcpConnection::handleReadBody, shared_from_this(),
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
                                      boost::regex("\r\n\r\n"),
                                      boost::bind(&SipClientTcpConnection::handleRead, shared_from_this(),  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
                                      );
    }
  }
  else
  {
    // high verbosity level to let calling class control logging
    if (error != boost::asio::error::eof && error != boost::asio::error::operation_aborted)
      VLOG(15) << "Error in read: " << error.message();

    // update state to error here: the read handler might try to use the same connection!
    m_eState = CONNECTION_ERROR;
    if (m_readCompletionHandler)
    {
      m_readCompletionHandler(error, "", shared_from_this());
    }
    // Check if connection has been closed
    handleClose();
  }
}

void SipClientTcpConnection::handleReadBody( const boost::system::error_code& error, std::size_t bytesTransferred )
{
  if (!error)
  {
    // TODO: check if Content-Length == bytesTransferred?
    std::istream is(&m_streamBuffer);
    size_t size = m_streamBuffer.size();
    assert(size >= m_uiMessageBodyLength);
    VLOG(15) << "Read body: bytesTransferred: " << bytesTransferred << " size: " << size << " m_uiMessageBodyLength: " << m_uiMessageBodyLength ;
    is.read((char*)&m_incomingMessageBuffer[m_uiMessageLength], m_uiMessageBodyLength);

    uint32_t uiTotalMessageSize = m_uiMessageLength + m_uiMessageBodyLength;

    std::string sSipMessage((char*)&m_incomingMessageBuffer[0], uiTotalMessageSize);

#ifdef DEBUG_SIP
    VLOG(2) << "[UAC->UAS MB] " << sSipMessage;
#endif

    m_readCompletionHandler(error, sSipMessage, shared_from_this());

    /// Create task for reading next message
    boost::asio::async_read_until( m_socket,
      m_streamBuffer,
      boost::regex("\r\n\r\n"),
      boost::bind(&SipClientTcpConnection::handleRead, shared_from_this(),  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
      );
  }
  else
  {
    LOG(WARNING) << "Error in read body: " << error.message();

    // update state to error here: the read handler might try to use the same connection!
    m_eState = CONNECTION_ERROR;
    if (m_readCompletionHandler)
    {
      m_readCompletionHandler(error, "", shared_from_this());
    }
    handleClose();
  }
}

void SipClientTcpConnection::handleWrite( const boost::system::error_code& error, size_t bytes_transferred)
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

    if ( !m_qSipRequests.empty() )
    {
      m_sMessage = m_qSipRequests.front();
#ifdef DEBUG_SIP
      VLOG(2) << "[UAS->UAC] " << m_sMessage;
#endif

      boost::asio::async_write(m_socket, boost::asio::buffer(m_sMessage),
        boost::bind(&SipClientTcpConnection::handleWrite, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
    }
  }
  else
  {
    LOG(WARNING) << "Error on write: " << error.message();
    // update state to error here: the read handler might try to use the same connection!
    m_eState = CONNECTION_ERROR;
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

