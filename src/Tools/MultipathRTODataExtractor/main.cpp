#include "MultipathRTODataExtractorPch.h"
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/Version.h>

#include "DataStructures.h"

using namespace std;
using namespace boost::filesystem;
using namespace boost::gregorian;

/// Application configuration
// I0317 10:26:54.035169 24691 main.cpp:228] ./MultipathRTO --sdp icme/tub_s_mp_fb.sdp --nc icme/s_mprtp.cfg --channel-config icme/channel_mprtp.cfg --buf-lat 1000 --mprtp-sa rr --pred mavg --mp-pred mp-comp --dur 50 --pto 0.1
const std::string APPLICATION("./MultipathRTO ");
/// Time packet is sent
// I0315 15:34:22.865576 24796 MpRtpSession.cpp:106] _LogSentinel_ Packet sent SN: 33179 Flow Id: 1 FSSN: 59972 Time: 2013-Mar-15 14:34:22.865569
const std::string SENT("] _LogSentinel_ Packet sent SN: ");
/// Time packet is received
// I0315 15:34:22.852108 24795 MpRtpFeedbackManager.cpp:83] _LogSentinel_ Packet arrived SN: 33173 Flow Id: 1 FSSN: 59968 Arrival: 2013-Mar-15 14:34:22.852096
const std::string ARRIVED("] _LogSentinel_ Packet arrived SN: ");
/// Crosspath estimator
// I0315 15:39:19.817910 24964 CrossPathRtoEstimator.cpp:494] _LogSentinel_ CP packet estimation now: 2013-Mar-15 14:39:19.817908 SN: 26936 short: 2013-Mar-15 14:39:19.857963 long: 2013-Mar-15 14:39:20.039163
const std::string CP_ESTIMATED("] _LogSentinel_ CP packet estimation");
// I0227 15:56:29.047559 26771 CrossPathRtoEstimator.cpp:373] _LogSentinel_ Packet Short Timeout SN: 48143
const std::string CP_SHORT1("] _LogSentinel_ Packet Short Timeout SN: ");
// I0227 15:56:29.244698 26771 CrossPathRtoEstimator.cpp:335] _LogSentinel_ Packet Long Timeout Lost SN: 48143
const std::string CP_LONG_TO_LOST("] _LogSentinel_ Packet Long Timeout Lost SN: ");
// I0227 15:56:29.244698 26771 CrossPathRtoEstimator.cpp:335] _LogSentinel_ Packet Long Timeout Received SN: 48143
const std::string CP_LONG_TO_RECEIVED("] _LogSentinel_ Packet Long Timeout Received SN: ");
// I0304 11:08:46.228765 17993 CrossPathRtoEstimator.cpp:137] _LogSentinel_ On Packet Arrival Lost FlowId: 1 FSSN: 22647
const std::string CP_EARLY("] _LogSentinel_ On Packet Arrival Lost FlowId: ");

/// Path specific estimator
// I0315 15:39:19.892130 24964 BasicRtoEstimator.cpp:224] _LogSentinel_ PS packet estimation now: 2013-Mar-15 14:39:19.892127 ID: 0 SN: 50959 at: 2013-Mar-15 14:39:20.046609 (in 154 ms)
const std::string PS_ESTIMATED("] _LogSentinel_ PS packet estimation");

bool g_verbose = false;

/// method to construct ptime from google glog time
boost::posix_time::ptime constructDateTime(const string& log_date, const string& time)
{
  int year = boost::posix_time::second_clock::local_time().date().year();
  int month = convert<int>(log_date.substr(0, 2), false);
  int day = convert<int>(log_date.substr(2,2), false);
  date today(year, month, day);
  string complete_date = boost::gregorian::to_simple_string(today);
  string date_time = complete_date + " " +  time;
  boost::posix_time::ptime tDate = boost::posix_time::time_from_string(date_time);
  return tDate;
}

boost::optional<PacketInfo> findByFlowIdAndFssn(const map<uint16_t, PacketInfo>& packetInfoMap,
                                                const string& sFlowId, const string& sFSSN)
{
  for (auto& pair: packetInfoMap)
  {
    if (pair.second.FID == sFlowId && pair.second.FSSN == sFSSN)
      return boost::optional<PacketInfo>(pair.second);
  }
  return boost::optional<PacketInfo>();
}

