#include "CorePch.h"
#include <rtp++/network/TcpRtpConnection.h>
#include <boost/bind.hpp>
#include <boost/asio/ip/multicast.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/make_shared.hpp>
#include <cpputil/OBitStream.h>
#include <rtp++/RtpTime.h>
#include <rtp++/network/NetworkPacket.h>

#ifndef _WIN32
#define LOG_TCP_INFO
#ifdef LOG_TCP_INFO
#include <netinet/tcp.h>
#endif
#endif

using namespace boost::asio::ip;

namespace rtp_plus_plus
{

TcpRtpConnection::ptr TcpRtpConnection::create(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort)
{
  return boost::make_shared<TcpRtpConnection>(boost::ref(ioService), boost::ref(sBindIp), uiBindPort);
}

TcpRtpConnection::TcpRtpConnection(boost::asio::io_service& ioService)
  :m_rIoService(ioService),
  m_timer(ioService),
  m_socket(m_rIoService),
  m_bConnectionInProgress(false)
{
  VLOG(2) << "TcpRtpConnection constructor";
}

#define BIND_TO_0_0_0_0_FOR_EC2_NAT_TRAVERSAL
TcpRtpConnection::TcpRtpConnection(boost::asio::io_service& ioService, const std::string& sBindIp, unsigned short uiBindPort)
  :m_rIoService(ioService),
  m_timer(ioService),
#ifdef BIND_TO_0_0_0_0_FOR_EC2_NAT_TRAVERSAL
  m_sIpAddress("0.0.0.0"),
#else
  m_sIpAddress(sBindIp),
#endif
  m_uiPort(uiBindPort),
  m_address(boost::asio::ip::address::from_string(m_sIpAddress)),
  m_endpoint(m_address, uiBindPort),
#if 0
  m_socket(m_rIoService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), uiBindPort)),
#else
  m_socket(m_rIoService),
#endif
  m_bConnectionInProgress(false)
{
  initialise();
  VLOG(2) << "TcpRtpConnection constructor: socket bound to " << sBindIp << ":" << uiBindPort;
}

TcpRtpConnection::~TcpRtpConnection()
{
  DLOG(INFO) << "[" << this << "] Destructor";
}

#define TCP_RTP_HEADER_SIZE 2

void TcpRtpConnection::start(tcp::resolver::iterator endpoint_iter)
{
  connect(endpoint_iter);
}

