#pragma once
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <rtp++/RtpPacket.h>
#include <rtp++/network/EndPoint.h>
#include <rtp++/rfc4571/PassiveTcpRtpNetworkInterface.h>

namespace rtp_plus_plus
{
namespace test
{

class Rfc4571Test
{
public:

  static void test()
  {
//    Rfc4571Test::test_TcpRtpSockets();
  }

  static void handleRtp(const RtpPacket& rtpPacket, const EndPoint& ep)
  {
    LOG(INFO) << "Read RTP packet from socket: " <<  rtpPacket.getSequenceNumber();
  }
  static void handleRtcp(const CompoundRtcpPacket& rtcpPacket, const EndPoint& ep)
  {
    LOG(INFO) << "Read RTCP packet from socket: " <<  rtcpPacket.size();
  }
  static void handleRtpOut(const RtpPacket& rtpPacket, const EndPoint& ep)
  {
    LOG(INFO) << "Sent RTP packet from socket: " <<  rtpPacket.getSequenceNumber();
  }
  static void handleRtcpOut(const CompoundRtcpPacket& rtcpPacket, const EndPoint& ep)
  {
    LOG(INFO) << "Sent RTCP packet from socket: " <<  rtcpPacket.size();
  }

  static void sendPacket(rfc4571::TcpRtpNetworkInterface* pActive)
  {
    EndPoint rtpEp("127.0.0.1", 49170);
    EndPoint rtcpEp("127.0.0.1", 49171);

    LOG(INFO) << "Sending packet";
    RtpPacket rtpPacket;
    Buffer payload(new uint8_t[1000], 1000);
    rtpPacket.setPayload(payload);
    pActive->send(rtpPacket, rtpEp);

    RtpPacket rtpPacket2;
    rtpPacket2.getHeader().setSequenceNumber(1);
    Buffer payload2(new uint8_t[1000], 1000);
    rtpPacket2.setPayload(payload2);
    pActive->send(rtpPacket2, rtpEp);

    CompoundRtcpPacket rtcp;
    rtcp.push_back(rfc3550::RtcpSr::create());
    pActive->send(rtcp, rtcpEp);
  }

  static void closeConnections(rfc4571::PassiveTcpRtpNetworkInterface* pListener, rfc4571::TcpRtpNetworkInterface* pActive)
  {
    LOG(INFO) << "Shutting down passive TCP RTP network interface";
    pListener->shutdown();
    pActive->shutdown();
  }

  /// test sequence number loss and reordering patterns
  static void test_TcpRtpSockets()
  {
    boost::asio::io_service ioService;
    EndPoint rtpEp("127.0.0.1", 49170);
    EndPoint rtcpEp("127.0.0.1", 49171);

    // server connection
    rfc4571::PassiveTcpRtpNetworkInterface* pListener = new rfc4571::PassiveTcpRtpNetworkInterface(ioService, rtpEp, rtcpEp);
    pListener->setIncomingRtpHandler(boost::bind(&Rfc4571Test::handleRtp, _1, _2));
    pListener->setIncomingRtcpHandler(boost::bind(&Rfc4571Test::handleRtcp, _1, _2));
    pListener->setOutgoingRtpHandler(boost::bind(&Rfc4571Test::handleRtpOut, _1, _2));
    pListener->setOutgoingRtcpHandler(boost::bind(&Rfc4571Test::handleRtcpOut, _1, _2));
    // client connection
    rfc4571::TcpRtpNetworkInterface* pActive = new rfc4571::TcpRtpNetworkInterface(ioService, rtpEp, rtcpEp);
    pActive->setIncomingRtpHandler(boost::bind(&Rfc4571Test::handleRtp, _1, _2));
    pActive->setIncomingRtcpHandler(boost::bind(&Rfc4571Test::handleRtcp, _1, _2));
    pActive->setOutgoingRtpHandler(boost::bind(&Rfc4571Test::handleRtpOut, _1, _2));
    pActive->setOutgoingRtcpHandler(boost::bind(&Rfc4571Test::handleRtcpOut, _1, _2));

    boost::asio::deadline_timer timer1(ioService);
    boost::asio::deadline_timer timer2(ioService);
    timer1.expires_from_now(boost::posix_time::seconds(1));
    timer1.async_wait(boost::bind(&Rfc4571Test::sendPacket, pActive));
    timer2.expires_from_now(boost::posix_time::seconds(5));
    timer2.async_wait(boost::bind(&Rfc4571Test::closeConnections, pListener, pActive));

    ioService.run();
    BOOST_CHECK_EQUAL(0, 0);
    delete pActive;
    delete pListener;
  }

};

}
}
