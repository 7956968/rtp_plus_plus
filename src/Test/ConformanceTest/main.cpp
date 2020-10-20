#include "ConformanceTestPch.h"
#include <string>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/FileUtil.h>
#include <cpputil/MakeService.h>
#include <rtp++/network/AddressDescriptorParser.h>
#include <rtp++/network/RtpForwarder.h>

using namespace std;
using namespace rtp_plus_plus;

namespace po = boost::program_options;

/**
  * The conformance tester is an application that sits between the RtpSender and RtpReceiver applications
  * and tests the conformance of the RTP implementation according to RFC.
  */
int main(int argc, char** argv)
{
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);
  boost::filesystem::path app = boost::filesystem::system_complete(argv[0]);
  std::string logfile = app.filename().string();
  boost::filesystem::path logpath = app.parent_path() / app.filename();

  std::ostringstream ostr;
  for (int i = 0; i < argc; ++i)
      ostr << argv[i] << " ";
  LOG(INFO) << ostr.str();

  try
  {
      // Store local onfiguration
      std::string sLocalConf;
      // Store receiver onfiguration
      std::string sReceiverConf;
      // SDP for remote media session
      std::string sSenderConf;
      // loss probability
      uint32_t uiLoss = 0;
      // jitter in ms
      uint32_t uiJitterMs = 0;

      // check program configuration
      // Declare the supported options.
      po::options_description desc("Allowed options");
      desc.add_options()
        ("help", "produce help message")
        ("r-nc", po::value<std::string>(&sReceiverConf)->default_value(""), "Receiver network configuration")
        ("l-nc", po::value<std::string>(&sLocalConf)->default_value(""), "Local network configuration")
        ("s-nc", po::value<std::string>(&sSenderConf)->default_value(""), "Sender network configuration")
        ("loss", po::value<uint32_t>(&uiLoss)->default_value(0), "Loss probability")
        ("jitter-ms", po::value<uint32_t>(&uiJitterMs)->default_value(0), "Simulated jitter in ms")
        ;

      po::positional_options_description p;
      p.add("l-nc",  0);

      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv).
          options(desc).positional(p).run(), vm);
      po::notify(vm);

      if (vm.count("help"))
      {
        ostringstream ostr;
        ostr << desc;
        LOG(WARNING) << ostr.str();
        return 1;
      }

      LOG(INFO) << "Starting RTP conformance application";

      // read address descriptor of local configuration
      std::string sLocalCfg = FileUtil::readFile(sLocalConf, false);
      // we need to parse this address descriptor
      boost::optional<InterfaceDescriptions_t> localConf = AddressDescriptorParser::parse(sLocalCfg);
      if (!localConf)
      {
          LOG(ERROR) << "Failed to read local configuration";
          return -1;
      }

      // read address descriptor of receiver configuration
      std::string sReceiverCfg = FileUtil::readFile(sReceiverConf, false);
      // we need to parse this address descriptor
      boost::optional<InterfaceDescriptions_t> receiverConf = AddressDescriptorParser::parse(sReceiverCfg);
      if (!localConf)
      {
          LOG(ERROR) << "Failed to read local configuration";
          return -1;
      }

      // read address descriptor of sender configuration
      std::string sSenderCfg = FileUtil::readFile(sSenderConf, false);
      // we need to parse this address descriptor
      boost::optional<InterfaceDescriptions_t> senderConf = AddressDescriptorParser::parse(sSenderCfg);
      if (!senderConf)
      {
          LOG(ERROR) << "Failed to read sender configuration";
          return -1;
      }

      boost::asio::io_service io_service;

      boost::shared_ptr<RtpForwarder> pRtpForwarder = RtpForwarder::create(io_service, *localConf, *receiverConf, *senderConf);
      startEventLoop(boost::bind(&boost::asio::io_service::run, boost::ref(io_service)), boost::bind(&boost::asio::io_service::stop, boost::ref(io_service)));
  }
  catch (boost::exception& e)
  {
    LOG(ERROR) << "Exception:" << boost::diagnostic_information(e);
  }
  catch (std::exception& e)
  {
    LOG(ERROR) << "Std Exception:" << e.what();
  }
  catch (...)
  {
    LOG(ERROR) << "Unknown Exception!!!";
  }
  return 0;
}
