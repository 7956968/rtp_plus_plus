#pragma once

#include <rtp++/LossEstimator.h>

namespace rtp_plus_plus
{
namespace test
{

BOOST_AUTO_TEST_SUITE(LossEstimation)
BOOST_AUTO_TEST_CASE( tc_test_lossEstimation)
{
  // sequence number test vectors
  std::vector<uint16_t> vSequenceNumbers;
  std::vector<uint16_t> vLost;
  LossEstimator estimator;

  // ########################################################################################
  // No loss
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(2);
  vSequenceNumbers.push_back(3);
  vSequenceNumbers.push_back(4);
  vSequenceNumbers.push_back(5);
  vSequenceNumbers.push_back(6);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 0);
  vSequenceNumbers.clear();
  estimator.clear();

  // ########################################################################################
  // single loss
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(2);
  vSequenceNumbers.push_back(4);
  vSequenceNumbers.push_back(5);
  vSequenceNumbers.push_back(6);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 1);
  BOOST_CHECK_EQUAL(vLost[0], 3);
  vSequenceNumbers.clear();
  estimator.clear();

  // ########################################################################################
  // multiple burst losses
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(2);
  vSequenceNumbers.push_back(5);
  vSequenceNumbers.push_back(6);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 2);
  BOOST_CHECK_EQUAL(vLost[0], 3);
  BOOST_CHECK_EQUAL(vLost[1], 4);
  vSequenceNumbers.clear();
  estimator.clear();

  // ########################################################################################
  // multiple interleaved losses
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(2);
  vSequenceNumbers.push_back(4);
  vSequenceNumbers.push_back(6);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 2);
  BOOST_CHECK_EQUAL(vLost[0], 3);
  BOOST_CHECK_EQUAL(vLost[1], 5);
  vSequenceNumbers.clear();
  estimator.clear();

  // wrap around
  // ########################################################################################
  // No loss
  vSequenceNumbers.push_back(USHRT_MAX - 3);
  vSequenceNumbers.push_back(USHRT_MAX - 2);
  vSequenceNumbers.push_back(USHRT_MAX - 1);
  vSequenceNumbers.push_back(USHRT_MAX - 0);
  vSequenceNumbers.push_back(0);
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(2);
  vSequenceNumbers.push_back(3);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 0);
  vSequenceNumbers.clear();
  estimator.clear();

  // single loss + re-order
  vSequenceNumbers.push_back(USHRT_MAX - 3);
  vSequenceNumbers.push_back(USHRT_MAX - 2);
  vSequenceNumbers.push_back(USHRT_MAX - 1);
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(0);
  vSequenceNumbers.push_back(3);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 2);
  BOOST_CHECK_EQUAL(vLost[1], 2);
  BOOST_CHECK_EQUAL(vLost[0], USHRT_MAX);
  vSequenceNumbers.clear();
  estimator.clear();

  // ########################################################################################
  // single loss
  vSequenceNumbers.push_back(USHRT_MAX - 3);
  vSequenceNumbers.push_back(USHRT_MAX - 2);
  vSequenceNumbers.push_back(USHRT_MAX - 1);
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(2);
  vSequenceNumbers.push_back(3);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 2);
  BOOST_CHECK_EQUAL(vLost[1], 0);
  BOOST_CHECK_EQUAL(vLost[0], USHRT_MAX);
  vSequenceNumbers.clear();
  estimator.clear();

  // single loss2
  vSequenceNumbers.push_back(USHRT_MAX - 3);
  vSequenceNumbers.push_back(USHRT_MAX - 2);
  vSequenceNumbers.push_back(0);
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(2);
  vSequenceNumbers.push_back(3);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 2);
  BOOST_CHECK_EQUAL(vLost[0], USHRT_MAX - 1);
  BOOST_CHECK_EQUAL(vLost[1], USHRT_MAX);
  vSequenceNumbers.clear();
  estimator.clear();

  // ########################################################################################
  // multiple burst losses
  vSequenceNumbers.push_back(USHRT_MAX - 3);
  vSequenceNumbers.push_back(USHRT_MAX - 2);
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(2);
  vSequenceNumbers.push_back(3);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 3);
  BOOST_CHECK_EQUAL(vLost[2], 0);
  BOOST_CHECK_EQUAL(vLost[0], USHRT_MAX - 1);
  BOOST_CHECK_EQUAL(vLost[1], USHRT_MAX);
  vSequenceNumbers.clear();
  estimator.clear();

  // ########################################################################################
  // multiple interleaved losses
  vSequenceNumbers.push_back(USHRT_MAX - 3);
  vSequenceNumbers.push_back(USHRT_MAX - 2);
  vSequenceNumbers.push_back(USHRT_MAX - 1);
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(2);
  vSequenceNumbers.push_back(4);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 3);
  BOOST_CHECK_EQUAL(vLost[1], 0);
  BOOST_CHECK_EQUAL(vLost[2], 3);
  BOOST_CHECK_EQUAL(vLost[0], USHRT_MAX);
  vSequenceNumbers.clear();
  estimator.clear();

  // Reordering
  // ########################################################################################
  // multiple interleaved losses
  vSequenceNumbers.push_back(USHRT_MAX - 3);
  vSequenceNumbers.push_back(USHRT_MAX - 2);
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(6);
  vSequenceNumbers.push_back(3);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 6);
  BOOST_CHECK_EQUAL(vLost[2], 0);
  BOOST_CHECK_EQUAL(vLost[3], 2);
  BOOST_CHECK_EQUAL(vLost[4], 4);
  BOOST_CHECK_EQUAL(vLost[5], 5);
  BOOST_CHECK_EQUAL(vLost[0], USHRT_MAX - 1);
  BOOST_CHECK_EQUAL(vLost[1], USHRT_MAX);
  vSequenceNumbers.clear();
  estimator.clear();
}

BOOST_AUTO_TEST_CASE( tc_test_lossEstimationBehaviour)
{
  // sequence number test vectors
  std::vector<uint16_t> vSequenceNumbers;
  std::vector<uint16_t> vLost;
  LossEstimator estimator;

  vSequenceNumbers.push_back(USHRT_MAX - 3);
  vSequenceNumbers.push_back(USHRT_MAX - 2);
  vSequenceNumbers.push_back(1);
  vSequenceNumbers.push_back(6);
  vSequenceNumbers.push_back(3);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 6);
  BOOST_CHECK_EQUAL(vLost[2], 0);
  BOOST_CHECK_EQUAL(vLost[3], 2);
  BOOST_CHECK_EQUAL(vLost[4], 4);
  BOOST_CHECK_EQUAL(vLost[5], 5);
  BOOST_CHECK_EQUAL(vLost[0], USHRT_MAX - 1);
  BOOST_CHECK_EQUAL(vLost[1], USHRT_MAX);

  vSequenceNumbers.clear();
  vSequenceNumbers.push_back(7);
  vSequenceNumbers.push_back(8);
  vSequenceNumbers.push_back(10);
  vSequenceNumbers.push_back(11);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 1);
  BOOST_CHECK_EQUAL(vLost[0], 9);

  vSequenceNumbers.clear();
  vSequenceNumbers.push_back(12);
  vSequenceNumbers.push_back(13);
  vSequenceNumbers.push_back(14);
  vSequenceNumbers.push_back(15);

  std::for_each(vSequenceNumbers.begin(), vSequenceNumbers.end(), [&estimator](uint16_t uiSN)
  {
    estimator.addSequenceNumber(uiSN);
  });
  vLost = estimator.getEstimatedLostSequenceNumbers();
  BOOST_CHECK_EQUAL(vLost.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