void parseSentInfo(map<uint16_t, PacketInfo>& packetInfoMap,
                   boost::posix_time::ptime& tReference,
                   const string& sLine)
{
  bool bDummy = false;
  vector<string> tokens = StringTokenizer::tokenize(sLine, " ", false, true);
  assert(tokens.size() == 17);
  // create bogus date just so that we can do the calculations:
//  string date = tokens[0].substr(1);
//  string time_sent = tokens[1];
//  boost::posix_time::ptime tSent = constructDateTime(date, time_sent);

  string sDate = tokens[15];
  string sTime = tokens[16];
  boost::posix_time::ptime tSent = boost::posix_time::time_from_string(sDate + " " + sTime);

  const string sSN = tokens[8];
  uint16_t uiSN = convert<uint16_t>(sSN, bDummy);
  string sFlowId = tokens[11];
  uint16_t uiFlowId = convert<uint16_t>(sFlowId, bDummy);
  string sFSSN = tokens[13];
  if (g_verbose)
    cout << "Sent Time: " << tSent << " SN: " << sSN << " Flow: " << sFlowId << " FSSN: " << sFSSN << endl;
  if (tReference.is_not_a_date_time())
  {
    tReference = tSent;
    PacketInfo info(tSent, 0, sSN, sFlowId, sFSSN);
    packetInfoMap[uiSN] = info;
  }
  else
  {
    uint32_t uiDiff = (tSent - tReference).total_milliseconds();
    PacketInfo info(tSent, uiDiff, sSN, sFlowId, sFSSN);
    packetInfoMap[uiSN] = info;
  }
}

void parseReceivedInfo(map<uint16_t, PacketInfo>& packetInfoMap, const string& sLine)
{
  bool bDummy = false;
  vector<string> tokens = StringTokenizer::tokenize(sLine, " ", false, true);
  assert(tokens.size() == 17);
//  string date = tokens[0].substr(1);
//  string time_received = tokens[1];
//  boost::posix_time::ptime tRecv = constructDateTime(date, time_received);

  string sDate = tokens[15];
  string sTime = tokens[16];
  boost::posix_time::ptime tRecv = boost::posix_time::time_from_string(sDate + " " + sTime);

  string sSN = tokens[8];
  uint16_t uiSN = convert<uint16_t>(sSN, bDummy);

  if (g_verbose)
    cout << "Recv Time: " << tRecv << " SN: " << sSN << endl;
  packetInfoMap[uiSN].RECEIVED = tRecv;
  uint32_t uiDiff = (tRecv - packetInfoMap[uiSN].SENT).total_milliseconds();
  packetInfoMap[uiSN].OWD = uiDiff;
}

void parsePathSpecificEstimate(const boost::posix_time::ptime& tReference,
                               map<uint16_t, vector<PathSpecificEstimationInfo> >& psEstimationMap,
                               map<uint16_t, boost::posix_time::ptime>& tPsReference,
                               map<uint16_t, uint32_t>& mDiffToSendReference,
                               const string& sLine)
{
  bool bDummy = false;
  vector<string> tokens = StringTokenizer::tokenize(sLine, " ", false, true);
  // create bogus date just so that we can do the calculations:
//  string date = tokens[0].substr(1);
//  string time_sent = tokens[1];
//  boost::posix_time::ptime tEst = constructDateTime(date, time_sent);

  string sDate = tokens[9];
  string sTime = tokens[10];
  boost::posix_time::ptime tEstimatedAt = boost::posix_time::time_from_string(sDate + " " + sTime);

  string sFlowId = tokens[12];
  uint16_t uiFlowId = convert<uint16_t>(sFlowId, bDummy);

  string sFSSN = tokens[14];
  uint16_t uiFSSN = convert<uint16_t>(sFSSN, bDummy);

  string sDate2 = tokens[16];
  string sTime2 = tokens[17];
  boost::posix_time::ptime tEstimated = boost::posix_time::time_from_string(sDate2 + " " + sTime2);

  string sDelay = tokens[19];
  int32_t iDelay = convert<int32_t>(sDelay, bDummy);

  if (g_verbose)
    cout << "Estimated at : " << tEstimatedAt << " FID: " << sFlowId << " FSSN: " << sFSSN << " estimation: " << tEstimated << endl;

  if (tPsReference.find(uiFlowId) == tPsReference.end())
  {
    tPsReference[uiFlowId] = tEstimatedAt;
    mDiffToSendReference[uiFlowId] = (tEstimatedAt - tReference).total_milliseconds();
    PathSpecificEstimationInfo info(tEstimatedAt, mDiffToSendReference[uiFlowId], sFlowId, sFSSN, iDelay);
    info.ESTIMATED = tEstimated;
    psEstimationMap[uiFlowId].push_back(info);
  }
  else
  {
    uint32_t uiDiff = (tEstimatedAt - tPsReference[uiFlowId]).total_milliseconds();
    uint32_t uiDiffToReference = mDiffToSendReference[uiFlowId];
    PathSpecificEstimationInfo info(tEstimatedAt, uiDiff + uiDiffToReference, sFlowId, sFSSN, iDelay);
    info.ESTIMATED = tEstimated;
    auto it = std::find_if(psEstimationMap[uiFlowId].begin(),
                           psEstimationMap[uiFlowId].end(),
                           [sFSSN](PathSpecificEstimationInfo& estInfo)
    {
       return (estInfo.FSSN == sFSSN);
    });
    if (it == psEstimationMap[uiFlowId].end())
      psEstimationMap[uiFlowId].push_back(info);
    else
      *it = info;
  }
}

