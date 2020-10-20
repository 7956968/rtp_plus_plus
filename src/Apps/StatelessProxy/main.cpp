#include "StatelessProxyPch.h"
#include <iostream>
#include <sstream>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <cpputil/ConsoleApplicationUtil.h>
#include <cpputil/GenericParameters.h>
#include <cpputil/ServiceManager.h>
#include <rtp++/application/Application.h>
#include <rtp++/network/MediaSessionNetworkManager.h>
#include <rtp++/rfc3261/ResponseCodes.h>
#include <rtp++/rfc3261/Rfc3261.h>
#include <rtp++/rfc3261/SipContext.h>
#include <rtp++/rfc3261/SipMessage.h>
#include <rtp++/rfc3261/StatelessProxy.h>

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;
using namespace rtp_plus_plus::rfc3261;
namespace po = boost::program_options;

/**
 * @brief   Stateless SIP proxy application
 * @author  Ralf Globisch
 */int main(int argc, char** argv)
{
  if (!ApplicationContext::initialiseApplication(argv[0]))
  {
    LOG(WARNING) << "Failed to initialise application";
    return 0;
  }

  try
  {
    ApplicationConfiguration ac;
    ac.addOptions(ApplicationConfiguration::GENERIC);
    ac.addOptions(ApplicationConfiguration::SIP_PROXY);
    ac.addOptions(ApplicationConfiguration::NETWORK);

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
    boost::optional<std::string> domain = applicationParameters.getStringParameter(ApplicationParameters::domain);

    if (!domain)
    {
      LOG(WARNING) << "No domain specified";
      return -1;
    }
    SipContext context("", "", *domain);

    ServiceManager serviceManager;

    /// network manager
    MediaSessionNetworkManager mediaSessionNetworkManager(serviceManager.getIoService());

    boost::system::error_code ec;
    rfc3261::StatelessProxy sipProxy(serviceManager.getIoService(), mediaSessionNetworkManager, ac.SipProxy.SipPort, ec);
    if (ec)
    {
      LOG(WARNING) << "Failed to create stateless proxy: " << ec.message();
      return -1;
    }

    // register media session with manager
    uint32_t uiServiceId;
    bool bSuccess = serviceManager.registerService(boost::bind(&rfc3261::StatelessProxy::start, boost::ref(sipProxy)),
      boost::bind(&rfc3261::StatelessProxy::stop, boost::ref(sipProxy)),
      uiServiceId);

    if (bSuccess)
    {
      // the user can now specify the handlers for incoming samples.
      startEventLoop(boost::bind(&ServiceManager::start, boost::ref(serviceManager)),
        boost::bind(&ServiceManager::stop, boost::ref(serviceManager)));
    }
    else
    {
      LOG(WARNING) << "Failed to register service";
      return -1;
    }

    if (!ApplicationContext::cleanupApplication())
    {
      LOG(WARNING) << "Failed to cleanup application cleanly";
    }
    return 0;
  }
  catch (boost::exception& e)
  {
    LOG(ERROR) << "Exception: " << boost::diagnostic_information(e);
  }
  catch (std::exception& e)
  {
    LOG(ERROR) << "Exception: " << e.what();
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

