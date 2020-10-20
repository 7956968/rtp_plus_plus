#pragma once
#include <rtp++/RtpJitterBuffer.h>

/**
  * This class gives us access to the playout buffer internals
  */

namespace rtp_plus_plus
{
namespace test
{

class RtpJitterBufferTest
{
public:
  RtpJitterBufferTest(RtpJitterBuffer& playoutBuffer)
    :m_playoutBuffer(playoutBuffer)
  {
  }

  std::list<RtpPacketGroup> getNodeList() { return m_playoutBuffer.m_playoutBufferList; }

  uint32_t getDuplicateSequenceNumberCount() const { return m_playoutBuffer.m_uiTotalDuplicates; }

  static void test()
  {
    test_RtpPlayoutBuffer();
  }

  static void test_RtpPlayoutBuffer()
  {
    RtpJitterBuffer playoutBuffer;
    RtpJitterBufferTest testBuffer(playoutBuffer);

    typedef std::pair<uint32_t, uint32_t> TS_XSN_PAIR_t;
    std::vector<TS_XSN_PAIR_t> packet_info;
    packet_info.push_back( std::make_pair(UINT_MAX - 30, 5) );
    packet_info.push_back( std::make_pair(UINT_MAX - 20, 6) );
    // duplicate SN
    packet_info.push_back( std::make_pair(UINT_MAX - 20, 6) );
    // node update
    packet_info.push_back( std::make_pair(UINT_MAX - 20, 7) );
    packet_info.push_back( std::make_pair(UINT_MAX - 10, 8) );
    packet_info.push_back( std::make_pair(UINT_MAX,      9) );
    // wrap around
    packet_info.push_back( std::make_pair(20,            10) );
    // re-ordering
    packet_info.push_back( std::make_pair(10,            11) );
    // duplicate SN
    packet_info.push_back( std::make_pair(10,            11) );

    // generate RTP packets
    std::vector<RtpPacket> rtpPackets;
    for (size_t i = 0; i < packet_info.size(); ++i)
    {
      RtpPacket packet;
      packet.getHeader().setRtpTimestamp(packet_info[i].first);
      packet.setExtendedSequenceNumber(packet_info[i].second);

      rtpPackets.push_back(packet);
    }

    // add packets into playout buffer
    boost::posix_time::ptime tPresentationDummy;
    boost::posix_time::ptime tPlayoutDummy;
    std::for_each(rtpPackets.begin(), rtpPackets.end(), [&playoutBuffer, &tPresentationDummy, &tPlayoutDummy](const RtpPacket& packet)
    {
      bool bRtcpSync = false;
      bool bDup = false;
      uint32_t uiLate = 0;
      playoutBuffer.addRtpPacket(packet, tPresentationDummy, bRtcpSync, tPlayoutDummy, uiLate, bDup);
    });

    // check correctness

    std::list<RtpPacketGroup> nodes = testBuffer.getNodeList();

    // test order
    std::vector<uint32_t> expected_timestamps;
    expected_timestamps.push_back(UINT_MAX - 30);
    expected_timestamps.push_back(UINT_MAX - 20);
    expected_timestamps.push_back(UINT_MAX - 10);
    expected_timestamps.push_back(UINT_MAX );
    expected_timestamps.push_back(10);
    expected_timestamps.push_back(20);

    auto nodes_it = nodes.begin();
    for (size_t i = 0; i < expected_timestamps.size(); ++i)
    {
      BOOST_CHECK_EQUAL( expected_timestamps[i], nodes_it->getRtpTimestamp() );
      ++nodes_it;
    }

    // test number of nodes
    BOOST_CHECK_EQUAL( 6, nodes.size());

    // test node update
    auto it = nodes.begin();
    // go to second node which should have 2 elements
    ++it;
    std::list<RtpPacket> groupedRtpPackets = it->getRtpPackets();
    BOOST_CHECK_EQUAL( 2, groupedRtpPackets.size());
    // check order of coded data
    auto data_it = groupedRtpPackets.begin();
    BOOST_CHECK_EQUAL( 6, data_it->getSequenceNumber());
    ++data_it;
    BOOST_CHECK_EQUAL( 7, data_it->getSequenceNumber());

    // test duplicate sequence number
    BOOST_CHECK_EQUAL( 2, testBuffer.getDuplicateSequenceNumberCount() );
  }

private:
  RtpJitterBuffer& m_playoutBuffer;
};

}
}