void parseCrossPathEstimate(const boost::posix_time::ptime& tReference,
                            boost::posix_time::ptime& tEstReference,
                            uint32_t& uiDiffToSendReference,
                            map<uint16_t, CrosspathEstimationInfo>& estimationMap,
                            const string& sLine)
{
  bool bDummy = false;
  vector<string> tokens = StringTokenizer::tokenize(sLine, " ", false, true);
  // create bogus date just so that we can do the calculations:
//  string date = tokens[0].substr(1);
//  string time_sent = tokens[1];
//  boost::posix_time::ptime tEst = constructDateTime(date, time_sent);

  string sDate = tokens[9];
  string sTime = tokens[10];
  boost::posix_time::ptime tEstimatedAt = boost::posix_time::time_from_string(sDate + " " + sTime);

  string sSN = tokens[12];
  uint16_t uiSN = convert<uint16_t>(sSN, bDummy);

  string sDate2 = tokens[14];
  string sTime2 = tokens[15];
  boost::posix_time::ptime tEstimatedShort = boost::posix_time::time_from_string(sDate2 + " " + sTime2);

  string sDate3 = tokens[17];
  string sTime3 = tokens[18];
  boost::posix_time::ptime tEstimatedLong = boost::posix_time::time_from_string(sDate3 + " " + sTime3);

  if (g_verbose)
    cout << "Estimated: " << tEstimatedAt << " SN: " << sSN << endl;
  if (tEstReference.is_not_a_date_time())
  {
    tEstReference = tEstimatedAt;
    uiDiffToSendReference = (tEstReference - tReference).total_milliseconds();
    CrosspathEstimationInfo info(tEstimatedAt, uiDiffToSendReference, sSN);
    info.ESTIMATED_SHORT = tEstimatedShort;
    info.ESTIMATED_LONG = tEstimatedLong;
    estimationMap[uiSN] = info;
  }
  else
  {
    uint32_t uiDiff = (tEstimatedAt - tEstReference).total_milliseconds();
    CrosspathEstimationInfo info(tEstimatedAt, uiDiff + uiDiffToSendReference, sSN);
    info.ESTIMATED_SHORT = tEstimatedShort;
    info.ESTIMATED_LONG = tEstimatedLong;
    estimationMap[uiSN] = info;
  }
}

