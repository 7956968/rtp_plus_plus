#pragma once
#include <tuple> 
#include <rtp++/RtpJitterBufferV2.h>

/**
  * This class gives us access to the playout buffer internals
  */

namespace rtp_plus_plus
{
namespace test
{

class RtpJitterBufferV2Test
{
public:
  static void test()
  {
    test_insertion();
  }

  static void test_insertion()
  {
    // create test data
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();

    // TS SN PTS res late dup
    typedef std::tuple<uint32_t, uint32_t, boost::posix_time::ptime, bool, uint32_t, bool> PacketInfo_t;
    std::vector<PacketInfo_t> packet_info;
    // 0 First insertion
    packet_info.push_back( std::make_tuple(0,     5,  tNow + boost::posix_time::milliseconds(40), true, 0, false ) );
    // 1 New insertion
    packet_info.push_back( std::make_tuple(3600,  6,  tNow + boost::posix_time::milliseconds(80), true, 0, false ) );
    // 2 duplicate
    packet_info.push_back( std::make_tuple(3600,  6,  tNow + boost::posix_time::milliseconds(80), false, 0, true ) );
    // 3 Node update
    packet_info.push_back( std::make_tuple(3600,  7,  tNow + boost::posix_time::milliseconds(80), false, 0, false ) );
    // 4 Node update
    packet_info.push_back( std::make_tuple(3600,  8,  tNow + boost::posix_time::milliseconds(80), false, 0, false  ) );
    // 5 New insertion
    packet_info.push_back( std::make_tuple(7200,  10, tNow + boost::posix_time::milliseconds(120), true, 0, false  ) );
    // 6 Node update
    packet_info.push_back( std::make_tuple(7200,  11, tNow + boost::posix_time::milliseconds(120), false, 0, false ) );
    // 7 late
    packet_info.push_back( std::make_tuple(3600,  9,  tNow + boost::posix_time::milliseconds(80), false, 5, false ) );
    // 8 New insertion
    packet_info.push_back( std::make_tuple(10800, 13, tNow + boost::posix_time::milliseconds(160), true, 0, false ) );
    // 9 Node update
    packet_info.push_back( std::make_tuple(10800, 14, tNow + boost::posix_time::milliseconds(160), false, 0, false ) );
    // 10 old update
    packet_info.push_back( std::make_tuple(7200,  12, tNow + boost::posix_time::milliseconds(120), false, 0, false ) );
    // 11 duplicate
    packet_info.push_back( std::make_tuple(10800, 14, tNow + boost::posix_time::milliseconds(160), false, 0, true  ) );
    // 12 New insertion
    packet_info.push_back( std::make_tuple(14400, 15, tNow + boost::posix_time::milliseconds(200), true, 0, false ) );
    // 13 New insertion
    packet_info.push_back( std::make_tuple(21600, 17, tNow + boost::posix_time::milliseconds(280), true, 0, false ) );
    // 14 Node update
    packet_info.push_back( std::make_tuple(21600, 18, tNow + boost::posix_time::milliseconds(280), false, 0, false ) );
    // 15 Node update
    packet_info.push_back( std::make_tuple(21600, 19, tNow + boost::posix_time::milliseconds(280), false, 0, false ) );
    // 16 Old insertion
    packet_info.push_back( std::make_tuple(18000, 16, tNow + boost::posix_time::milliseconds(240), true, 0, false ) );
    // 17 super late with new TS (artifical sleep
    packet_info.push_back( std::make_tuple(25200, 20, tNow + boost::posix_time::milliseconds(320), false, 5, false ) );

    // generate RTP packets
    std::vector<RtpPacket> rtpPackets;
    for (size_t i = 0; i < packet_info.size(); ++i)
    {
      RtpPacket packet;
      // must set arrival time since jitter buffer uses it as reference
      packet.setArrivalTime(tNow);
      packet.getHeader().setRtpTimestamp(std::get<0>(packet_info[i]));
      packet.setExtendedSequenceNumber(std::get<1>(packet_info[i]));
      rtpPackets.push_back(packet);
    }

    RtpJitterBufferV2 buffer(1000);

    for (size_t i = 0; i < packet_info.size(); ++i)
    {
      boost::posix_time::ptime tPlayout;
      bool bRtcpSync = false;
      uint32_t uiLate = 0;
      bool bDup = false;
      bool bRes = buffer.addRtpPacket(rtpPackets[i], std::get<2>(packet_info[i]), bRtcpSync, tPlayout, uiLate, bDup);
      BOOST_CHECK_EQUAL( bRes, std::get<3>(packet_info[i]) );
      BOOST_CHECK_EQUAL( uiLate > 0, std::get<4>(packet_info[i])>0 );
      BOOST_CHECK_EQUAL( bDup, std::get<5>(packet_info[i]) );

      switch (i)
      {
        case 6:
        {
          // read first group causing 7 to be late
          const RtpPacketGroup group = buffer.getNextPlayoutBufferNode();
          BOOST_CHECK_EQUAL( group.getSize(), 1 );
          const RtpPacketGroup group2 = buffer.getNextPlayoutBufferNode();
          BOOST_CHECK_EQUAL( group2.getSize(), 3 );
          break;
        }
        case 16:
        {
          // sleep to cause last new packet to be late
#ifdef WIN32
          Sleep(2000);
#else
          sleep(2);
#endif
        }
      }
    }
  }
};

}
}
