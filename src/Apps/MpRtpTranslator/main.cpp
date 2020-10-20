#include "MpRtpTranslatorPch.h"
#include <sstream>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/GenericParameters.h>
#include <rtp++/mprtp/MpRtpTranslator.h>
#include <rtp++/application/Application.h>

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;

namespace po = boost::program_options;

/**
 * @brief main This application functions as a translator between an MPRTP agent and an RTP agent.
 * It is assumed that the session descriptions of the participants are compatible
 * It creates 2 RTP sessions: one MPRTP session and one standard RTP session.
 * Incoming RTP packets are sent in the MPRTP session over multiple paths.
 * Incoming MPRTP packets are sent the RTP session.
 * Each session maintains it's own RTCP.
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv)
{
  if (!ApplicationContext::initialiseApplication(argv[0]))
  {
    LOG(WARNING) << "Failed to initialise application";
    return 0;
  }

  // read local SDP
  try
  {
    ApplicationConfiguration ac;
    ac.addOptions(ApplicationConfiguration::GENERIC);
    ac.addOptions(ApplicationConfiguration::SESSION);
    ac.addOptions(ApplicationConfiguration::SENDER);
    ac.addOptions(ApplicationConfiguration::RECEIVER);
    ac.addOptions(ApplicationConfiguration::VIDEO);
    ac.addOptions(ApplicationConfiguration::AUDIO);
    ac.addOptions(ApplicationConfiguration::MULTIPATH);

    po::options_description cmdline_options = ac.generateOptions();

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(cmdline_options).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
      ostringstream ostr;
      ostr << cmdline_options;
      LOG(ERROR) << ostr.str();
      return 1;
    }

    if (ac.Application.logfile != "")
    {
      // update the log file: we want to be able to parse this file
      google::SetLogDestination(google::GLOG_INFO, (ac.Application.logfile + ".INFO").c_str());
      google::SetLogDestination(google::GLOG_WARNING, (ac.Application.logfile + ".WARNING").c_str());
      google::SetLogDestination(google::GLOG_ERROR, (ac.Application.logfile + ".ERROR").c_str());
    }

    ApplicationUtil::printApplicationInfo(argc, argv);

    // generic parameters
    GenericParameters applicationParameters = ac.getGenericParameters();

    // Parse session descriptions
    std::string sSdp1 = ac.Session.sessionDescriptions.at(0);
    std::string sSdp2 = ac.Session.sessionDescriptions.at(1);

    boost::optional<rfc4566::SessionDescription> sdp1 = ApplicationUtil::readSdp(sSdp1);
    if (!sdp1) { return -1; }

    boost::optional<rfc4566::SessionDescription> sdp2 = ApplicationUtil::readSdp(sSdp2);
    if (!sdp2) { return -1; }

    std::string sNetConf1 = ac.Session.NetworkConf.at(0);
    std::string sNetConf2 = ac.Session.NetworkConf.at(1);
    boost::optional<InterfaceDescriptions_t> netConf1 = ApplicationUtil::readNetworkConfigurationFile(sNetConf1, false);
    if (!netConf1) { return -1; }

    boost::optional<InterfaceDescriptions_t> netConf2 = ApplicationUtil::readNetworkConfigurationFile(sNetConf2, false);
    if (!netConf2) { return -1; }

    boost::shared_ptr<mprtp::MpRtpTranslator> pTranslator = mprtp::MpRtpTranslator::create(applicationParameters, *sdp1, *sdp2, *netConf1, *netConf2);

    if (pTranslator)
    {
      // the user can now specify the handlers for incoming samples.
      startEventLoop(boost::bind(&mprtp::MpRtpTranslator::start, pTranslator),
                     boost::bind(&mprtp::MpRtpTranslator::stop, pTranslator));
    }
    else
    {
      LOG(ERROR) << "Failed to create MPRTP to RTP translator";
    }

    if (!ApplicationContext::cleanupApplication())
    {
      LOG(WARNING) << "Failed to cleanup application cleanly";
    }

    return 0;
  }
  catch (boost::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << boost::diagnostic_information(e);
  }
  catch (std::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << e.what();
  }
  catch (...)
  {
    LOG(ERROR) << "Unknown exception!!!";
  }

  if (!ApplicationContext::cleanupApplication())
  {
    LOG(WARNING) << "Failed to cleanup application cleanly";
  }

  return 0;
}