/*
void parseCrossPathEstimateShortTimeout(map<uint16_t, CrosspathEstimationInfo>& estimationMap,
                            const string& sLine)
{
  bool bDummy = false;
  vector<string> tokens = StringTokenizer::tokenize(sLine, " ", false, true);
  string date = tokens[0].substr(1);
  string time_received = tokens[1];
  boost::posix_time::ptime tShort = constructDateTime(date, time_received);
  string sequence_number = tokens[9];
  uint16_t uiSN = convert<uint16_t>(sequence_number, bDummy);

  cout << "Short Timeout: " << tShort << " SN: " << sequence_number << endl;
  uint32_t uiDiff = (tShort - estimationMap[uiSN].ESTIMATED).total_milliseconds();
  estimationMap[uiSN].SHORT_DELAY = uiDiff;
  estimationMap[uiSN].SHORT_TIMEOUT = tShort;
}

void parseCrossPathEstimateLongTimeoutLost(map<uint16_t, CrosspathEstimationInfo>& estimationMap,
                            const string& sLine)
{
  bool bDummy = false;
  vector<string> tokens = StringTokenizer::tokenize(sLine, " ", false, true);
  string date = tokens[0].substr(1);
  string time_received = tokens[1];
  boost::posix_time::ptime tLong = constructDateTime(date, time_received);
  string sequence_number = tokens[10];
  uint16_t uiSN = convert<uint16_t>(sequence_number, bDummy);

  cout << "Long Timeout: " << tLong << " SN: " << sequence_number << endl;
  uint32_t uiDiff = (tLong - estimationMap[uiSN].SHORT_TIMEOUT).total_milliseconds();
  estimationMap[uiSN].LONG_DELAY1 = uiDiff;
}

void parseCrossPathEstimateLongTimeoutReceived(map<uint16_t, CrosspathEstimationInfo>& estimationMap,
                            const string& sLine)
{
  bool bDummy = false;
  vector<string> tokens = StringTokenizer::tokenize(sLine, " ", false, true);
  string date = tokens[0].substr(1);
  string time_received = tokens[1];
  boost::posix_time::ptime tLong = constructDateTime(date, time_received);
  string sequence_number = tokens[10];
  uint16_t uiSN = convert<uint16_t>(sequence_number, bDummy);
  cout << "Long Timeout: " << tLong << " SN: " << sequence_number << endl;
  uint32_t uiDiff = (tLong - estimationMap[uiSN].SHORT_TIMEOUT).total_milliseconds();
  estimationMap[uiSN].LONG_DELAY2 = uiDiff;
}
*/

void parseCrossPathEstimateLongTimeoutLost(map<uint16_t, CrosspathEstimationInfo>& cpEstimationMap,
                            const string& sLine)
{
  bool bDummy = false;
  vector<string> tokens = StringTokenizer::tokenize(sLine, " ", false, true);
  string sequence_number = tokens[10];
  uint16_t uiSN = convert<uint16_t>(sequence_number, bDummy);
  auto it = cpEstimationMap.find(uiSN);
  if (it == cpEstimationMap.end()) return;
  cpEstimationMap[uiSN].LONG_TIMEOUT_OCCURRED = true;
}

void parseCrossPathEstimateLongTimeoutReceived(map<uint16_t, CrosspathEstimationInfo>& cpEstimationMap,
                            const string& sLine)
{
  bool bDummy = false;
  vector<string> tokens = StringTokenizer::tokenize(sLine, " ", false, true);
  string sequence_number = tokens[10];
  uint16_t uiSN = convert<uint16_t>(sequence_number, bDummy);
  auto it = cpEstimationMap.find(uiSN);
  if (it == cpEstimationMap.end()) return;
  cpEstimationMap[uiSN].LONG_TIMEOUT_OCCURRED = true;
}

void parseCrossPathEarlyDetections(vector<EarlyLossDetection>& earlyDetections,
                                   const string& sLine)
{
  vector<string> tokens = StringTokenizer::tokenize(sLine, " ", false, true);
  // create bogus date just so that we can do the calculations:
  string date = tokens[0].substr(1);
  string time_sent = tokens[1];
  boost::posix_time::ptime tDetected = constructDateTime(date, time_sent);
  string flow_id = tokens[10];
  string fs_sequence_number = tokens[12];
  earlyDetections.push_back(EarlyLossDetection(tDetected, flow_id, fs_sequence_number));
}

void reconcileEarlyDetections(vector<EarlyLossDetection>& earlyDetections,
                              map<uint16_t, CrosspathEstimationInfo>& cpEstimationMap,
                              map<uint16_t, PacketInfo>& packetInfoMap)
{
  for (EarlyLossDetection& early: earlyDetections)
  {
    // find SN in packet info map
    for (auto& pair: packetInfoMap )
    {
      if (pair.second.FID == early.FID &&
          pair.second.FSSN == early.FSSN )
      {
        // found match
        uint32_t uiDiffMs = (early.DETECTED - pair.second.SENT).total_milliseconds();
        // now search for SN in estimation map
        bool bDummy = false;
        uint16_t uiSN = convert<uint16_t>(pair.second.SN, bDummy);
        // subtract short timeout to compensate for offset from send time
        auto it = cpEstimationMap.find(uiSN);
        if (it == cpEstimationMap.end()) continue;
        cpEstimationMap[uiSN].EARLY_DETECT = uiDiffMs;
      }
    }
  }
}

