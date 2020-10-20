#pragma once
#include <rtp++/rto/AR2Predictor.h>
#include <rtp++/rto/MovingAveragePredictor.h>
#include <rtp++/rto/NormalDistribution.h>
#include <rtp++/rto/SimpleRtoEstimator.h>
#include <rtp++/util/TracesUtil.h>

#define RTO_TEST_LOG_LEVEL 10

namespace rtp_plus_plus
{
namespace test
{

uint32_t g_uiLost = 0;
uint32_t g_uiFalsePositive = 0;

class RtoTest
{
public:
  static void test()
  {
    RtoTest::test_Prediction();
    RtoTest::test_SimplePacketLossDetection();
    RtoTest::test_SimplePacketLossDetectionRollover();
    RtoTest::test_SimplePacketLossDetectionRolloverWithLoss();
  }

  static void onLoss(uint32_t uiSN)
  {
    ++g_uiLost;
    VLOG(RTO_TEST_LOG_LEVEL) << "Packet loss detected: " << uiSN;
  }

  static bool onFalsePositive(uint32_t uiSN)
  {
    --g_uiLost;
    ++g_uiFalsePositive;
    VLOG(RTO_TEST_LOG_LEVEL) << "False positive detected: " << uiSN;
    return true;
  }

  static void test_SimplePacketLossDetection()
  {
    VLOG(RTO_TEST_LOG_LEVEL) << "test_SimplePacketLossDetection";
    rto::SimplePacketLossDetection detection;
    detection.setLostCallback(boost::bind(&RtoTest::onLoss, _1));
    detection.setFalsePositiveCallback(boost::bind(&RtoTest::onFalsePositive, _1));

    std::vector<uint16_t> vSequenceNumbers = {3000, 3001, 3003, 3005, 3004, 3007, 3006, 3009, 3008, 3011, 3010};
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    detection.onPacketArrival(tNow, vSequenceNumbers[0]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(40), vSequenceNumbers[1]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(80), vSequenceNumbers[2]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(120), vSequenceNumbers[3]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(160), vSequenceNumbers[4]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(200), vSequenceNumbers[5]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(240), vSequenceNumbers[6]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(280), vSequenceNumbers[7]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(320), vSequenceNumbers[8]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(360), vSequenceNumbers[9]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(400), vSequenceNumbers[10]);
    BOOST_CHECK_EQUAL( g_uiLost, 1);
    BOOST_CHECK_EQUAL( g_uiFalsePositive, 4);
    detection.stop();
  }

  static void test_SimplePacketLossDetectionRollover()
  {
    VLOG(RTO_TEST_LOG_LEVEL) << "test_SimplePacketLossDetectionRollover";
    g_uiLost = 0;
    g_uiFalsePositive = 0;
    rto::SimplePacketLossDetection detection;
    detection.setLostCallback(boost::bind(&RtoTest::onLoss, _1));
    detection.setFalsePositiveCallback(boost::bind(&RtoTest::onFalsePositive, _1));

    std::vector<uint32_t> vSequenceNumbers = {65530, 65531, 65532, 65533, 65534, 65535, 65536, 65537, 65538, 65539};
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    VLOG(RTO_TEST_LOG_LEVEL) << "Adding SN " << vSequenceNumbers[0];
    detection.onPacketArrival(tNow, vSequenceNumbers[0]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(40), vSequenceNumbers[1]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(80), vSequenceNumbers[2]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(120), vSequenceNumbers[3]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(160), vSequenceNumbers[4]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(200), vSequenceNumbers[5]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(240), vSequenceNumbers[6]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(280), vSequenceNumbers[7]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(320), vSequenceNumbers[8]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(360), vSequenceNumbers[9]);
    BOOST_CHECK_EQUAL( g_uiLost, 0);
    BOOST_CHECK_EQUAL( g_uiFalsePositive, 0);
    detection.stop();
  }

