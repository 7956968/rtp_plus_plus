#pragma once
#include <cpputil/RunningAverageQueue.h>
#include <cpputil/IBitStream.h>
#include <cpputil/OBitStream.h>

namespace rtp_plus_plus
{
namespace test
{

BOOST_AUTO_TEST_SUITE(CppUtilTest)

BOOST_AUTO_TEST_CASE( tc_test_BitStreams )
{
  uint32_t uiVal = 123456;

  OBitStream ob(sizeof(uint32_t));
  ob.write(uiVal, sizeof(uint32_t) * 8);

  IBitStream ib(ob.data());
  uint32_t uiVal2 = 0;
  assert (ib.read(uiVal2, sizeof(uint32_t)* 8));

  BOOST_CHECK_EQUAL(uiVal, uiVal2);
}

BOOST_AUTO_TEST_CASE( tc_test_CppUtil )
{
  uint32_t max = 20;
  RunningAverageQueue<uint32_t> queue1(max);
  RunningAverageQueue<uint32_t, double> queue2(max);

  uint32_t uiCount = 10;
  uint32_t uiTotal = 0;
  for (size_t i = 0; i < uiCount; ++i)
  {
    uiTotal += i;
    queue1.insert(i);
    queue2.insert(i);
  }

  BOOST_CHECK_EQUAL(queue1.getAverage(), uiTotal/uiCount);
  BOOST_CHECK_EQUAL(queue2.getAverage(), uiTotal/static_cast<double>(uiCount));
}

BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