void writePacketInfo(const std::string& sFile, map<uint16_t, PacketInfo>& packetInfoMap)
{
  std::ofstream out(sFile.c_str(), ios_base::out);
  out << "Time SN FID FSSN OWD" << endl;
  for (auto& pair: packetInfoMap)
  {
    out << pair.second.TIME << " "
        << pair.second.SN << " "
        << pair.second.FID << " "
        << pair.second.FSSN << " "
        << pair.second.OWD
        << endl;
  }
}

void writeCpEstimationInfo(const std::string& sFile, map<uint16_t, CrosspathEstimationInfo>& estimationMap)
{
  std::ofstream out(sFile.c_str(), ios_base::out);
  out << "Time SN SHORT LONG LONG_USED" << endl;
  for (auto& pair: estimationMap)
  {
    CrosspathEstimationInfo& info = pair.second;
    uint32_t uiShortDelay = (info.ESTIMATED_SHORT - info.ESTIMATED_AT).total_milliseconds();
    uint32_t uiLongDelay = (info.ESTIMATED_LONG - info.ESTIMATED_AT).total_milliseconds();
//    out << pair.second.TIME << " "
//         << pair.second.SN << " "
//         << pair.second.SHORT_DELAY << " "
//         << pair.second.LONG_DELAY1 << " "
//         << pair.second.LONG_DELAY2 << " "
//         << pair.second.EARLY_DETECT
//         << endl;
    out << info.TIME << " "
        << info.SN << " "
        << uiShortDelay << " "
        << uiLongDelay << " "
        << info.LONG_TIMEOUT_OCCURRED
        << endl;
  }
}

void writePsEstimationInfo(map<uint16_t, vector<PathSpecificEstimationInfo> >& psEstimationMap)
{
  for (auto pair: psEstimationMap)
  {
    uint16_t uiFlowId = pair.first;
    vector<PathSpecificEstimationInfo>& vInfo = pair.second;
    ostringstream oss;
    oss << "mprtp_ps_est_data_" << uiFlowId << ".txt";
    std::ofstream out(oss.str().c_str(), ios_base::out);
    out << "Time ID SN DELAY" << endl;
    for (auto& info: vInfo)
    {
      uint32_t uiDelay = (info.ESTIMATED - info.ESTIMATED_AT).total_milliseconds();
      out << info.TIME << " "
          << info.FLOWID << " "
          << info.FSSN << " "
          << uiDelay
          << endl;
    }
  }
}