  static void test_SimplePacketLossDetectionRolloverWithLoss()
  {
    VLOG(RTO_TEST_LOG_LEVEL) << "test_SimplePacketLossDetectionRolloverWithLoss";
    g_uiLost = 0;
    g_uiFalsePositive = 0;
    rto::SimplePacketLossDetection detection;
    detection.setLostCallback(boost::bind(&RtoTest::onLoss, _1));
    detection.setFalsePositiveCallback(boost::bind(&RtoTest::onFalsePositive, _1));

    std::vector<uint32_t> vSequenceNumbers = {65530, 65531, 65532, 65533, 65534, 65535, 65537, 65538, 65539};
    boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
    detection.onPacketArrival(tNow, vSequenceNumbers[0]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(40), vSequenceNumbers[1]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(80), vSequenceNumbers[2]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(120), vSequenceNumbers[3]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(160), vSequenceNumbers[4]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(200), vSequenceNumbers[5]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(240), vSequenceNumbers[6]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(280), vSequenceNumbers[7]);
    detection.onPacketArrival(tNow + boost::posix_time::milliseconds(320), vSequenceNumbers[8]);
    BOOST_CHECK_EQUAL( g_uiLost, 1);
    BOOST_CHECK_EQUAL( g_uiFalsePositive, 0);
    detection.stop();
  }

  template <typename T, typename R>
  static double testPredictor(rto::PredictorBase<T, R>* pPredictor, std::vector<T>& vDeltas)
  {
    for (size_t i = 0; i < vDeltas.size(); ++i)
    {
      pPredictor->insert(vDeltas[i]);
    }

    //      R uiSumErrorSquared = std::inner_product(vErrors.begin(), vErrors.end(), vErrors.begin(), 0.0);
    //      return uiSumErrorSquared/vErrors.size();
    return pPredictor->getErrorStandardDeviation();
  }

  static void test_AR2()
  {
    std::vector<uint32_t> vDeltas;
    vDeltas.push_back(39);
    for (int i = 0; i < 1000; ++i)
      vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    vDeltas.push_back(40);
    rto::AR2Predictor<uint32_t, int32_t>* pAR2Predictor = new rto::AR2Predictor<uint32_t, int32_t>();
    double uiErrorSumAR2 = testPredictor(pAR2Predictor, vDeltas);
    VLOG(RTO_TEST_LOG_LEVEL) << "AR2 - sum of error squared: " << uiErrorSumAR2;
    delete pAR2Predictor;
  }

  static void test_Critz()
  {
    double val1 = rto::NormalDistribution::getZScore(0.95);
    double val2 = rto::NormalDistribution::getZScore(0.001);
    int t = 0;
  }

  static void test_Prediction()
  {
    test_Critz();
    testInnerProduct();
    test_AR2();
    return;

    // load trace file
    std::vector<uint32_t> vDeltas;
    bool bSuccess = TracesUtil::loadInterarrivalDataFile("interarrival.dat", vDeltas);
    BOOST_CHECK_EQUAL(bSuccess, true);

    std::vector<uint64_t> vDeltasMs;
    vDeltasMs.resize(vDeltas.size());
    std::transform(vDeltas.begin(), vDeltas.end(), vDeltasMs.begin(),
                   [](uint32_t uiVal)
    {
      return static_cast<uint64_t>(uiVal);
    });


    // moving average
    const uint32_t historySize = 20;
    rto::MovingAveragePredictor<uint64_t, int64_t>* pPredictor = new rto::MovingAveragePredictor<uint64_t, int64_t>(historySize, historySize >> 1);
    double uiErrorSum = testPredictor(pPredictor, vDeltasMs);
    VLOG(RTO_TEST_LOG_LEVEL) << "Moving average - sum of error squared: " << uiErrorSum;
    delete pPredictor;

    rto::AR2Predictor<uint64_t, int64_t>* pAR2Predictor = new rto::AR2Predictor<uint64_t, int64_t>();
    double uiErrorSumAR2 = testPredictor(pAR2Predictor, vDeltasMs);
    VLOG(RTO_TEST_LOG_LEVEL) << "AR2 - sum of error squared: " << uiErrorSumAR2;
    delete pAR2Predictor;

    // TEST with double
#ifdef USE_MS_DOUBLE
    std::vector<double> vDeltasMs;
    vDeltasMs.resize(vDeltas.size());
    std::transform(vDeltas.begin(), vDeltas.end(), vDeltasMs.begin(),
                   [](uint32_t uiVal)
    {
      return uiVal/1000.0;
    });

    const uint32_t historySize = 20;
    MovingAveragePredictor<double>* pPredictor = new MovingAveragePredictor<double>(historySize);
    double errorSum = testPredictor(pPredictor, vDeltasMs);
    VLOG(RTO_TEST_LOG_LEVEL) << "Moving average - sum of error squared: " << errorSum;
    delete pPredictor;

    AR2Predictor<double>* pAR2Predictor = new AR2Predictor<double>();
    double errorSumAR = testPredictor(pAR2Predictor, vDeltasMs);
    VLOG(RTO_TEST_LOG_LEVEL) << "AR2 - sum of error squared: " << errorSumAR;
    delete pAR2Predictor;
#endif
  }

