#include "stdafx.h"
#include <cstdint>
#include <iostream>
#include <map>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/FileUtil.h>

const std::string sZones("zones.txt");
const std::string sConfig("rtc_client.cfg");

using namespace std;

struct Zone
{
  std::string zone_id;
  std::string location;
  std::string region;
  int zone_no;
};

struct Config
{
  std::string offerer;
  std::string answerer;
  int duration;
  int experiment_no;
  int zone_no_offerer;
  int zone_no_answerer;
};

std::string sReadTable = "<<var>> <- read.table(\"../temp/<<file>>\", header=TRUE, colClasses=c(Date=\"character\"))\n"\
"<<var>>$DT <- paste(<<var>>$Date,<<var>>$Time)\n"\
"<<var>>$DateTime <- as.POSIXct(strptime(<<var>>$DT, '%m%d %H:%M:%S.%OS'))\n";

std::string sBiDirPlot = "<<plot>> <- ggplot() + "\
    "geom_point(data = <<var1>>, aes(x=DateTime, y=<<field>>), color=\"red\", pch=3) +"\
    "geom_point(data = <<var2>>, aes(x=DateTime, y=<<field>>), color=\"blue\", pch=4)\n"\
    "ggsave(filename=\"../temp/<<file>>_bi_<<field>>.png\", plot=<<plot>>)";

void generateRForExperiment(const std::string& field,
                            std::ostringstream& oss, int ex_no,
                            int count, std::vector<std::string>& vPlots,
                            std::map<int, Zone> mRegionLookupMap,
                            std::map<uint32_t, Config> mConfigs, const std::string& sFileSuffix)
{
  // search for file with pattern "RtcServer_<<ex_no>>_*_trans.txt"
  std::ostringstream fileSignature;
  fileSignature << "RtcServer_" << ex_no << "_";
  std::vector<std::string> vFiles = FileUtil::getFileList( "./temp", fileSignature.str() );

  // search only for file ending in "_trans.txt"
  std::vector<std::string> vFiltered;
  for (auto file : vFiles)
  {
    // if (boost::algorithm::ends_with(file, "_trans.txt"))
    if (boost::algorithm::ends_with(file, sFileSuffix))
      vFiltered.push_back(file);
  }

  const std::string& region1 = mRegionLookupMap[mConfigs[ex_no].zone_no_offerer].region;
  const std::string& region2 = mRegionLookupMap[mConfigs[ex_no].zone_no_answerer].region;

  std::string regionTitle = (region1 == region2) ? region1 : region1 + " - " + region2;
  cout << "Region " << regionTitle << " ex " << ex_no << ":" << toString(vFiltered) << endl;

  assert(vFiltered.size() == 2);
  // bi-dir RTP session on one plot
  std::ostringstream title;
  title << "title" << count;
  oss << title.str() << "<-\"" << regionTitle << " "
      << mRegionLookupMap[mConfigs[ex_no].zone_no_offerer].location << "-"
      << mRegionLookupMap[mConfigs[ex_no].zone_no_answerer].location << "\"" << endl;

  std::ostringstream var1;
  var1 << "t" << count;
  std::ostringstream plot1;
  plot1 << "p" << count;
  vPlots.push_back(plot1.str());
  std::ostringstream var2;
  var2 << "t" << count + 1;
  std::ostringstream plot2;
  plot2 << "p" << count + 1;
  // read table
  std::string sCopy1 = boost::algorithm::replace_all_copy(sReadTable, "<<var>>", var1.str());
  boost::algorithm::replace_all(sCopy1, "<<file>>", vFiltered[0]);
  boost::algorithm::replace_all(sCopy1, "<<plot>>", plot1.str());
  oss << endl << sCopy1 << endl;
  // read table
  std::string sCopy2 = boost::algorithm::replace_all_copy(sReadTable, "<<var>>", var2.str());
  boost::algorithm::replace_all(sCopy2, "<<file>>", vFiltered[1]);
  boost::algorithm::replace_all(sCopy2, "<<plot>>", plot2.str());
  oss << endl << sCopy2 << endl;

  // plot
  std::string sCopyPlot = boost::algorithm::replace_all_copy(sBiDirPlot, "<<var1>>", var1.str());
  boost::algorithm::replace_all(sCopyPlot, "<<var2>>", var2.str());
  boost::algorithm::replace_all(sCopyPlot, "<<file>>", fileSignature.str());
  boost::algorithm::replace_all(sCopyPlot, "<<plot>>", plot1.str());
  boost::algorithm::replace_all(sCopyPlot, "<<field>>", field);
  oss << endl << sCopyPlot << endl;

  // add title
  oss << plot1.str() << "<-" << plot1.str() << "+ggtitle(" << title.str() << ")" << endl;
}

