#include "SipProxyPch.h"
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
#include <rtp++/network/NetworkInterfaceUtil.h>
#include <rtp++/rfc3261/ResponseCodes.h>
#include <rtp++/rfc3261/Rfc3261.h>
#include <rtp++/rfc3261/SipContext.h>
#include <rtp++/rfc3261/SipMessage.h>
#include <rtp++/rfc3261/SipProxy.h>

using namespace std;
using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;
using namespace rtp_plus_plus::rfc3261;
namespace po = boost::program_options;


void registerHandler(uint32_t uiTxId, const SipMessage& sipRequest, SipMessage& sipResponse)
{
  VLOG(2) << "TODO: REGISTER received";
}

void inviteHandler(const uint32_t uiTxId, const SipMessage& invite, SipProxy& sipAgent, boost::asio::io_service& ioService)
{
  VLOG(2) << "TODO: INVITE received";
}

void inviteResponseHandler(const boost::system::error_code& ec, uint32_t uiTxId, const SipMessage& sipResponse)
{
  VLOG(2) << "TODO: INVITE response received";
}

/**
 * @brief   Sample SIP proxy application
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
    // boost::optional<std::string> sipUser = applicationParameters.getStringParameter(ApplicationParameters::sip_user);
    boost::optional<std::string> fqdn = applicationParameters.getStringParameter(ApplicationParameters::fqdn);
    boost::optional<std::string> domain = applicationParameters.getStringParameter(ApplicationParameters::domain);

    if (!domain)
    {
      LOG(WARNING) << "No domain specified";
      return -1;
    }

    std::string sFqdn;
    if (!fqdn || *fqdn == "")
    {
      std::vector<NetworkInterface> networkInterfaces = NetworkInterfaceUtil::getLocalInterfaces(true, true);
      sFqdn = networkInterfaces.at(0).Address;
    }
    else
    {
      sFqdn = *fqdn;
    }


    SipContext context("", sFqdn, *domain);

    ServiceManager serviceManager;

    boost::system::error_code ec;
    rfc3261::SipProxy sipProxy(serviceManager.getIoService(), context, ec);
    if (ec)
    {
      LOG(WARNING) << "Failed to create SIP proxy: " << ec.message();
      return -1;
    }
    // INVITE handler
    sipProxy.setOnInviteHandler(boost::bind(&inviteHandler, _1, _2, boost::ref(sipProxy), boost::ref(serviceManager.getIoService())));
    // INVITE response handler
    sipProxy.setInviteResponseHandler(boost::bind(&inviteResponseHandler, _1, _2, _3));
    // REGISTER response handler
    sipProxy.setRegisterHandler(boost::bind(&registerHandler, _1, _2, _3));

    // register media session with manager
    uint32_t uiServiceId;
    bool bSuccess = serviceManager.registerService(boost::bind(&rfc3261::SipProxy::start, boost::ref(sipProxy)),
      boost::bind(&rfc3261::SipProxy::stop, boost::ref(sipProxy)),
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