map<uint16_t, EstimationStatistics> collateStatistics(map<uint16_t, PacketInfo>& packetInfoMap,
                                                      map<uint16_t, vector<PathSpecificEstimationInfo> >& psEstimationMap,
                                                      map<uint16_t, CrosspathEstimationInfo>& cpEstimationMap)
{
  map<uint16_t, EstimationStatistics> mStats;
  uint32_t uiCpFpCount = 0;
  uint32_t uiPsFpCount = 0;
  bool bDummy;
  for (auto pair: psEstimationMap)
  {
    uint16_t uiFlowId = pair.first;
    vector<PathSpecificEstimationInfo>& vInfo = pair.second;

    for (auto& info: vInfo)
    {
      string fssn = info.FSSN;
      // get global sequence number and packet info
      boost::optional<PacketInfo> pPacketInfo = findByFlowIdAndFssn(packetInfoMap, info.FLOWID, info.FSSN);
      // check if we have a record for the packet
      if (!pPacketInfo) continue;
      PacketInfo& packetInfo = *pPacketInfo;
      if (packetInfo.OWD == -1) continue;
      string sn = packetInfo.SN;
      uint16_t uiSN = convert<uint16_t>(sn, bDummy);
      // search for SN in crosspath method
      // check if we have a matching record in the cross path method
      auto it = cpEstimationMap.find(uiSN);
      if (it == cpEstimationMap.end()) continue;
      CrosspathEstimationInfo& rCrossPathInfo = cpEstimationMap[uiSN];

      EstimationStatistics stats;
      stats.SN = sn;
      stats.SENT = packetInfo.SENT;
      stats.RECEIVED = packetInfo.RECEIVED;
      stats.TIME = packetInfo.TIME;
      stats.OWD = packetInfo.OWD;
      stats.FLOWID = info.FLOWID;
      stats.FSSN = info.FSSN;
      // PS
      stats.PS_ESTIMATED = info.ESTIMATED;
      uint32_t uiDelayMs = (info.ESTIMATED - info.ESTIMATED_AT).total_milliseconds();
      stats.PS_TIME_EST = info.TIME + uiDelayMs;

      if (stats.PS_ESTIMATED > stats.RECEIVED)
      {
        int32_t iError =  (stats.PS_ESTIMATED - stats.RECEIVED).total_milliseconds();
        stats.PS_ERR = iError;
        stats.PS_OVERWAITING_TIME = iError;
      }
      else
      {
        int32_t iError =  -1 * (stats.RECEIVED - stats.PS_ESTIMATED).total_milliseconds();
        stats.PS_ERR = iError;
        ++uiPsFpCount;
        stats.PS_FALSE_POSITIVE = true;
      }

      //CP
      // RG: if a packet is lost, we have to wait for the long timeout anyway
#if 1
      if (rCrossPathInfo.LONG_TIMEOUT_OCCURRED)
      {
        uint32_t uiDelayUs = (rCrossPathInfo.ESTIMATED_LONG - rCrossPathInfo.ESTIMATED_AT).total_microseconds();
        uint32_t uiDelayMs = static_cast<uint32_t>(uiDelayUs/1000 + 0.5);
        stats.CP_TIME_EST = rCrossPathInfo.TIME + uiDelayMs;
        stats.CP_ESTIMATED = rCrossPathInfo.ESTIMATED_LONG;
      }
      else
      {
        uint32_t uiDelayUs = (rCrossPathInfo.ESTIMATED_SHORT - rCrossPathInfo.ESTIMATED_AT).total_microseconds();
        uint32_t uiDelayMs = static_cast<uint32_t>(uiDelayUs/1000 + 0.5);
        stats.CP_TIME_EST = rCrossPathInfo.TIME + uiDelayMs;
        stats.CP_ESTIMATED = rCrossPathInfo.ESTIMATED_SHORT;
      }
#else
      uint32_t uiCpDelayUs = (rCrossPathInfo.ESTIMATED_LONG - rCrossPathInfo.ESTIMATED_AT).total_microseconds();
      uint32_t uiCpDelayMs = static_cast<uint32_t>(uiCpDelayUs/1000 + 0.5);
      stats.CP_TIME_EST = rCrossPathInfo.TIME + uiCpDelayMs;
      stats.CP_ESTIMATED = rCrossPathInfo.ESTIMATED_LONG;
#endif
      if (stats.CP_ESTIMATED > stats.RECEIVED)
      {
        int32_t iError =  (stats.CP_ESTIMATED - stats.RECEIVED).total_milliseconds();
        stats.CP_ERR = iError;
        stats.CP_OVERWAITING_TIME = iError;
      }
      else
      {
        int32_t iError =  -1 * (stats.RECEIVED - stats.CP_ESTIMATED).total_milliseconds();
        stats.CP_ERR = iError;
        ++uiCpFpCount;
        stats.CP_FALSE_POSITIVE = true;
      }

      mStats[uiSN] = stats;
    }
  }
  return mStats;
}

void printStatistics(const map<uint16_t, EstimationStatistics>& mStats)
{
  std::ofstream out("mprtp_stats_summary.txt", ios_base::out);
  out << "SN ID FSSN SENT RECEIVED PS_EST PS_WAIT PS_ERR PS_FP CP_EST CP_WAIT CP_ERR CR_FP" << endl;
  for (auto& pair : mStats)
  {
    const EstimationStatistics& rStat = pair.second;
    out << rStat.SN << " "
            << rStat.FLOWID << " "
            << rStat.FSSN << " "
            << rStat.TIME << " "
            << rStat.TIME + rStat.OWD << " "
            << rStat.PS_TIME_EST << " "
            << rStat.PS_OVERWAITING_TIME << " "
            << rStat.PS_ERR << " "
            << rStat.PS_FALSE_POSITIVE << " "
            << rStat.CP_TIME_EST << " "
            << rStat.CP_OVERWAITING_TIME << " "
            << rStat.CP_ERR << " "
            << rStat.CP_FALSE_POSITIVE << " "
            << endl;
  }
}

void calculateMeanAndStandardDeviation(const vector<int32_t>& vStats, double& dMean, double& dStdDev )
{
  dMean = std::accumulate(vStats.begin(), vStats.end(), 0)/static_cast<double>(vStats.size());
  std::vector<double> vVariance(vStats.size());
  std::transform(vStats.begin(), vStats.end(), vVariance.begin(), [dMean](int32_t val)
  {
    return val - dMean;
  });
  double dSumErrorSquared = std::inner_product(vVariance.begin(), vVariance.end(), vVariance.begin(), 0.0);
  dStdDev = std::sqrt(dSumErrorSquared/vVariance.size());
}