int main(int argc, char** argv)
{
  if (!boost::filesystem::exists(sZones))
  {
    cout << sZones << " does not exist" << endl;
    return -1;
  }
  if (!boost::filesystem::exists(sConfig))
  {
    cout << sConfig << " does not exist" << endl;
    return -1;
  }

  // map from zone_no to zone
  std::map<int, Zone> mRegionLookupMap;
  // map from region to all zone_no in region
  std::map<std::string, std::set<int>> mRegionMap;

  std::string sLine;
  fstream in1(sZones.c_str(), std::ios_base::in);
  std::vector<Zone> zones;
  while (!std::getline(in1, sLine).eof())
  {
    istringstream istr(sLine);
    Zone z;
    istr >> z.zone_id >> z.location >> z.region >> z.zone_no;
#if 0
    cout << "Zone: " << z.zone_id << " " << z.location << " " << z.region << " " << z.zone_no << endl;
#endif
    zones.push_back(z);
    mRegionLookupMap[z.zone_no] = z;
    mRegionMap[z.region].insert(z.zone_no);
  }

  fstream in2(sConfig.c_str(), std::ios_base::in);
  std::map<uint32_t, Config> mConfigs;
  while (!std::getline(in2, sLine).eof())
  {
    istringstream istr(sLine);
    Config conf;
    istr >> conf.offerer >> conf.answerer >> conf.duration >> conf.experiment_no >> conf.zone_no_offerer >> conf.zone_no_answerer;
#if 0
    cout << "Conf: " << conf.experiment_no << " " << conf.zone_no_offerer << " " << conf.zone_no_answerer<< endl;
#endif
    mConfigs[conf.experiment_no]=conf;
  }

  for (auto& pair : mRegionMap)
  {
    std::string sRegion = pair.first;
    std::set<int>& zones = pair.second;
    std::ostringstream ostr;
    ostr << "Region " << sRegion << ": [";
    for (int no : zones)
    {
      ostr << no << " (" << mRegionLookupMap[no].zone_id << ") ";
    }
    ostr << "]" << endl;
    cout << ostr.str();
  }
  // get all intra experiments

  std::map<std::string, std::set<int>> mIntraRegionExperiments;
  std::map<std::string, std::set<int>> mInterRegionExperiments;

  for (auto& pair: mConfigs)
  {
    Config& config = pair.second;
    int hostA = config.zone_no_offerer;
    int hostB = config.zone_no_answerer;
    if (mRegionLookupMap[hostA].region == mRegionLookupMap[hostB].region)
    {
      // intra region
      mIntraRegionExperiments[mRegionLookupMap[hostA].region].insert(config.experiment_no);
      cout << "Ex " << config.experiment_no << " Intra - region " << mRegionLookupMap[hostA].region << endl;
    }
    else
    {
      // inter region
      mInterRegionExperiments[mRegionLookupMap[hostA].region].insert(config.experiment_no);
      mInterRegionExperiments[mRegionLookupMap[hostB].region].insert(config.experiment_no);
      cout << "Ex " << config.experiment_no << " Inter - region " << mRegionLookupMap[hostA].region << " to " << mRegionLookupMap[hostB].region << endl;
    }
  }

  // read R template header
  std::string sHeader = FileUtil::readFile("template.R.header", false);
  const std::string sTrans("_trans.txt");
  const std::string sOwd("_owd.txt");

  // generate intra scripts in intra directory
  for (auto& pair : mIntraRegionExperiments)
  {
    // the region that all the sessions are linked too
    std::string region = pair.first;
    std::ostringstream outFile;
    outFile << "intra/" << pair.first << ".R";
    std::ostringstream oss;
    oss << sHeader << endl;
    std::vector<std::string> vPlots;
    int count = 0;

    for (int ex_no : pair.second)
    {
      generateRForExperiment("RTT", oss, ex_no, count, vPlots, mRegionLookupMap, mConfigs, sTrans);
      count += 2;
    }
    for (int ex_no : pair.second)
    {
      generateRForExperiment("Jitter", oss, ex_no, count, vPlots, mRegionLookupMap, mConfigs, sTrans);
      count += 2;
    }
    for (int ex_no : pair.second)
    {
      generateRForExperiment("Loss", oss, ex_no, count, vPlots, mRegionLookupMap, mConfigs, sTrans);
      count += 2;
    }
    for (int ex_no : pair.second)
    {
      generateRForExperiment("LossFraction", oss, ex_no, count, vPlots, mRegionLookupMap, mConfigs, sTrans);
      count += 2;
    }
    for (int ex_no : pair.second)
    {
      generateRForExperiment("Delay", oss, ex_no, count, vPlots, mRegionLookupMap, mConfigs, sOwd);
      count += 2;
    }

    int height = 240 * vPlots.size();
    oss << "png(\"../intra/RtcServer_" << region << "_intra_summary.png\",width=1800,height=" << height << ")" << endl;
    oss << "arrange(" << toString(vPlots, ',') << ",ncol=1)" << endl;
    oss << "dev.off()" << endl;

    FileUtil::writeFile(outFile.str().c_str(), oss.str(), false);
    cout << endl;
  }

  // generate inter scripts in intra directory
  for (auto& pair : mInterRegionExperiments)
  {
    // the region that all the sessions are linked too
    std::string region = pair.first;
    std::ostringstream outFile;
    outFile << "inter/" << pair.first << ".R";
    std::ostringstream oss;
    oss << sHeader << endl;
    std::vector<std::string> vPlots;
    int count = 0;

    for (int ex_no : pair.second)
    {
      generateRForExperiment("RTT", oss, ex_no, count, vPlots, mRegionLookupMap, mConfigs, sTrans);
      count += 2;
    }
    for (int ex_no : pair.second)
    {
      generateRForExperiment("Jitter", oss, ex_no, count, vPlots, mRegionLookupMap, mConfigs, sTrans);
      count += 2;
    }
    for (int ex_no : pair.second)
    {
      generateRForExperiment("Loss", oss, ex_no, count, vPlots, mRegionLookupMap, mConfigs, sTrans);
      count += 2;
    }
    for (int ex_no : pair.second)
    {
      generateRForExperiment("LossFraction", oss, ex_no, count, vPlots, mRegionLookupMap, mConfigs, sTrans);
      count += 2;
    }
    for (int ex_no : pair.second)
    {
      generateRForExperiment("Delay", oss, ex_no, count, vPlots, mRegionLookupMap, mConfigs, sOwd);
      count += 2;
    }

    int height = 240 * vPlots.size();
    oss << "png(\"../inter/RtcServer_" << region << "_inter_summary.png\",width=1800,height=" << height << ")" << endl;
    oss << "arrange(" << toString(vPlots, ',') << ",ncol=1)" << endl;
    oss << "dev.off()" << endl;

    FileUtil::writeFile(outFile.str().c_str(), oss.str(), false);
    cout << endl;
  }

  return 0;
}
