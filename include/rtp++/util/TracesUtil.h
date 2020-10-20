#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <cpputil/FileUtil.h>

namespace rtp_plus_plus
{

class TracesUtil
{
public:
  static bool loadOneWayDelayDataFile(const std::string& sDataFile, std::vector<double>& vDelays)
  {
    if (!FileUtil::fileExists(sDataFile))
    {
      LOG(WARNING) << "Data file does not exist: " << sDataFile;
      return false;
    }

    const std::string sHeader("Date Time SN Delay");
    // data file starts with line
    /*
      Date Time SN Delay
      1203 17:02:06.955836 23756 0.045144
      1203 17:02:07.035830 23758 0.045073
    */
    std::ifstream if1(sDataFile.c_str(), std::ifstream::in );
    std::string sLine;
    getline(if1, sLine);
    std::istringstream istr(sLine);
    if (sHeader != sLine)
    {
      LOG(WARNING) << "Header line of data file does not match";
      return false;
    }

    std::string sDate, sTime;
    uint16_t uiSN = 0;
    double dOneWayDelay = 0.0;

    getline(if1, sLine);
    while (if1.good())
    {
      std::istringstream istr(sLine);
      istr >> sDate >> sTime >> uiSN >> dOneWayDelay;
      vDelays.push_back(dOneWayDelay);
      getline(if1, sLine);
    }

    return true;
  }

  static bool loadInterarrivalDataFile(const std::string& sDataFile, std::vector<uint32_t>& vInterarrivalTimesMicroseconds)
  {
    if (!FileUtil::fileExists(sDataFile))
    {
      LOG(WARNING) << "Data file does not exist: " << sDataFile;
      return false;
    }

    std::ifstream if1(sDataFile.c_str(), std::ifstream::in );
    uint32_t uiDelta = 0;

    std::string sLine;
    getline(if1, sLine);
    while (if1.good())
    {
      std::istringstream istr(sLine);
      istr >> uiDelta;
      vInterarrivalTimesMicroseconds.push_back(uiDelta);
      // hack for debugging
      if (vInterarrivalTimesMicroseconds.size() == 10)
        break;
      getline(if1, sLine);
    }

    return true;
  }
};

} // rtp_plus_plus

