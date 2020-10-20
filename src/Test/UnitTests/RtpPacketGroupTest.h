#pragma once
#include <map>
#include <rtp++/RtpPacketGroup.h>

namespace rtp_plus_plus
{
namespace test
{
class RtpPacketGroupTest
{
public:
  static void test()
  {
    test_ordering();
  }

  /// test sequence number loss and reordering patterns
  static void test_ordering()
  {
    test_ordering1();
    test_ordering2();
    test_ordering3();
    test_ordering4();
    test_ordering5();
    test_ordering6();
    test_ordering7();
    test_ordering8();
    test_ordering9();
    test_ordering10();
    test_ordering11();
    test_ordering12();
  }

  static void test_ordering1()
  {
    // test normal operation
    std::vector<uint32_t> vSequenceNumbers;
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(2);
    vSequenceNumbers.push_back(3);
    vSequenceNumbers.push_back(4);
    vSequenceNumbers.push_back(5);
    vSequenceNumbers.push_back(6);

    test_values(vSequenceNumbers, vSequenceNumbers);
  }

  static void test_ordering2()
  {
    // test loss
    std::vector<uint32_t> vSequenceNumbers;
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(2);
    vSequenceNumbers.push_back(4);
    vSequenceNumbers.push_back(5);
    vSequenceNumbers.push_back(6);

    test_values(vSequenceNumbers, vSequenceNumbers);
  }

  static void test_ordering3()
  {
    // test loss
    std::vector<uint32_t> vSequenceNumbers;
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(2);
    vSequenceNumbers.push_back(5);
    vSequenceNumbers.push_back(6);

    test_values(vSequenceNumbers, vSequenceNumbers);
  }

  static void test_ordering4()
  {
    // test loss
    std::vector<uint32_t> vSequenceNumbers;
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(2);
    vSequenceNumbers.push_back(4);
    vSequenceNumbers.push_back(6);

    test_values(vSequenceNumbers, vSequenceNumbers);
  }

  static void test_ordering5()
  {
    // test duplicate
    std::vector<uint32_t> vSequenceNumbers;
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(2);
    vSequenceNumbers.push_back(2);
    vSequenceNumbers.push_back(3);
    vSequenceNumbers.push_back(4);
    vSequenceNumbers.push_back(5);
    vSequenceNumbers.push_back(6);

    std::vector<uint32_t> vSequenceNumbersEx;
    vSequenceNumbersEx.push_back(1);
    vSequenceNumbersEx.push_back(2);
    vSequenceNumbersEx.push_back(3);
    vSequenceNumbersEx.push_back(4);
    vSequenceNumbersEx.push_back(5);
    vSequenceNumbersEx.push_back(6);

    test_values(vSequenceNumbers, vSequenceNumbersEx);
  }

  static void test_ordering6()
  {
    std::vector<uint32_t> vSequenceNumbers;
    // test wrap around
    vSequenceNumbers.push_back(UINT_MAX - 3);
    vSequenceNumbers.push_back(UINT_MAX - 2);
    vSequenceNumbers.push_back(UINT_MAX - 1);
    vSequenceNumbers.push_back(UINT_MAX - 0);
    vSequenceNumbers.push_back(0);
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(2);
    vSequenceNumbers.push_back(3);

    test_values(vSequenceNumbers, vSequenceNumbers);
  }

  static void test_ordering7()
  {
    std::vector<uint32_t> vSequenceNumbers;
    // test wrap around
    // single loss + re-order
    vSequenceNumbers.push_back(UINT_MAX - 3);
    vSequenceNumbers.push_back(UINT_MAX - 2);
    vSequenceNumbers.push_back(UINT_MAX - 1);
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(0);
    vSequenceNumbers.push_back(3);

    std::vector<uint32_t> vSequenceNumbersEx;
    vSequenceNumbersEx.push_back(UINT_MAX - 3);
    vSequenceNumbersEx.push_back(UINT_MAX - 2);
    vSequenceNumbersEx.push_back(UINT_MAX - 1);
    vSequenceNumbersEx.push_back(0);
    vSequenceNumbersEx.push_back(1);
    vSequenceNumbersEx.push_back(3);

    test_values(vSequenceNumbers, vSequenceNumbersEx);
  }

