#include "RtpProxyPch.h"
#include <string>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/FileUtil.h>
#include <cpputil/MakeService.h>
#include <rtp++/network/AddressDescriptorParser.h>
#include <rtp++/network/RtpForwarder.h>
#include <rtp++/application/Application.h>
#include <rtp++/util/TracesUtil.h>

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;

namespace po = boost::program_options;

/**
 * @brief The RtpProxy is an application that sits between the RtpSender and RtpReceiver applications
 * and tests the conformance of the RTP implementation according to RFC.
 */
int main(int argc, char** argv)
{
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);

  ApplicationUtil::printApplicationInfo(argc, argv);

  try
  {
    ApplicationConfiguration ac;
    ac.addOptions(ApplicationConfiguration::GENERIC);
    ac.addOptions(ApplicationConfiguration::RTPPROXY);

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

    // process interface and fwd conf if available
    // local
    InterfaceDescriptions_t interfaceDescriptions;
    // host 1
    InterfaceDescriptions_t interfaceDescriptions1;
    // host 2
    InterfaceDescriptions_t interfaceDescriptions2;
    typedef std::tuple<uint32_t, uint32_t, uint32_t> NetworkProperties_t;
    std::vector<NetworkProperties_t> networkProps;

    const uint32_t DEFAULT_LOSS = 0;
    const uint32_t DEFAULT_OWD = 0;
    const uint32_t DEFAULT_JITTER = 0;

    if (!ac.RtpProxy.InterfaceConf.empty() && !ac.RtpProxy.FwdConf.empty())
    {
      for (size_t i = 0; i < ac.RtpProxy.InterfaceConf.size(); ++i)
      {
        std::string& sInterface = ac.RtpProxy.InterfaceConf[i];
        boost::optional<InterfaceDescriptions_t> localInterfaceDescriptions = AddressDescriptorParser::parseInterfaceDescriptionShort(sInterface);
        if (!localInterfaceDescriptions)
        {
          LOG(WARNING) << "Invalid interface description: " << sInterface << " Should be <<address>>:<<rtp_port>>:<<rtcp_port>>";
          return -1;
        }
        else
        {
          interfaceDescriptions.push_back(localInterfaceDescriptions->at(0));
        }
      }

      // the fwd interface is of the form address::rtp_port:rtcp_port,address::rtp_port:rtcp_port
      for (size_t i = 0; i < ac.RtpProxy.FwdConf.size(); ++i)
      {
        std::string& sFwd = ac.RtpProxy.FwdConf[i];
        VLOG(2) << "Processing forwarding pair: " << sFwd;
        boost::optional<InterfaceDescriptions_t> fwdInterfaceDescriptions = AddressDescriptorParser::parseInterfaceDescriptionShort(sFwd);
        if (!fwdInterfaceDescriptions)
        {
          LOG(WARNING) << "Invalid fwd interface descriptions: " << sFwd << " Should be <<address>>:<<rtp_port>>:<<rtcp_port>>,<<address>>:<<rtp_port>>:<<rtcp_port>>";
          return -1;
        }
        else
        {
          if (fwdInterfaceDescriptions->size() != 2)
          {
            LOG(WARNING) << "Invalid fwd interface descriptions: " << sFwd << " Should be <<address>>:<<rtp_port>>:<<rtcp_port>>,<<address>>:<<rtp_port>>:<<rtcp_port>>";
            return -1;
          }
          else
          {
            interfaceDescriptions1.push_back(fwdInterfaceDescriptions->at(0));
            interfaceDescriptions2.push_back(fwdInterfaceDescriptions->at(1));
            // we need to simultaneously store path chars
            uint32_t uiLoss = (i < ac.RtpProxy.LossProbability.size()) ? ac.RtpProxy.LossProbability[i] : DEFAULT_LOSS;
            uint32_t uiOwd = (i < ac.RtpProxy.OwdMs.size()) ? ac.RtpProxy.OwdMs[i] : DEFAULT_OWD;
            uint32_t uiJitter = (i < ac.RtpProxy.JitterMs.size()) ? ac.RtpProxy.JitterMs[i] : DEFAULT_JITTER;
            networkProps.push_back(std::make_tuple(uiLoss, uiOwd, uiJitter));
          }
        }
      }
    }
    else
    {
      // read address descriptor of local configuration
      std::string sLocalCfg = FileUtil::readFile(ac.RtpProxy.LocalConf, false);
      // we need to parse this address descriptor
      boost::optional<InterfaceDescriptions_t> localConf = AddressDescriptorParser::parse(sLocalCfg);
      if (!localConf)
      {
        LOG(ERROR) << "Failed to read local configuration";
        return -1;
      }
      else
      {
        interfaceDescriptions = *localConf;
      }

      // read address descriptor of receiver configuration
      std::string sReceiverCfg = FileUtil::readFile(ac.RtpProxy.ReceiverConf, false);
      // we need to parse this address descriptor
      boost::optional<InterfaceDescriptions_t> receiverConf = AddressDescriptorParser::parse(sReceiverCfg);
      if (!receiverConf)
      {
        LOG(ERROR) << "Failed to read local configuration";
        return -1;
      }
      else
      {
        interfaceDescriptions1 = *receiverConf;
      }

      // read address descriptor of sender configuration
      std::string sSenderCfg = FileUtil::readFile(ac.RtpProxy.SenderConf, false);
      // we need to parse this address descriptor
      boost::optional<InterfaceDescriptions_t> senderConf = AddressDescriptorParser::parse(sSenderCfg);
      if (!senderConf)
      {
        LOG(ERROR) << "Failed to read sender configuration";
        return -1;
      }
      else
      {
        interfaceDescriptions2 = *senderConf;
      }

      uint32_t uiLoss = (!ac.RtpProxy.LossProbability.empty()) ? ac.RtpProxy.LossProbability[0] : DEFAULT_LOSS;
      uint32_t uiOwd = (!ac.RtpProxy.OwdMs.empty()) ? ac.RtpProxy.OwdMs[0] : DEFAULT_OWD;
      uint32_t uiJitter = (!ac.RtpProxy.JitterMs.empty()) ? ac.RtpProxy.JitterMs[0] : DEFAULT_JITTER;
      networkProps.push_back(std::make_tuple(uiLoss, uiOwd, uiJitter));
    }

    boost::asio::io_service io_service;


    boost::shared_ptr<RtpForwarder> pRtpForwarder;

    if (ac.RtpProxy.ChannelConfigFile.empty())
    {
      uint32_t uiLoss = (!ac.RtpProxy.LossProbability.empty()) ? ac.RtpProxy.LossProbability[0] : DEFAULT_LOSS;
      uint32_t uiOwd = (!ac.RtpProxy.OwdMs.empty()) ? ac.RtpProxy.OwdMs[0] : DEFAULT_OWD;
      uint32_t uiJitter = (!ac.RtpProxy.JitterMs.empty()) ? ac.RtpProxy.JitterMs[0] : DEFAULT_JITTER;
      VLOG(2) << "Creating statistical jitter-based forwarding with "
              << " Loss: " << uiLoss
              << " OWD: " << uiOwd
                 << " Jitter: " << uiJitter;

      pRtpForwarder = RtpForwarder::create(io_service,
                                           interfaceDescriptions, interfaceDescriptions1, interfaceDescriptions2,
                                           networkProps,
                                           ac.RtpProxy.Strict);
    }
    else
    {
      std::vector<double> vDelays;
      bool bSuccess = TracesUtil::loadOneWayDelayDataFile(ac.RtpProxy.ChannelConfigFile, vDelays);
      if (!bSuccess)
      {
        LOG(WARNING) << "Failed to load delay trace vector";
        return -1;
      }
      else
      {
        VLOG(2) << "Loaded delay trace vector of size " << vDelays.size();
      }

      uint32_t uiLoss = (!ac.RtpProxy.LossProbability.empty()) ? ac.RtpProxy.LossProbability[0] : DEFAULT_LOSS;

      pRtpForwarder = RtpForwarder::create(io_service,
                                           interfaceDescriptions, interfaceDescriptions1, interfaceDescriptions2,
                                           uiLoss, vDelays,
                                           ac.RtpProxy.Strict);
    }

    startEventLoop(boost::bind(&boost::asio::io_service::run, boost::ref(io_service)),
                   boost::bind(&boost::asio::io_service::stop, boost::ref(io_service)));
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