void calculateStatistic(const map<uint16_t, EstimationStatistics>& mStats)
{
  vector<int32_t> vPsError;
  vector<int32_t> vCpError;
  vector<int32_t> vPsOverwaitingTime;
  vector<int32_t> vCpOverwaitingTime;
  for (auto& pair : mStats)
  {
    const EstimationStatistics& rStat = pair.second;
    vPsError.push_back(rStat.PS_ERR);
    vCpError.push_back(rStat.CP_ERR);
    if (rStat.PS_OVERWAITING_TIME > 0)
      vPsOverwaitingTime.push_back(rStat.PS_OVERWAITING_TIME);
    if (rStat.CP_OVERWAITING_TIME > 0)
      vCpOverwaitingTime.push_back(rStat.CP_OVERWAITING_TIME);
  }

  double dPsMean = 0.0, dPsStdDev = 0.0;
  calculateMeanAndStandardDeviation(vPsError, dPsMean, dPsStdDev);

  uint32_t uiPsFalsePositives = std::count_if(mStats.begin(), mStats.end(), [](const pair<uint16_t, EstimationStatistics>& val)
  {
    return val.second.PS_FALSE_POSITIVE;
  });

  cout << " PS Mean error: " << dPsMean
          << " Std Dev: " << dPsStdDev
          << " Measurements: " << vPsError.size()
          << " False positives: " << uiPsFalsePositives
          << endl;

  double dPsOverwaitingMean = 0.0, dPsOverwaitingStdDev = 0.0;
  calculateMeanAndStandardDeviation(vPsOverwaitingTime, dPsOverwaitingMean, dPsOverwaitingStdDev);
  cout << " PS Mean overwaiting time: " << dPsOverwaitingMean
          << " Std Dev: " << dPsOverwaitingStdDev
          << " Measurements: " << vPsOverwaitingTime.size()
          << endl;


  double dCpMean = 0.0, dCpStdDev = 0.0;
  calculateMeanAndStandardDeviation(vCpError, dCpMean, dCpStdDev);
  uint32_t uiCpFalsePositives = std::count_if(mStats.begin(), mStats.end(), [](const pair<uint16_t, EstimationStatistics>& val)
  {
    return val.second.CP_FALSE_POSITIVE;
  });

  cout << " CP Mean error: " << dCpMean
          << " Std Dev: " << dCpStdDev
          << " Measurements: " << vCpError.size()
          << " False positives: " << uiCpFalsePositives
          << endl;

  double dCpOverwaitingMean = 0.0, dCpOverwaitingStdDev = 0.0;
  calculateMeanAndStandardDeviation(vCpOverwaitingTime, dCpOverwaitingMean, dCpOverwaitingStdDev);
  cout << " CP Mean overwaiting time: " << dCpOverwaitingMean
          << " Std Dev: " << dCpOverwaitingStdDev
          << " Measurements: " << vCpOverwaitingTime.size()
          << endl;
}

void calculateStatisticSummaries(const map<uint16_t, EstimationStatistics>& mStats)
{
  cout << "########### Total statistics: #############" << endl;

  calculateStatistic(mStats);

  // flow 0
  map<uint16_t, EstimationStatistics> mFlow0Stats;
  map<uint16_t, EstimationStatistics> mFlow1Stats;

  std::for_each(mStats.begin(), mStats.end(),
                [&mFlow0Stats, &mFlow1Stats](const pair<const uint16_t, const EstimationStatistics>& val)
  {
    if (val.second.FLOWID == "0")
    {
      mFlow0Stats[val.first] = val.second;
    }
    else
    {
      mFlow1Stats[val.first] = val.second;
    }
  });

  cout << "########### Flow 0 statistics: #############" << endl;
  calculateStatistic(mFlow0Stats);

  // flow 1
  cout << "########### Flow 1 statistics: #############" << endl;
  calculateStatistic(mFlow1Stats);
}