  static void testInnerProduct()
  {
    std::vector<int64_t> vErrors;
    vErrors.push_back(-55);
    vErrors.push_back(-12);
    vErrors.push_back(5222);
    vErrors.push_back(30);
    vErrors.push_back(11);
    vErrors.push_back(129);
    vErrors.push_back(-33);
    vErrors.push_back(163);
    vErrors.push_back(-42);
    vErrors.push_back(170);
    vErrors.push_back(-5);
    vErrors.push_back(29);
    vErrors.push_back(17);
    vErrors.push_back(-19);
    vErrors.push_back(16);
    vErrors.push_back(184);
    vErrors.push_back(9);
    vErrors.push_back(-67);
    vErrors.push_back(854);
    vErrors.push_back(-78);
    vErrors.push_back(30);
    vErrors.push_back(196);
    vErrors.push_back(35);
    vErrors.push_back(-54);
    vErrors.push_back(-414);
    vErrors.push_back(-280);
    vErrors.push_back(1452);
    vErrors.push_back(107);
    vErrors.push_back(-17154);
    vErrors.push_back(180);
    vErrors.push_back(-149);
    vErrors.push_back(-100);
    vErrors.push_back(3);
    vErrors.push_back(-312);
    vErrors.push_back(117);
    vErrors.push_back(-109);
    vErrors.push_back(-42);
    vErrors.push_back(-24);
    vErrors.push_back(8);
    vErrors.push_back(129);
    vErrors.push_back(-446);
    vErrors.push_back(32);
    vErrors.push_back(-42);
    vErrors.push_back(95);
    vErrors.push_back(-123);
    vErrors.push_back(64);
    vErrors.push_back(154);
    vErrors.push_back(-99);
    vErrors.push_back(55);
    vErrors.push_back(-103);
    vErrors.push_back(55);
    vErrors.push_back(2);
    vErrors.push_back(-285);
    vErrors.push_back(-55);
    vErrors.push_back(50);
    vErrors.push_back(-91);
    vErrors.push_back(52);
    vErrors.push_back(-54);
    vErrors.push_back(-225);
    vErrors.push_back(133);
    vErrors.push_back(59);
    vErrors.push_back(320);
    vErrors.push_back(-256);
    vErrors.push_back(-125);
    vErrors.push_back(605);
    vErrors.push_back(-150);
    vErrors.push_back(-484);
    vErrors.push_back(433);
    vErrors.push_back(166);
    vErrors.push_back(-1097);
    vErrors.push_back(-88);
    vErrors.push_back(-277);
    vErrors.push_back(-846);
    vErrors.push_back(-41);
    vErrors.push_back(-12);
    vErrors.push_back(-235);
    vErrors.push_back(133);
    vErrors.push_back(333);
    vErrors.push_back(60);
    vErrors.push_back(-222);
    vErrors.push_back(-107);
    vErrors.push_back(194);
    vErrors.push_back(-207);
    vErrors.push_back(212);
    vErrors.push_back(-30);
    vErrors.push_back(-609);
    vErrors.push_back(-151);
    vErrors.push_back(119);
    vErrors.push_back(314);
    vErrors.push_back(-158);
    vErrors.push_back(-105);
    vErrors.push_back(-298);
    vErrors.push_back(-110);
    vErrors.push_back(69);
    vErrors.push_back(146);
    vErrors.push_back(7);
    vErrors.push_back(126);
    vErrors.push_back(-99);
    vErrors.push_back(-86);
    vErrors.push_back(71);
    vErrors.push_back(-872);
    vErrors.push_back(62);
    vErrors.push_back(306);
    vErrors.push_back(-111);
    vErrors.push_back(64);
    vErrors.push_back(165);
    vErrors.push_back(-112);
    vErrors.push_back(-513);
    vErrors.push_back(-3);
    vErrors.push_back(-19);
    vErrors.push_back(88);
    vErrors.push_back(-81);
    vErrors.push_back(11);
    vErrors.push_back(-18);
    vErrors.push_back(-179);
    vErrors.push_back(-7);
    vErrors.push_back(105);
    vErrors.push_back(-31);
    vErrors.push_back(-67);
    vErrors.push_back(12);
    vErrors.push_back(-263);
    vErrors.push_back(111);
    vErrors.push_back(163);
    vErrors.push_back(-6);
    vErrors.push_back(109);
    vErrors.push_back(-268);
    vErrors.push_back(-160);
    vErrors.push_back(-143);
    vErrors.push_back(308);
    vErrors.push_back(-18);
    vErrors.push_back(-278);
    vErrors.push_back(-81);
    vErrors.push_back(-662);
    vErrors.push_back(290);
    vErrors.push_back(7557);
    vErrors.push_back(1450);
    vErrors.push_back(206);
    vErrors.push_back(-4502);
    vErrors.push_back(188);
    vErrors.push_back(-133);
    vErrors.push_back(-77);
    vErrors.push_back(-192);
    vErrors.push_back(-77);
    vErrors.push_back(-4);
    vErrors.push_back(-25);
    vErrors.push_back(-26);
    vErrors.push_back(142);
    vErrors.push_back(-202);
    vErrors.push_back(-56);
    vErrors.push_back(-97);
    vErrors.push_back(-8);
    vErrors.push_back(-21);
    vErrors.push_back(-115);
    vErrors.push_back(52);
    vErrors.push_back(136);
    vErrors.push_back(-207);
    vErrors.push_back(-175);
    vErrors.push_back(4);
    vErrors.push_back(13);
    vErrors.push_back(9);
    vErrors.push_back(-49);
    vErrors.push_back(67);
    vErrors.push_back(-6);
    vErrors.push_back(25);
    vErrors.push_back(623);
    vErrors.push_back(-145);
    vErrors.push_back(95);
    vErrors.push_back(-65);
    vErrors.push_back(-2);
    vErrors.push_back(128);
    vErrors.push_back(-340);
    vErrors.push_back(-101);
    vErrors.push_back(121);
    vErrors.push_back(-7008);
    vErrors.push_back(-405);
    vErrors.push_back(220);
    vErrors.push_back(600);
    vErrors.push_back(-346);
    vErrors.push_back(27);
    vErrors.push_back(-7);
    vErrors.push_back(-15);
    vErrors.push_back(-2);
    vErrors.push_back(-77);
    vErrors.push_back(5);
    vErrors.push_back(-19);
    vErrors.push_back(66);
    vErrors.push_back(85);
    vErrors.push_back(-40);
    vErrors.push_back(-38);
    vErrors.push_back(-8);
    vErrors.push_back(-44);
    vErrors.push_back(95);
    vErrors.push_back(102);
    vErrors.push_back(-222);
    vErrors.push_back(-17);
    vErrors.push_back(56);
    vErrors.push_back(44);
    vErrors.push_back(16);
    vErrors.push_back(-149);
    vErrors.push_back(106);
    vErrors.push_back(298);
    vErrors.push_back(-121);
    vErrors.push_back(73);
    vErrors.push_back(-142);
    vErrors.push_back(54);
    vErrors.push_back(-88);
    vErrors.push_back(-32);
    vErrors.push_back(-17);
    vErrors.push_back(-130);
    vErrors.push_back(124);
    vErrors.push_back(5540);
    vErrors.push_back(-189);
    vErrors.push_back(91);
    vErrors.push_back(5);
    vErrors.push_back(-50);
    vErrors.push_back(-128);
    vErrors.push_back(20);
    vErrors.push_back(-10);
    vErrors.push_back(-44);
    vErrors.push_back(-53);
    vErrors.push_back(117);
    vErrors.push_back(32);
    vErrors.push_back(-121);
    vErrors.push_back(-8);
    vErrors.push_back(105);
    vErrors.push_back(990);
    vErrors.push_back(-132);
    vErrors.push_back(-20);
    vErrors.push_back(-10);
    vErrors.push_back(52);
    vErrors.push_back(-8);
    vErrors.push_back(102);
    vErrors.push_back(-1133);
    vErrors.push_back(-144);
    vErrors.push_back(97);
    vErrors.push_back(-5);
    vErrors.push_back(7);
    vErrors.push_back(-59);
    vErrors.push_back(44);
    vErrors.push_back(-2019);
    vErrors.push_back(-492);
    vErrors.push_back(202);
    double dSumErrorSquared = std::inner_product(vErrors.begin(), vErrors.end(), vErrors.begin(), 0.0);
    double dError = dSumErrorSquared/vErrors.size();
    VLOG(RTO_TEST_LOG_LEVEL) << "Error: " << dError;
  }
};

} // test
} // rtp_plus_plus
