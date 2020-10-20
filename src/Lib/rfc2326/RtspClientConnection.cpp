#include "CorePch.h"
#include <rtp++/rfc2326/RtspClientConnection.h>
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
#include <rtp++/rfc2326/RtspUtil.h>
#include <rtp++/RtpTime.h>

// #define DEBUG_RTSP

namespace rtp_plus_plus
{
namespace rfc2326
{

using boost::asio::ip::tcp;

const unsigned DEFAULT_RTSP_BUFFER_SIZE = 512;

RtspClientConnectionPtr RtspClientConnection::create( boost::asio::io_service& io_service, const std::string& sIp, uint16_t uiPort )
{
  return boost::make_shared<RtspClientConnection>( boost::ref(io_service), sIp, uiPort );
}

RtspClientConnection::RtspClientConnection( boost::asio::io_service& io_service, const std::string& sIp, uint16_t uiPort )
  :m_ioService(io_service),
    m_socket(io_service),
    m_eState(CONNECTION_READY),
    m_sIpOrHostName(sIp),
    m_uiPort(uiPort),
    m_incomingMessageBuffer(new unsigned char[DEFAULT_RTSP_BUFFER_SIZE], DEFAULT_RTSP_BUFFER_SIZE),
    m_uiMessageLength(0),
    m_uiMessageBodyLength(0)
{

}

RtspClientConnection::~RtspClientConnection()
{
  VLOG(15) << "Destructor";
  doClose();
}

void RtspClientConnection::registerRtpRtcpReadCompletionHandler(uint32_t uiChannel, RtpRtcpReadCompletionHandler val)
{
  VLOG(5) << "Registering read handler for channel " << uiChannel;
  m_mRtpRtcpReadCompletionHandlers[uiChannel] = val;
}

void RtspClientConnection::registerRtpRtcpWriteCompletionHandler(uint32_t uiChannel, RtpRtcpWriteCompletionHandler val)
{
  VLOG(5) << "Registering write handler for channel " << uiChannel;
  m_mRtpRtcpWriteCompletionHandlers[uiChannel] = val;
}

boost::system::error_code RtspClientConnection::connect()
{
  switch (m_eState)
  {
    case CONNECTION_READY:
    {
      tcp::resolver resolver(m_socket.get_io_service());
      tcp::resolver::query query(m_sIpOrHostName, ::toString(m_uiPort));
      tcp::resolver::iterator iterator = resolver.resolve(query);

      tcp::endpoint endpoint = *iterator;
      m_socket.async_connect(endpoint, boost::bind(&RtspClientConnection::handleConnect, shared_from_this(), boost::asio::placeholders::error, ++iterator));
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

void RtspClientConnection::write(uint32_t uiSequenceNumber, const std::string& sRtspMessage)
{
  switch (m_eState)
  {
    case CONNECTION_CONNECTED:
    {
      boost::mutex::scoped_lock l(m_lock);
      bool bBusyWriting = !m_qRtspRequests.empty();
      m_qRtspRequests.push_back(std::make_tuple(uiSequenceNumber, sRtspMessage, MT_RTSP));

      if (!bBusyWriting)
      {
        RtspMessage_t rtspMessage = m_qRtspRequests.front();
        m_sMessage = std::get<1>(rtspMessage);

    #ifdef DEBUG_RTSP
        VLOG(2) << "[S->C]:\r\n" << m_sMessage;
    #endif

        boost::asio::async_write(m_socket, boost::asio::buffer(m_sMessage),
          boost::bind(&RtspClientConnection::handleWrite, shared_from_this(),
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
        m_qRtspRequests.push_back(std::make_tuple(uiSequenceNumber, sRtspMessage, MT_RTSP));
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
      m_qRtspRequests.push_back(std::make_tuple(uiSequenceNumber, sRtspMessage, MT_RTSP));
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

void RtspClientConnection::writeInterleavedData(uint32_t uiChannel, const Buffer data)
{
  // Here we assume that the connection is already connected since the RTSP exchange must already have taken place
  switch (m_eState)
  {
    case CONNECTION_CONNECTED:
    {
      boost::mutex::scoped_lock l(m_lock);

      OBitStream ob(data.getSize() + 4);
      ob.write8Bits(0x24);
      ob.write8Bits(uiChannel);
      ob.write8Bits((uint8_t)((data.getSize() >> 8) & 0xFF));
      ob.write8Bits((uint8_t)((data.getSize()) & 0xFF));
      IBitStream ib(data);
      ob.write(ib);

      bool bBusyWriting = !m_qRtspRequests.empty();
      m_qRtspRequests.push_back(std::make_tuple(uiChannel, ob.data().toStdString(), MT_RTP_RTCP));

      if (!bBusyWriting)
      {
        RtspMessage_t rtspMessage = m_qRtspRequests.front();
        m_sMessage = std::get<1>(rtspMessage);

    #ifdef DEBUG_RTSP
        VLOG(2) << "[S->C]:\r\n" << m_sMessage;
    #endif

        boost::asio::async_write(m_socket, boost::asio::buffer(m_sMessage),
          boost::bind(&RtspClientConnection::handleWrite, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
      }
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

void RtspClientConnection::handleConnect( const boost::system::error_code& error, tcp::resolver::iterator endpointIterator )
{
  if (!error)
  {
    m_eState = CONNECTION_CONNECTED;
    boost::regex regex("(\r\n\r\n)|(\\$)");
    // Start read for server commands, responses and media data
    boost::asio::async_read_until( m_socket, m_streamBuffer, regex,
                                   boost::bind(&RtspClientConnection::handleRead,
                                               shared_from_this(),  boost::asio::placeholders::error,
                                               boost::asio::placeholders::bytes_transferred));

    if (!m_qRtspRequests.empty())
    {
      RtspMessage_t rtspMessage = m_qRtspRequests.front();
      m_sMessage = std::get<1>(rtspMessage);

  #ifdef DEBUG_RTSP
      VLOG(2) << "[S->C]:\r\n" << m_sMessage;
  #endif

      boost::asio::async_write(m_socket, boost::asio::buffer(m_sMessage),
        boost::bind(&RtspClientConnection::handleWrite, shared_from_this(),
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
        m_socket.async_connect(endpoint, boost::bind(&RtspClientConnection::handleConnect, shared_from_this(), boost::asio::placeholders::error, ++endpointIterator) );
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

void RtspClientConnection::close()
{
  m_ioService.post(boost::bind(&RtspClientConnection::doClose, shared_from_this()));
}

void RtspClientConnection::doClose()
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

void RtspClientConnection::handleClose()
{
#ifdef DEBUG_RTSP
  VLOG(2) << this << " closing connection";
#endif
  // Notify about closing of connection
  m_closeHandler( shared_from_this() );

  // Close socket
  close();
}

void RtspClientConnection::handleRead( const boost::system::error_code& error, std::size_t bytesTransferred )
{
  VLOG(15) << "handleRead " << bytesTransferred;

  if (!error )
  {
    std::istream is(&m_streamBuffer);
    size_t size = m_streamBuffer.size();

    int firstChar = is.peek();
    if (firstChar == 0x24)
    {
      // we're parsing an interleaved RTP packet
      // check if we've managed to read the length
      if (size < 4)
      {
        // launch next read to at least read interleaving header
        size_t to_read = 4 - size;
        boost::asio::async_read(m_socket,
                                m_streamBuffer, boost::asio::transfer_at_least(to_read),
                                boost::bind(&RtspClientConnection::handleReadRtpLength, shared_from_this(),
                                            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
      }
      else
      {
        // read length and start read for content
        is.read((char*)&m_incomingMessageBuffer[0], 4);
        // now check message length
        uint32_t uiRtpRtcpSize = (m_incomingMessageBuffer[2] << 8) | m_incomingMessageBuffer[3];
        m_uiMessageBodyLength = uiRtpRtcpSize;
        VLOG(10) << "Read RTP message length: " << uiRtpRtcpSize;
        // check if we've already read enough
        int iToBeRead = uiRtpRtcpSize + 4 - size;
        if (iToBeRead > 0)
        {
          boost::asio::async_read(m_socket,
                                  m_streamBuffer, boost::asio::transfer_at_least(iToBeRead),
                                  boost::bind(&RtspClientConnection::handleReadRtpMessage, shared_from_this(),
                                              boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
        else
        {
          is.read((char*)&m_incomingMessageBuffer[4], uiRtpRtcpSize);
          // output RTP/RTCP message
#if 0
          std::string sRtpMessage((char*)&m_incomingMessageBuffer[0], uiRtpRtcpSize + 4);
          m_readCompletionHandler(boost::system::error_code(), sRtpMessage, shared_from_this());
#else
          uint32_t uiChannel = static_cast<uint32_t>(m_incomingMessageBuffer[1]);
          outputRtpRtcpPacket(uiChannel, uiRtpRtcpSize);
#endif
          /// Create task for reading next message
          boost::asio::async_read_until( m_socket,
                                         m_streamBuffer,
                                         boost::regex("(\r\n\r\n)|(\\$)"),
                                         boost::bind(&RtspClientConnection::handleRead, shared_from_this(),  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
                                         );
        }
      }
    }
    else
    {
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
      if (getContentLength(sContent, m_uiMessageBodyLength) && m_uiMessageBodyLength > 0)
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
        // TODO: create message big enough for RTSP message + message body
        //      m_messageBuffer = MessageBuffer_t( new char[uiContentLength] );
        size_t to_read = m_uiMessageBodyLength - overflow;

        boost::asio::async_read(m_socket,
                                m_streamBuffer, boost::asio::transfer_at_least(to_read),
                                boost::bind(&RtspClientConnection::handleReadBody, shared_from_this(),
                                            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
      }
      else
      {
#ifdef DEBUG_RTSP
        VLOG(2) << "[C->S]:\r\n" << m_incomingMessageBuffer.data();
#endif

        // No message body
        if (m_readCompletionHandler)
        {
          std::string sRtspMessage((char*)&m_incomingMessageBuffer[0], bytesTransferred);
          m_readCompletionHandler(error, sRtspMessage, shared_from_this());
        }

        /// Create task for reading next message
        boost::asio::async_read_until( m_socket,
                                       m_streamBuffer,
                                       boost::regex("(\r\n\r\n)|(\\$)"),
                                       boost::bind(&RtspClientConnection::handleRead, shared_from_this(),  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
                                       );
      }
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

void RtspClientConnection::handleReadRtpLength(const boost::system::error_code& error, std::size_t bytesTransferred )
{
  if (!error)
  {
    // we have at leat 4 bytes in the buffer containing the RTP packet length
    // TODO: check if Content-Length == bytesTransferred?
    std::istream is(&m_streamBuffer);
    uint32_t size = m_streamBuffer.size();

    // TODO: should be in loop: same as other method
    // read length and start read for content
    is.read((char*)&m_incomingMessageBuffer[0], 4);
    // now check message length
    uint32_t uiRtpRtcpSize = (m_incomingMessageBuffer[2] << 8) | m_incomingMessageBuffer[3];
    m_uiMessageBodyLength = uiRtpRtcpSize;
    VLOG(10) << "Read RTP message length: " << uiRtpRtcpSize;
    // check if we've already read enough
    int iToBeRead = uiRtpRtcpSize + 4 - size;

    VLOG(10) << "Trans: " << bytesTransferred
            << " stream_buf: " << m_streamBuffer.size()
            << " ToBeRead: " << iToBeRead
            << " RTP/RTCP: " << uiRtpRtcpSize;

    if (iToBeRead > 0)
    {
      boost::asio::async_read(m_socket,
                              m_streamBuffer, boost::asio::transfer_at_least(iToBeRead),
                              boost::bind(&RtspClientConnection::handleReadRtpMessage, shared_from_this(),
                                          boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      is.read((char*)&m_incomingMessageBuffer[4], uiRtpRtcpSize);
      // output RTP/RTCP message

#if 0
      std::string sRtpMessage((char*)&m_incomingMessageBuffer[0], uiRtpRtcpSize + 4);
      m_readCompletionHandler(boost::system::error_code(), sRtpMessage, shared_from_this());
#else
      uint32_t uiChannel = static_cast<uint32_t>(m_incomingMessageBuffer[1]);
      outputRtpRtcpPacket(uiChannel, uiRtpRtcpSize);
#endif


      /// Create task for reading next message
      boost::asio::async_read_until( m_socket,
                                     m_streamBuffer,
                                     boost::regex("(\r\n\r\n)|(\\$)"),
                                     boost::bind(&RtspClientConnection::handleRead, shared_from_this(),  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
                                     );
    }
  }
  else
  {
    LOG(WARNING) << "Error in read RTP length: " << error.message();

    // update state to error here: the read handler might try to use the same connection!
    m_eState = CONNECTION_ERROR;
    if (m_readCompletionHandler)
    {
      m_readCompletionHandler(error, "", shared_from_this());
    }
    handleClose();
  }
}

inline void RtspClientConnection::outputRtpRtcpPacket(uint32_t uiChannel, uint32_t uiLength)
{
  VLOG(15) << "Outputting RTP/RTCP packet for channel " << uiChannel << " Length: " << uiLength;
  if (m_mRtpRtcpReadCompletionHandlers.find(uiChannel) != m_mRtpRtcpReadCompletionHandlers.end())
  {
    NetworkPacket networkPacket(RtpTime::getNTPTimeStamp());
    uint8_t* pData = new uint8_t[uiLength];
    memcpy(pData, (const char*)&m_incomingMessageBuffer[4], uiLength);
    networkPacket.setData(pData, uiLength);
    EndPoint ep(m_socket.remote_endpoint().address().to_string(), m_socket.remote_endpoint().port() );
    m_mRtpRtcpReadCompletionHandlers[uiChannel](boost::system::error_code(), networkPacket, ep, shared_from_this());
  }
  else
    LOG(WARNING) << "No handler for RTP/RTCP channel " << uiChannel;
}

void RtspClientConnection::handleReadRtpMessage(const boost::system::error_code& error, std::size_t bytesTransferred )
{
  if (!error)
  {
    // check if buffer is big enough
    if (m_incomingMessageBuffer.getSize() < m_uiMessageBodyLength + 4)
    {
      uint32_t uiNewSize = m_uiMessageBodyLength + 4;
      uint8_t* pNewBuffer = new uint8_t[uiNewSize];
      // copy old header
      memcpy(pNewBuffer, &m_incomingMessageBuffer[0], 4);
      m_incomingMessageBuffer.setData(pNewBuffer, uiNewSize);
    }

    // TODO: check if Content-Length == bytesTransferred?
    std::istream is(&m_streamBuffer);
    is.read((char*)&m_incomingMessageBuffer[4], m_uiMessageBodyLength);
    // output RTP/RTCP message
#if 0
    std::string sRtpMessage((char*)&m_incomingMessageBuffer[0], m_uiMessageBodyLength + 4);
    m_readCompletionHandler(boost::system::error_code(), sRtpMessage, shared_from_this());
#else
    uint32_t uiChannel = static_cast<uint32_t>(m_incomingMessageBuffer[1]);
    outputRtpRtcpPacket(uiChannel, m_uiMessageBodyLength);
#endif

    /// Create task for reading next message
    boost::asio::async_read_until( m_socket,
                                   m_streamBuffer,
                                   boost::regex("(\r\n\r\n)|(\\$)"),
                                   boost::bind(&RtspClientConnection::handleRead, shared_from_this(),  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
                                   );
  }
  else
  {
    LOG(WARNING) << "Error in read RTP message: " << error.message();

    // update state to error here: the read handler might try to use the same connection!
    m_eState = CONNECTION_ERROR;
    if (m_readCompletionHandler)
    {
      m_readCompletionHandler(error, "", shared_from_this());
    }
    handleClose();
  }
}

void RtspClientConnection::handleReadBody( const boost::system::error_code& error, std::size_t bytesTransferred )
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

    std::string sRtspMessage((char*)&m_incomingMessageBuffer[0], uiTotalMessageSize);

#ifdef DEBUG_RTSP
    VLOG(2) << "[C->S MB] " << sRtspMessage;
#endif

    m_readCompletionHandler(error, sRtspMessage, shared_from_this());

    /// Create task for reading next message
    boost::asio::async_read_until( m_socket,
      m_streamBuffer,
      boost::regex("(\r\n\r\n)|(\\$)"),
      boost::bind(&RtspClientConnection::handleRead, shared_from_this(),  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
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

void RtspClientConnection::handleWrite( const boost::system::error_code& error, size_t bytes_transferred)
{
  if (!error)
  {
    // Remove request from queue
    RtspMessage_t rtspRequest = m_qRtspRequests.front();
    m_qRtspRequests.pop_front();

    // check if it was an RTSP message or RTP/RTCP message

    uint32_t uiId = std::get<0>(rtspRequest);
    const std::string& sMessage = std::get<1>(rtspRequest);
    MessageType eType = std::get<2>(rtspRequest);

    switch( eType )
    {
      case MT_RTSP:
      {
        if (m_writeCompletionHandler)
        {
          m_writeCompletionHandler(error, uiId, sMessage, shared_from_this());
        }
        break;
      }
      case MT_RTP_RTCP:
      {
        // check if we have the appropriate channel handler
        if (m_mRtpRtcpWriteCompletionHandlers.find(uiId) != m_mRtpRtcpWriteCompletionHandlers.end())
        {
//          NetworkPacket networkPacket(RtpTime::getNTPTimeStamp());
//          uint8_t* pData = new uint8_t[uiLength];
//          memcpy(pData, (const char*)&m_incomingMessageBuffer[4], uiLength);
//          networkPacket.setData(pData, uiLength);
//          EndPoint ep(m_socket.remote_endpoint().address().to_string(), m_socket.remote_endpoint().port() );
          // TODO: could return Buffer, etc here, but string should do it for now
          m_mRtpRtcpWriteCompletionHandlers[uiId](boost::system::error_code(), sMessage, shared_from_this());
        }
        else
          LOG(WARNING) << "No write completion handler for channel " << uiId;
        break;
      }
    }


    if ( !m_qRtspRequests.empty() )
    {
      RtspMessage_t rtspRequest = m_qRtspRequests.front();
      m_sMessage = std::get<1>(rtspRequest);
#ifdef DEBUG_RTSP
      VLOG(2) << "[S->C] " << m_sMessage;
#endif

      boost::asio::async_write(m_socket, boost::asio::buffer(m_sMessage),
        boost::bind(&RtspClientConnection::handleWrite, shared_from_this(),
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
      RtspMessage_t rtspRequest = m_qRtspRequests.front();
      m_qRtspRequests.pop_front();
      m_writeCompletionHandler(error, std::get<0>(rtspRequest), std::get<1>(rtspRequest), shared_from_this());
    }
    // Check if connection has been closed
    handleClose();
  }
}

} // rfc2326
} // rtp_plus_plus