// In this code we assume that the local and remote SDPs match!
int main(int argc, char** argv)
{
  cout << "MultipathRTODataExtractor rtp++ version: " << RTP_PLUS_PLUS_VERSION_STRING;

  if (argc < 2)
  {
    cout << "Usage: " << argv[0] << " <<filename>> [<<output_filename>>] [-v]" << endl;
    return -1;
  }

  if (!exists(argv[1]))
  {
    cout << "File " << argv[1] << " does not exist" << endl;
    return -1;
  }
  else
  {
    // Look for verbosity
    for (int i = 1; i < argc; ++i)
    {
      if (memcmp(argv[i],"-v", 2) == 0)
      {
        g_verbose = true;
      }
    }

    std::ifstream in(argv[1], std::ios_base::in);
    std::string sLine;
    getline(in, sLine);

    // data structures for send/receive reference
    map<uint16_t, PacketInfo> packetInfoMap;
    boost::posix_time::ptime tReference;
    // data structure for path specific reference
    // flow_id -> fssn -> estimation
    map<uint16_t, vector<PathSpecificEstimationInfo> > psEstimationMap;
    // flow specfic reference times of first packet sent
    map<uint16_t, boost::posix_time::ptime> tPsReference;
    map<uint16_t, uint32_t> mDiffToSendReference;

    // data structure for cross path reference
    boost::posix_time::ptime tEstReference;
    map<uint16_t, CrosspathEstimationInfo> cpEstimationMap;
    vector<EarlyLossDetection> earlyDetections;
    uint32_t uiDiffToSendReference = 0;

    bool bDummy = false;
    while (in.good())
    {
      if (boost::algorithm::contains(sLine, APPLICATION))
      {
        cout << "Application: " << sLine << endl;
      }
      /* Packet sending and arrival times */
      else if (boost::algorithm::contains(sLine, SENT))
      {
        parseSentInfo(packetInfoMap, tReference, sLine);
      }
      else if (boost::algorithm::contains(sLine, ARRIVED))
      {
        parseReceivedInfo(packetInfoMap, sLine);
      }
      /* Path specific specific logging */
      else if (boost::algorithm::contains(sLine, PS_ESTIMATED))
      {
        parsePathSpecificEstimate(tReference, psEstimationMap, tPsReference, mDiffToSendReference, sLine);
      }
      /* Cross path specific logging */
      else if (boost::algorithm::contains(sLine, CP_ESTIMATED))
      {
        parseCrossPathEstimate(tReference, tEstReference, uiDiffToSendReference, cpEstimationMap, sLine);
      }
      else if (boost::algorithm::contains(sLine, CP_LONG_TO_LOST))
      {
        parseCrossPathEstimateLongTimeoutLost(cpEstimationMap, sLine);
      }
      else if (boost::algorithm::contains(sLine, CP_LONG_TO_RECEIVED))
      {
        parseCrossPathEstimateLongTimeoutReceived(cpEstimationMap, sLine);
      }

//      else if (boost::algorithm::contains(sLine, CP_SHORT1))
//      {
//        parseCrossPathEstimateShortTimeout(cpEstimationMap, sLine);
//      }
//      else if (boost::algorithm::contains(sLine, CP_LONG_TO_LOST))
//      {
//        parseCrossPathEstimateLongTimeoutLost(cpEstimationMap, sLine);
//      }
//      else if (boost::algorithm::contains(sLine, CP_LONG_TO_RECEIVED))
//      {
//        parseCrossPathEstimateLongTimeoutReceived(cpEstimationMap, sLine);
//      }
      else if (boost::algorithm::contains(sLine, CP_EARLY))
      {
        parseCrossPathEarlyDetections(earlyDetections, sLine);
      }

      getline(in, sLine);
    }

    // reconcile early detections with packet estimations
    // reconcileEarlyDetections(earlyDetections, cpEstimationMap, packetInfoMap);

    std::string sOutFile = "mprtp_data.txt";
    if (argc > 2)
    {
      sOutFile = string(argv[2]);
    }

    writePacketInfo(sOutFile, packetInfoMap);


    std::string sEstOutFile = "mprtp_cp_est_data.txt";
    if (argc > 3)
    {
      sEstOutFile = string(argv[3]);
    }

    writeCpEstimationInfo(sEstOutFile, cpEstimationMap);

    // reconcile path specific info
    writePsEstimationInfo(psEstimationMap);

    // finally calculate statistics
    if (!psEstimationMap.empty() && !cpEstimationMap.empty())
    {
      map<uint16_t, EstimationStatistics> mStats = collateStatistics(packetInfoMap, psEstimationMap, cpEstimationMap);
//      if (g_verbose)
      printStatistics(mStats);
      calculateStatisticSummaries(mStats);
    }
  }

  return 0;
}

