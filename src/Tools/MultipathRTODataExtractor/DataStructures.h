#pragma once
#include <cstdint>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>

struct PacketInfo
{
  PacketInfo()
    :TIME(0),
      OWD(-1)
  {

  }

  PacketInfo(boost::posix_time::ptime tSent, uint32_t uiTime, std::string sn, std::string fid, std::string fssn)
    :SENT(tSent),
      TIME(uiTime),
      SN(sn),
      FID(fid),
      FSSN(fssn),
      OWD(-1)
  {

  }

  // time the packet was sent extracted from log
  boost::posix_time::ptime SENT;
  boost::posix_time::ptime RECEIVED;
  // difference from time first packet was sent in ms
  int32_t TIME;
  // Sequence number
  std::string SN;
  // Flow id
  std::string FID;
  // Flow specific sequence number
  std::string FSSN;
  // One way delay in ms
  int32_t OWD;
};

struct PathSpecificEstimationInfo
{
  PathSpecificEstimationInfo()
    :TIME(0),
      DELAY(-1)
  {

  }

  PathSpecificEstimationInfo(boost::posix_time::ptime tEstimatedAt, uint32_t uiTime, std::string flow_id, std::string fssn, int delay)
    :ESTIMATED_AT(tEstimatedAt),
      TIME(uiTime),
      FLOWID(flow_id),
      FSSN(fssn),
      DELAY(delay)
  {

  }

  boost::posix_time::ptime ESTIMATED_AT;
  boost::posix_time::ptime ESTIMATED;
  int32_t TIME; // difference from time first packet was sent in ms
  std::string FLOWID;
  std::string FSSN;    // Flow specific sequence number
  int32_t DELAY;  // One way delay in ms
};

struct CrosspathEstimationInfo
{
  CrosspathEstimationInfo()
    :LONG_TIMEOUT_OCCURRED(false),
      TIME(0),
      SHORT_DELAY(-1),
      LONG_DELAY1(-1),
      LONG_DELAY2(-1),
      EARLY_DETECT(-1)
  {

  }

  CrosspathEstimationInfo(boost::posix_time::ptime tEstimatedAt, uint32_t uiTime, std::string sn)
    :ESTIMATED_AT(tEstimatedAt),
      LONG_TIMEOUT_OCCURRED(false),
      TIME(uiTime),
      SN(sn),
      SHORT_DELAY(-1),
      LONG_DELAY1(-1),
      LONG_DELAY2(-1),
      EARLY_DETECT(-1)
  {

  }

  boost::posix_time::ptime ESTIMATED_AT;
  boost::posix_time::ptime ESTIMATED_SHORT;
  boost::posix_time::ptime ESTIMATED_LONG;
  bool LONG_TIMEOUT_OCCURRED;

  int32_t TIME; // difference from time first packet was sent in ms
  std::string SN;    // Sequence number
  boost::posix_time::ptime SHORT_TIMEOUT;
  int32_t SHORT_DELAY;  // One way delay in ms
  int32_t LONG_DELAY1;  // One way delay in ms
  int32_t LONG_DELAY2;  // One way delay in ms
  int32_t EARLY_DETECT;
};

struct EarlyLossDetection
{
  EarlyLossDetection(boost::posix_time::ptime tDetected, std::string fid, std::string fssn)
    :DETECTED(tDetected),
      FID(fid),
      FSSN(fssn)
  {

  }

  boost::posix_time::ptime DETECTED;
  std::string FID;    // Flow id
  std::string FSSN;   // Flow specific sequence number
};

struct EstimationStatistics
{
  EstimationStatistics()
    :TIME(0),
      OWD(0),
      PS_TIME_EST(0),
      PS_OVERWAITING_TIME(-1),
      PS_ERR(0),
      PS_FALSE_POSITIVE(false),
      CP_TIME_EST(0),
      CP_OVERWAITING_TIME(-1),
      CP_ERR(0),
      CP_FALSE_POSITIVE(false)
  {

  }

  std::string SN;    // Sequence number
  std::string FLOWID;
  std::string FSSN;    // Flow specific sequence number
  boost::posix_time::ptime SENT;
  boost::posix_time::ptime RECEIVED;
  boost::posix_time::ptime PS_ESTIMATED;
  boost::posix_time::ptime CP_ESTIMATED;

  // difference from time first packet was sent in ms
  int32_t TIME;
  // One way delay in ms
  int32_t OWD;
  int PS_TIME_EST;
  int PS_OVERWAITING_TIME;
  int PS_ERR;
  bool PS_FALSE_POSITIVE;
  int CP_TIME_EST;
  int CP_OVERWAITING_TIME;
  int CP_ERR;
  bool CP_FALSE_POSITIVE;
};