void TcpRtpConnection::start()
{
  boost::asio::async_read(m_socket,
        m_streamBuffer, boost::asio::transfer_at_least(TCP_RTP_HEADER_SIZE),
        boost::bind(&TcpRtpConnection::readHeaderHandler, shared_from_this(),
        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

  //m_socket.async_read_some(boost::asio::buffer(m_buffer),
  //  boost::bind(&TcpRtpConnection::readCompletionHandler, shared_from_this(),
  //  boost::asio::placeholders::error,
  //  boost::asio::placeholders::bytes_transferred));

  //if (m_bTimeOut)
  //{
  //  /* Create a task that will be called if we wait more than 300ms */
  //  m_timer.expires_from_now(boost::posix_time::milliseconds(m_uiTimeoutMs));
  //  m_timer.async_wait(boost::bind(&TcpRtpConnection::timeoutHandler, shared_from_this(), boost::asio::placeholders::error));
  //}

}

void TcpRtpConnection::stop()
{
  m_socket.close();
}

void TcpRtpConnection::connect(tcp::resolver::iterator endpoint_iter)
{
  //tcp::resolver resolver(m_rIoService);
  //// TODO:
  //std::string sRemoteIp = "127.0.0.1";
  //uint16_t uiRemotePort = 49170;
  //tcp::resolver::query query(sRemoteIp, toString(uiRemotePort));
  //tcp::resolver::iterator iterator = resolver.resolve(query);

  tcp::endpoint endpoint = *endpoint_iter;
  socket().async_connect(endpoint, 
    boost::bind(&TcpRtpConnection::handleConnect, shared_from_this(), boost::asio::placeholders::error, ++endpoint_iter));
  m_bConnectionInProgress = true;
}

void TcpRtpConnection::handleConnect( const boost::system::error_code& error, tcp::resolver::iterator endpointIterator )
{
  if (!error)
  {
#ifdef RTVC_DEBUG_XMLP_SOCKET
    LOG_DBG1(rLogger, LOG_FUNCTION, "[%1%] ### Connection Ok ###", this);
#endif
    LOG(INFO) << "Connection success";
    m_bConnectionInProgress = false;
    // start read
    start();
    // check for queued writes
     // send next sample if one was queued in the meantime
    if ( !m_vDeliveryQueue.empty() )
    {
#if 0
      DLOG(INFO) << "Sending queued packet Queue size: " << m_vDeliveryQueue.size();
#endif
      NetworkPackage_t package = m_vDeliveryQueue.front();
      Buffer networkPacket = package.first;
      EndPoint ep = package.second;
      boost::asio::async_write( m_socket,
        boost::asio::buffer(networkPacket.data(), networkPacket.getSize()),
        boost::bind(&TcpRtpConnection::writeCompletionHandler,
        shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred)
        );
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
        // TODO: how to notify application: will a read or write on the unconnected socket trigger
        // an error?
        //m_writeCompletionHandler(error, pRequest, shared_from_this());
      }
      else
      {
        tcp::endpoint endpoint = *endpointIterator;
        socket().async_connect(endpoint, boost::bind(&TcpRtpConnection::handleConnect, 
          shared_from_this(), boost::asio::placeholders::error, ++endpointIterator));
      }
    }
    else
    {
      // TODO: how to notify application: will a read or write on the unconnected socket trigger
      // an error?
      //m_writeCompletionHandler(error, pRequest, shared_from_this());
    }
  }
}

void TcpRtpConnection::send(Buffer networkPacket, const EndPoint& endpoint)
{
  VLOG(10) << "Outgoing data packet of size " << networkPacket.getSize();
  // prepend size for TCP framing
  Buffer newBuffer(new uint8_t[networkPacket.getSize() + 2], networkPacket.getSize() + 2);
  OBitStream out(newBuffer);
  uint16_t uiSize = networkPacket.getSize();
  out.write(uiSize, 16);

  const uint8_t* pDest = networkPacket.data();
  out.writeBytes(pDest, uiSize);

  boost::mutex::scoped_lock l(m_lock);
  bool bBusyWriting = !m_vDeliveryQueue.empty();
  m_vDeliveryQueue.push_back( std::make_pair(newBuffer, endpoint) );
  if (!m_bConnectionInProgress && !bBusyWriting)
  {
    boost::asio::async_write(m_socket, boost::asio::buffer(newBuffer.data(), newBuffer.getSize()),
      boost::bind(&TcpRtpConnection::writeCompletionHandler,
      shared_from_this(),
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred)
      );
  }
  else
  {
#if 1
    VLOG(10) << "Packet queued, connection or write in progress";
#endif
  }
}

void TcpRtpConnection::close()
{
  VLOG(1) << "[" << this << "] Closing socket [" << m_sIpAddress << ":" << m_uiPort << "]";
  m_socket.close();
}

void TcpRtpConnection::initialise()
{
  m_socket.open(m_endpoint.protocol());
  // Binding to port 0 should result in the OS selecting the port
  VLOG(1) << "[" << this << "] Binding socket [" << m_sIpAddress << ":" << m_uiPort << "]";

  // this needs to happen before the bind
  boost::asio::socket_base::reuse_address option(true);
  m_socket.set_option(option);

  boost::system::error_code ec;
  //m_socket.bind(m_endpoint, ec);
  //m_socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), m_uiPort), ec);
  m_socket.bind(m_endpoint, ec);

  if (ec)
  {
    LOG(WARNING) << "Error binding socket to " << m_sIpAddress << ":" << m_uiPort << " " << ec.message();
  }
  // it helps to increase the buffer size to lessen packet loss
  //boost::asio::socket_base::receive_buffer_size option(udp_receiver_buffer_size_kb);
  //m_socket.set_option(option);
}

void TcpRtpConnection::writeCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
  boost::mutex::scoped_lock l(m_lock);

#ifdef LOG_TCP_INFO
  /* Fill tcp_info structure with data */
  uint32_t tcp_info_length = sizeof(tcp_info);
  struct tcp_info tcp_info;
  boost::asio::ip::tcp::socket::native_handle_type tcpSocket = m_socket.native_handle();
  if ( getsockopt( tcpSocket, SOL_TCP, TCP_INFO, (void *)&tcp_info, &tcp_info_length ) == 0 )
  {
    VLOG(2) << "[" << this << "] Wrote " << bytes_transferred << " TCP Stats: "
            << " sent: " << tcp_info.tcpi_last_data_sent
            << " recv: " << tcp_info.tcpi_last_data_recv
            << " cwnd: " << tcp_info.tcpi_snd_cwnd
            << " send_sst " << tcp_info.tcpi_snd_ssthresh
            << " recv_sst " << tcp_info.tcpi_rcv_ssthresh
            << " rtt: " << tcp_info.tcpi_rtt
            << " rttvar: " << tcp_info.tcpi_rttvar
            << " unack: " << tcp_info.tcpi_unacked
            << " sack: " << tcp_info.tcpi_sacked
            << " lost: " << tcp_info.tcpi_lost
            << " rtx: " << tcp_info.tcpi_retrans
            << " fackets: " << tcp_info.tcpi_fackets;
#if 0
    fprintf(statistics,"%.6f %u %u %u %u %u %u %u %u %u %u %u %u\n",
        time_to_seconds( &time_start, &time_now ),
        tcp_info.tcpi_last_data_sent,
        tcp_info.tcpi_last_data_recv,
        tcp_info.tcpi_snd_cwnd,
        tcp_info.tcpi_snd_ssthresh,
        tcp_info.tcpi_rcv_ssthresh,
        tcp_info.tcpi_rtt,
        tcp_info.tcpi_rttvar,
        tcp_info.tcpi_unacked,
        tcp_info.tcpi_sacked,
        tcp_info.tcpi_lost,
        tcp_info.tcpi_retrans,
        tcp_info.tcpi_fackets
         );
    if ( fflush(statistics) != 0 ) {
      fprintf(stderr, "Cannot flush buffers: %s\n", strerror(errno) );
    }
#endif
  }
#endif

  NetworkPackage_t package = m_vDeliveryQueue.front();
  m_vDeliveryQueue.pop_front();
  if (m_fnOnSend)
  {
    m_fnOnSend(ec, shared_from_this(), package.first, package.second);
  }

  if (!ec)
  {
      VLOG(10) << "[" << this << "][" << m_sIpAddress << ":" << m_uiPort << "] Sent " << bytes_transferred << " to " << package.second.getAddress() << ":" << package.second.getPort();

    // send next sample if one was queued in the meantime
    if ( !m_vDeliveryQueue.empty() )
    {
#if 0
      DLOG(INFO) << "Sending queued packet Queue size: " << m_vDeliveryQueue.size();
#endif
      NetworkPackage_t package = m_vDeliveryQueue.front();
      Buffer networkPacket = package.first;
      EndPoint ep = package.second;
      boost::asio::async_write( m_socket,
        boost::asio::buffer(networkPacket.data(), networkPacket.getSize()),
        boost::bind(&TcpRtpConnection::writeCompletionHandler,
        shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred)
        );
    }
  }
  else
  {
    LOG(WARNING) << "Send failed: " << ec.message();
  }
}

