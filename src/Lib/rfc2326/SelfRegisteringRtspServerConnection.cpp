#include "CorePch.h"
#include <rtp++/rfc2326/SelfRegisteringRtspServerConnection.h>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <cpputil/Conversion.h>

namespace rtp_plus_plus
{

using boost::asio::ip::tcp;

SelfRegisteringRtspServerConnection::ptr SelfRegisteringRtspServerConnection::create(boost::asio::io_service& io_service, const std::string& sIpOrHostName, uint16_t uiPort)
{
  return boost::make_shared<SelfRegisteringRtspServerConnection>(boost::ref(io_service), sIpOrHostName, uiPort);
}

SelfRegisteringRtspServerConnection::SelfRegisteringRtspServerConnection(boost::asio::io_service& io_service, const std::string& sIpOrHostName, uint16_t uiPort)
  :rfc2326::RtspServerConnection(io_service),
  m_sIpOrHostName(sIpOrHostName),
  m_uiPort(uiPort)
{
  VLOG(10) << "[" << this << "] SelfRegisteringRtspServerConnection Constructor";
}

SelfRegisteringRtspServerConnection::~SelfRegisteringRtspServerConnection()
{
  VLOG(10) << "[" << this << "] SelfRegisteringRtspServerConnection Constructor";
}

boost::system::error_code SelfRegisteringRtspServerConnection::connect()
{
  switch (m_eState)
  {
  case CONNECTION_READY:
  {
    tcp::resolver resolver(socket().get_io_service());
    tcp::resolver::query query(m_sIpOrHostName, ::toString(m_uiPort));
    tcp::resolver::iterator iterator = resolver.resolve(query);

    tcp::endpoint endpoint = *iterator;
    socket().async_connect(endpoint, boost::bind(&SelfRegisteringRtspServerConnection::handleConnect, 
      boost::static_pointer_cast<SelfRegisteringRtspServerConnection>(shared_from_this()), 
      boost::asio::placeholders::error, 
      ++iterator));

    m_eState = CONNECTION_CONNECTING;
    return boost::system::error_code();
  }
  default:
  {
    return boost::system::error_code(boost::system::errc::invalid_argument, boost::system::get_generic_category());
  }
  }
}

void SelfRegisteringRtspServerConnection::handleConnect(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator endpointIterator)
{
  if (!error)
  {
    m_eState = CONNECTION_ACTIVE;
    // update member vars of parent class: this is usually set when a client connects
    m_sClientIp = m_sIpOrHostName;
    m_uiClientPort = m_uiPort;
#if 0
    boost::regex regex("(\r\n\r\n)|(\\$)");
    // Start read for server commands, responses and media data
    boost::asio::async_read_until(m_socket, m_streamBuffer, regex,
      boost::bind(&SelfRegisteringRtspServerConnection::handleRead,
      shared_from_this(), boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));
#endif
    read();

    writeIfRequestQueued();
#if 0
    if (!m_qRtspRequests.empty())
    {
      RtspMessage_t rtspMessage = m_qRtspRequests.front();
      m_sMessage = std::get<1>(rtspMessage);

#ifdef DEBUG_RTSP
      VLOG(2) << "[S->C]:\r\n" << m_sMessage;
#endif

      boost::asio::async_write(m_socket, boost::asio::buffer(m_sMessage),
        boost::bind(&SelfRegisteringRtspServerConnection::handleWrite, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
    }
#endif
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
        if (m_connectErrorHandler)
        {
          m_connectErrorHandler(error, boost::static_pointer_cast<SelfRegisteringRtspServerConnection>(shared_from_this()));
        }
        else
        {
          LOG(WARNING) << "No connect error handler set";
        }
      }
      else
      {
        VLOG(2) << "Failed to connect using endpoint! Trying next endpoint.";
        tcp::endpoint endpoint = *endpointIterator;
        m_socket.async_connect(endpoint, boost::bind(&SelfRegisteringRtspServerConnection::handleConnect, boost::static_pointer_cast<SelfRegisteringRtspServerConnection>(shared_from_this()), boost::asio::placeholders::error, ++endpointIterator));
      }
    }
    else
    {
      LOG(WARNING) << "Failed to connect: " << error.message();
      m_eState = CONNECTION_ERROR;
      if (m_connectErrorHandler)
      {
        m_connectErrorHandler(error, boost::static_pointer_cast<SelfRegisteringRtspServerConnection>(shared_from_this()));
      }
      else
      {
        LOG(WARNING) << "No connect error handler set";
      }
    }
  }
}

} // rtp_plus_plus