  static void test_ordering8()
  {
    std::vector<uint32_t> vSequenceNumbers;
    // test wrap around
    // single loss + re-order
    vSequenceNumbers.push_back(UINT_MAX - 3);
    vSequenceNumbers.push_back(UINT_MAX - 2);
    vSequenceNumbers.push_back(UINT_MAX - 1);
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(2);
    vSequenceNumbers.push_back(3);
    test_values(vSequenceNumbers, vSequenceNumbers);
  }

  static void test_ordering9()
  {
    std::vector<uint32_t> vSequenceNumbers;
    // test wrap around
    vSequenceNumbers.push_back(UINT_MAX - 3);
    vSequenceNumbers.push_back(UINT_MAX - 2);
    vSequenceNumbers.push_back(0);
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(2);
    vSequenceNumbers.push_back(3);
    test_values(vSequenceNumbers, vSequenceNumbers);
  }

  static void test_ordering10()
  {
    std::vector<uint32_t> vSequenceNumbers;
    // test wrap around
    vSequenceNumbers.push_back(UINT_MAX - 3);
    vSequenceNumbers.push_back(UINT_MAX - 2);
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(2);
    vSequenceNumbers.push_back(3);
    test_values(vSequenceNumbers, vSequenceNumbers);
  }

  static void test_ordering11()
  {
    std::vector<uint32_t> vSequenceNumbers;
    // test wrap around
    vSequenceNumbers.push_back(UINT_MAX - 3);
    vSequenceNumbers.push_back(UINT_MAX - 2);
    vSequenceNumbers.push_back(UINT_MAX - 1);
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(2);
    vSequenceNumbers.push_back(4);
    test_values(vSequenceNumbers, vSequenceNumbers);
  }

  static void test_ordering12()
  {
    std::vector<uint32_t> vSequenceNumbers;
    // test wrap around
    vSequenceNumbers.push_back(UINT_MAX - 3);
    vSequenceNumbers.push_back(UINT_MAX - 2);
    vSequenceNumbers.push_back(1);
    vSequenceNumbers.push_back(6);
    vSequenceNumbers.push_back(3);

    std::vector<uint32_t> vSequenceNumbersEx;
    // test wrap around
    vSequenceNumbersEx.push_back(UINT_MAX - 3);
    vSequenceNumbersEx.push_back(UINT_MAX - 2);
    vSequenceNumbersEx.push_back(1);
    vSequenceNumbersEx.push_back(3);
    vSequenceNumbersEx.push_back(6);

    test_values(vSequenceNumbers, vSequenceNumbersEx);
  }

private:
  static void test_values(const std::vector<uint32_t>& vSequenceNumbers,
                          const std::vector<uint32_t>& vExpectedSequenceNumbers)
  {
    std::map<uint32_t, RtpPacket> mRtpPackets = generateRtpPacketMap(vSequenceNumbers);
    auto it = vSequenceNumbers.begin();
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    RtpPacketGroup group1(mRtpPackets[*it], tNow, true, tNow + boost::posix_time::seconds(1));
    for (auto it2 = it + 1; it2 != vSequenceNumbers.end(); ++it2)
    {
      group1.insert(mRtpPackets[*it2]);
    }
    const std::list<RtpPacket>& packets = group1.getRtpPackets();
    BOOST_CHECK_EQUAL(packets.size(), vExpectedSequenceNumbers.size());
    auto itRtpList = packets.begin();
    auto itExpected = vExpectedSequenceNumbers.begin();
    for ( ; itRtpList != packets.end(); ++itRtpList, ++itExpected)
    {
      BOOST_CHECK_EQUAL(itRtpList->getExtendedSequenceNumber(), *itExpected);
    }
  }

  static std::map<uint32_t, RtpPacket> generateRtpPacketMap(const std::vector<uint32_t>& vSequenceNumbers)
  {
    std::map<uint32_t, RtpPacket> mPackets;
    for (uint32_t uiSN : vSequenceNumbers)
    {
      RtpPacket packet;
      packet.setExtendedSequenceNumber(uiSN);
      mPackets[uiSN] = packet;
    }
    return mPackets;
  }

};

}
}