void TcpRtpConnection::timeoutHandler(const boost::system::error_code& ec)
{
  if (ec != boost::asio::error::operation_aborted)
  {
    // socket read timed out
    if (m_fnOnTimeout) m_fnOnTimeout(shared_from_this());
  }
}

void TcpRtpConnection::readCompletionHandler(const boost::system::error_code& ec, std::size_t bytes_received, uint32_t uiPacketSize)
{
// #define DEBUG_NTP
#ifdef DEBUG_NTP
  DLOG(INFO) << "NTP arrival time: " << convertNtpTimestampToPosixTime(getNTPTimeStamp());
#endif

#ifdef LOG_TCP_INFO
  /* Fill tcp_info structure with data */
  uint32_t tcp_info_length = sizeof(tcp_info);
  struct tcp_info tcp_info;
  boost::asio::ip::tcp::socket::native_handle_type tcpSocket = m_socket.native_handle();
  if ( getsockopt( tcpSocket, SOL_TCP, TCP_INFO, (void *)&tcp_info, &tcp_info_length ) == 0 )
  {
    VLOG(2) << "[" << this << "] Read " << bytes_received << " TCP Stats: "
            << " sent: " << tcp_info.tcpi_last_data_sent
            << " recv: " << tcp_info.tcpi_last_data_recv
            << " cwnd: " << tcp_info.tcpi_snd_cwnd
            << " send_sst " << tcp_info.tcpi_snd_ssthresh
            << " recv_sst " << tcp_info.tcpi_rcv_ssthresh
            << " rtt: " << tcp_info.tcpi_rtt
            << " rttvar: " << tcp_info.tcpi_rttvar
            << " unack: " << tcp_info.tcpi_unacked
            << " sack: " << tcp_info.tcpi_sacked
            << " lost: " << tcp_info.tcpi_lost
            << " rtx: " << tcp_info.tcpi_retrans
            << " fackets: " << tcp_info.tcpi_fackets;
#if 0
    fprintf(statistics,"%.6f %u %u %u %u %u %u %u %u %u %u %u %u\n",
        time_to_seconds( &time_start, &time_now ),
        tcp_info.tcpi_last_data_sent,
        tcp_info.tcpi_last_data_recv,
        tcp_info.tcpi_snd_cwnd,
        tcp_info.tcpi_snd_ssthresh,
        tcp_info.tcpi_rcv_ssthresh,
        tcp_info.tcpi_rtt,
        tcp_info.tcpi_rttvar,
        tcp_info.tcpi_unacked,
        tcp_info.tcpi_sacked,
        tcp_info.tcpi_lost,
        tcp_info.tcpi_retrans,
        tcp_info.tcpi_fackets
         );
    if ( fflush(statistics) != 0 ) {
      fprintf(stderr, "Cannot flush buffers: %s\n", strerror(errno) );
    }
#endif
  }
#endif

  NetworkPacket networkPacket(RtpTime::getNTPTimeStamp());

  if (!ec)
  {
    std::string sHostIp = m_socket.remote_endpoint().address().to_string();
    uint16_t uiHostPort = m_socket.remote_endpoint().port();
    VLOG(10) << "[" << this << "][" << m_sIpAddress << ":" << m_uiPort  << "] Received " << bytes_received << " from " << sHostIp << ":" << uiHostPort;

    EndPoint ep;
    ep.setAddress(sHostIp);
    ep.setPort(uiHostPort);

    uint32_t uiSBSize = m_streamBuffer.size();

    std::istream is(&m_streamBuffer);
    // read media sample
    NetworkPacket networkPacket(RtpTime::getNTPTimeStamp());
    networkPacket.setData(new uint8_t[uiPacketSize], uiPacketSize);
    is.read((char*)networkPacket.data(), uiPacketSize);
    if (m_fnOnRecv)
      m_fnOnRecv(ec, shared_from_this(), networkPacket, ep);
    // start next read
    do
    {
      uint32_t uiNextRead = 0;
      uiSBSize = m_streamBuffer.size();
      if (uiSBSize >= 2)
      {
        is.read((char*)m_sizeBuffer, TCP_RTP_HEADER_SIZE);
        uint16_t uiPacketSize = (m_sizeBuffer[0] << 8) | m_sizeBuffer[1];

        // TODO: should do this repeatedly until no more packets can be parsed
        if (m_streamBuffer.size() >= uiPacketSize )
        {
          // read media sample
          NetworkPacket networkPacket(RtpTime::getNTPTimeStamp());
          networkPacket.setData(new uint8_t[uiPacketSize], uiPacketSize);
          is.read((char*)networkPacket.data(), uiPacketSize);
          if (m_fnOnRecv)
            m_fnOnRecv(ec, shared_from_this(), networkPacket, ep);
        }
        else
        {
          uiNextRead = uiPacketSize - m_streamBuffer.size();
            boost::asio::async_read(m_socket,
            m_streamBuffer, boost::asio::transfer_at_least(uiNextRead),
            boost::bind(&TcpRtpConnection::readCompletionHandler, shared_from_this(),
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, uiPacketSize));
          break;
        }
      }
      else
      {
        // read next header
        uiNextRead = TCP_RTP_HEADER_SIZE - m_streamBuffer.size();
        boost::asio::async_read(m_socket,
          m_streamBuffer, boost::asio::transfer_at_least(uiNextRead),
          boost::bind(&TcpRtpConnection::readHeaderHandler, shared_from_this(),
          boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        break;
      }
    }while (true);

  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      LOG(WARNING) << "Receive failed: " << ec.message();
      if (m_fnOnRecv)
        m_fnOnRecv(ec, shared_from_this(), NetworkPacket(), EndPoint());
    }
  }

}

void TcpRtpConnection::readHeaderHandler(const boost::system::error_code& ec, std::size_t bytes_received)
{
  VLOG(15) << "Read header: bytes received: " << bytes_received;
// #define DEBUG_NTP
#ifdef DEBUG_NTP
  DLOG(INFO) << "NTP arrival time: " << convertNtpTimestampToPosixTime(getNTPTimeStamp());
#endif

#ifdef LOG_TCP_INFO
  /* Fill tcp_info structure with data */
  uint32_t tcp_info_length = sizeof(tcp_info);
  struct tcp_info tcp_info;
  boost::asio::ip::tcp::socket::native_handle_type tcpSocket = m_socket.native_handle();
  if ( getsockopt( tcpSocket, SOL_TCP, TCP_INFO, (void *)&tcp_info, &tcp_info_length ) == 0 )
  {
    VLOG(2) << "[" << this << "] Read " << bytes_received << " TCP Stats: "
            << " sent: " << tcp_info.tcpi_last_data_sent
            << " recv: " << tcp_info.tcpi_last_data_recv
            << " cwnd: " << tcp_info.tcpi_snd_cwnd
            << " send_sst " << tcp_info.tcpi_snd_ssthresh
            << " recv_sst " << tcp_info.tcpi_rcv_ssthresh
            << " rtt: " << tcp_info.tcpi_rtt
            << " rttvar: " << tcp_info.tcpi_rttvar
            << " unack: " << tcp_info.tcpi_unacked
            << " sack: " << tcp_info.tcpi_sacked
            << " lost: " << tcp_info.tcpi_lost
            << " rtx: " << tcp_info.tcpi_retrans
            << " fackets: " << tcp_info.tcpi_fackets;
#if 0
    fprintf(statistics,"%.6f %u %u %u %u %u %u %u %u %u %u %u %u\n",
        time_to_seconds( &time_start, &time_now ),
        tcp_info.tcpi_last_data_sent,
        tcp_info.tcpi_last_data_recv,
        tcp_info.tcpi_snd_cwnd,
        tcp_info.tcpi_snd_ssthresh,
        tcp_info.tcpi_rcv_ssthresh,
        tcp_info.tcpi_rtt,
        tcp_info.tcpi_rttvar,
        tcp_info.tcpi_unacked,
        tcp_info.tcpi_sacked,
        tcp_info.tcpi_lost,
        tcp_info.tcpi_retrans,
        tcp_info.tcpi_fackets
         );
    if ( fflush(statistics) != 0 ) {
      fprintf(stderr, "Cannot flush buffers: %s\n", strerror(errno) );
    }
#endif
  }
#endif

  EndPoint ep;
  if (!ec)
  {
    std::string sHostIp = m_socket.remote_endpoint().address().to_string();
    uint16_t uiHostPort = m_socket.remote_endpoint().port();
    VLOG(10) << "[" << this << "][" << m_sIpAddress << ":" << m_uiPort  << "] Received " << bytes_received << " from " << sHostIp << ":" << uiHostPort;

    EndPoint ep;
    ep.setAddress(sHostIp);
    ep.setPort(uiHostPort);

    // read size
    std::istream is(&m_streamBuffer);
    size_t size = m_streamBuffer.size();
    assert (size > 0);
    uint32_t uiNextRead = 0;
    do
    {
      if (m_streamBuffer.size() >= 2)
      {
        is.read((char*)m_sizeBuffer, TCP_RTP_HEADER_SIZE);
        uint16_t uiPacketSize = (m_sizeBuffer[0] << 8) | m_sizeBuffer[1];

        uint32_t uiSBSize = m_streamBuffer.size();
        // TODO: should do this repeatedly until no more packets can be parsed
        if (uiSBSize >= uiPacketSize )
        {
          // read media sample
          NetworkPacket networkPacket(RtpTime::getNTPTimeStamp());
          networkPacket.setData(new uint8_t[uiPacketSize], uiPacketSize);
          is.read((char*)networkPacket.data(), uiPacketSize);
          if (m_fnOnRecv)
            m_fnOnRecv(ec, shared_from_this(), networkPacket, ep);
        }
        else
        {
          uiNextRead = uiPacketSize - m_streamBuffer.size();
            boost::asio::async_read(m_socket,
            m_streamBuffer, boost::asio::transfer_at_least(uiNextRead),
            boost::bind(&TcpRtpConnection::readCompletionHandler, shared_from_this(),
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, uiPacketSize));
          break;
        }
      }
      else
      {
        // read next header
        uiNextRead = TCP_RTP_HEADER_SIZE - m_streamBuffer.size();
        boost::asio::async_read(m_socket,
          m_streamBuffer, boost::asio::transfer_at_least(uiNextRead),
          boost::bind(&TcpRtpConnection::readHeaderHandler, shared_from_this(),
          boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        break;
      }



    }while (true);


  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      LOG(WARNING) << "Receive failed: " << ec.message();
      if (m_fnOnRecv)
        m_fnOnRecv(ec, shared_from_this(), NetworkPacket(), EndPoint());
    }
  }

}

}
