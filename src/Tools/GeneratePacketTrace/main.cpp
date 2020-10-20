#include "GeneratePacketTracePch.h"
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include <cpputil/Conversion.h>
#include <rtp++/Version.h>

using namespace std;

namespace bfs = boost::filesystem;

struct LineInfo
{
  LineInfo()
    :AU(0),
      SN(0),
      Nut(0)
  {

  }

  uint32_t AU;
  uint16_t SN;
  uint32_t Nut;
  std::string Layer;
  std::string LayerInfo;
  std::string Type;
};

int main(int argc, char **argv)
{
  cout << "GeneratePacketTrace rtp++ version: " << RTP_PLUS_PLUS_VERSION_STRING << endl;
  if (argc < 4 ){
    cout << "Usage of the GeneratePacketTrace: ./generatePacketTrace <au_trace> <total_aus> <output_trace> <<SVC=[0,1]>>" << endl;
    exit(1);
  }

  if (!bfs::exists(argv[1]))
  {
    std::cout << "Access Unit trace file " << argv[2] << "does not exist" << endl;
  }

  bool bSvc = false;

  // RG: getting rid of SVC hack
  if (argc == 5)
  {
    const char* szSvc = argv[4];
    const char* szNo = "0";
    const char* szYes = "1";

    if (memcmp(szSvc, szNo, 1) == 0)
    {
      cout << "Generating AVC packet trace" << endl;
    }
    else if (memcmp(szSvc, szYes, 1) == 0)
    {
      cout << "Generating SVC packet trace" << endl;
      bSvc = true;
    }
    else
    {
      cout << "Invalid argument" << endl;
      return -1;
    }
  }

  // HACK for SVC detection
  ifstream auTrace(argv[1], ios_base::in);
  std::map<uint32_t, std::vector<LineInfo> > mLineInfo;
  while (auTrace.good())
  {
    LineInfo info;
    auTrace >> info.AU >> info.SN >> info.Nut >> info.Layer >> info.LayerInfo  >> info.Type;
    // HACK to not store the last LineInfo after eof
    if (auTrace.good())
    {
      if (info.Layer == "EL")
        bSvc = true;

      if (mLineInfo.find(info.AU) == mLineInfo.end())
        mLineInfo[info.AU] = std::vector<LineInfo>();

      mLineInfo[info.AU].push_back(info);
    }
  }

  bool bDummy = false;
  const static string LAYER_SVC = "2";
  const static string LAYER_AVC = "1";
  const static string LAYER_FREEZE = "0";
  uint32_t uiTotalAus = convert<uint32_t>(argv[2], bDummy);
  ofstream out(argv[3], ios_base::out);

  for (size_t i = 0; i < uiTotalAus; ++i)
  {
    if (bSvc)
    {
      // check if a base layer segment was lost
      if (mLineInfo.find(i) != mLineInfo.end())
      {
        std::vector<LineInfo>& lines = mLineInfo[i];
        bool bBaseLost = false;
        for (const LineInfo& line : lines)
        {
          if (line.Layer == "BL")
            bBaseLost = true;
        }
        if (bBaseLost)
        {
          out << LAYER_FREEZE;
        }
        else
        {
          // only lost EL
          out << LAYER_AVC;
        }
      }
      else
      {
        out << LAYER_SVC;
      }
    }
    else
    {
      if (mLineInfo.find(i) != mLineInfo.end())
      {
        out << LAYER_FREEZE;
      }
      else
      {
        out << LAYER_AVC;
      }
    }
  }

  auTrace.close();
  out.close();
  return 0;
}
