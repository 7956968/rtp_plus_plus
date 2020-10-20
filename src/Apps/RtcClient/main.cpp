#include "RtcClientPch.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <tuple>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <grpc/grpc.h>
#include <grpc++/channel_arguments.h>
#include <grpc++/create_channel.h>
#include <grpc++/credentials.h>
#include <rtp++/application/Application.h>
#include <rtp++/application/ApplicationConfiguration.h>

#include "RtcClient.h"
#include "RtcClientConfig.h"

using namespace rtp_plus_plus;
using namespace rtp_plus_plus::app;
namespace po = boost::program_options;
namespace bfs = boost::filesystem;

using grpc::ChannelArguments;

void menu()
{
  LOG(INFO) << "Select option: " << std::endl
            << "1: create offer" << std::endl
            << "2: create answer" << std::endl
            << "3: start streaming" << std::endl
            << "4: stop streaming" << std::endl
            << "5: shutdown" << std::endl
            << "6: run experiment" << std::endl
            << "x: exit";
}

// offerer, answerer, duration_s, rtc client offerer, rtc client answerer, complete, deadline timer
typedef std::tuple<std::string, std::string, uint32_t, RtcClient*, RtcClient*, bool, boost::asio::deadline_timer*> Experiment_t;

void handleTimeout(const boost::system::error_code& ec, Experiment_t& ex)
{
  VLOG(2) << "Handling timeout: " << ec.message();
  if (!ec)
  {
    RtcClient* pRtcClientOfferer = std::get<3>(ex);
    RtcClient* pRtcClientAnswerer = std::get<4>(ex);

    assert(pRtcClientOfferer);
    assert(pRtcClientAnswerer);

    if (pRtcClientOfferer->stopStreaming())
    {
      LOG(INFO) << "Stopped offerer streaming";
    }
    else
    {
      LOG(WARNING) << "Failed to stop offerer streaming";
    }
    if (pRtcClientAnswerer->stopStreaming())
    {
      LOG(INFO) << "Stopped answerer streaming";
    }
    else
    {
      LOG(WARNING) << "Failed to stop answerer streaming";
    }
    if (!pRtcClientOfferer->shutdownRemoteRtcServer())
    {
      LOG(INFO) << "Failed to shutdown remote offerer";
    }
    if (!pRtcClientAnswerer->shutdownRemoteRtcServer())
    {
      LOG(INFO) << "Failed to shutdown remote answerer";
    }
  }
  else
  {
    // Timer got cancelled
    LOG(INFO) << "Timer cancelled: " << ec.message();
  }
  // mark complete
  std::get<5>(ex) = true;
}

enum PeerState
{
  PS_NEW,
  PS_RECEIVED_OFFER,
  PS_RECEIVED_ANSWER,
  PS_REMOTE_DESCRIPTIONS_SET,
  PS_STREAMING,
  PS_COMPLETE,
  PS_ERROR
};

std::vector<Experiment_t> readConfig(const std::string& sConfigFile)
{
  std::vector<Experiment_t> vExperiments;
  std::ifstream in(sConfigFile.c_str(), std::ios_base::in);
  std::string sLine;
  std::string sOfferer;
  std::string sAnswerer;
  uint32_t uiDurationS;
  while (!std::getline(in, sLine).eof())
  {
    if (boost::algorithm::starts_with(sLine, "#"))
    {
      // comment line
      continue;
    }
    std::istringstream istr(sLine);
    istr >> sOfferer >> sAnswerer >> uiDurationS;
    LOG(INFO) << "Read offerer: " << sOfferer << " answerer: " << sAnswerer << " duration: " << uiDurationS << "s";
    vExperiments.push_back(std::make_tuple(sOfferer, sAnswerer, uiDurationS, nullptr, nullptr, false, nullptr));
  }
  return vExperiments;
}

bool startSession(Experiment_t& ex, boost::asio::io_service& ioService)
{
  RtcClient* pRtcClientOfferer = nullptr;
  RtcClient* pRtcClientAnswerer = nullptr;
  std::string sOfferer = std::get<0>(ex);
  std::string sAnswerer = std::get<1>(ex);
  uint32_t uiDurationS = std::get<2>(ex);

  LOG(INFO) << "Starting experiment - offerer: " << sOfferer << " answerer: " << sAnswerer << " dur: " << uiDurationS << "s";
  pRtcClientOfferer = new RtcClient( grpc::CreateChannel(sOfferer, grpc::InsecureCredentials(), ChannelArguments()));
  pRtcClientAnswerer = new RtcClient( grpc::CreateChannel(sAnswerer, grpc::InsecureCredentials(), ChannelArguments()));
  std::get<3>(ex) = pRtcClientOfferer;
  std::get<4>(ex) = pRtcClientAnswerer;

  // get offer
  boost::optional<std::string> offer = pRtcClientOfferer->createOffer("application/sdp");
  if (offer)
  {
    LOG(INFO) << "Got offer from server: " << std::endl << *offer;
  }
  else
  {
    LOG(WARNING) << "Failed to get offer from server.";
    return false;
  }

  boost::optional<std::string> answer = pRtcClientAnswerer->createAnswer(*offer);
  if (answer)
  {
    LOG(INFO) << "Got answer from server: " << std::endl << *answer;
    pRtcClientOfferer->setRemoteDescription(*answer);
  }
  else
  {
    LOG(WARNING) << "Failed to get answer from server.";
    return false;
  }

  if (pRtcClientOfferer->startStreaming())
  {
    LOG(INFO) << "Started offerer streaming";
    if (pRtcClientAnswerer->startStreaming())
    {
      LOG(INFO) << "Started answerer streaming";
    }
    else
    {
      LOG(WARNING) << "Failed to start answerer streaming";
      return false;
    }
  }
  else
  {
    LOG(WARNING) << "Failed to start offerer streaming";
    return false;
  }

  boost::asio::deadline_timer* pTimer = new boost::asio::deadline_timer(ioService);
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  pTimer->expires_at(tNow + boost::posix_time::seconds(uiDurationS));
  pTimer->async_wait(boost::bind(&handleTimeout, _1, boost::ref(ex)));
  std::get<6>(ex) = pTimer;
}

bool endSession(Experiment_t& ex)
{
  RtcClient* pRtcClientOfferer = nullptr;
  RtcClient* pRtcClientAnswerer = nullptr;
  std::string sOfferer = std::get<0>(ex);
  std::string sAnswerer = std::get<1>(ex);

  LOG(INFO) << "Ending experiment - offerer: " << sOfferer << " answerer: " << sAnswerer;
  pRtcClientOfferer = new RtcClient( grpc::CreateChannel(sOfferer, grpc::InsecureCredentials(), ChannelArguments()));
  pRtcClientAnswerer = new RtcClient( grpc::CreateChannel(sAnswerer, grpc::InsecureCredentials(), ChannelArguments()));
  std::get<3>(ex) = pRtcClientOfferer;
  std::get<4>(ex) = pRtcClientAnswerer;

  if (!pRtcClientOfferer->stopStreaming())
  {
    LOG(WARNING) << "Failed to stop offerer streaming";
  }
  if (!pRtcClientAnswerer->stopStreaming())
  {
    LOG(WARNING) << "Failed to stop offerer streaming";
  }
  if (!pRtcClientOfferer->shutdownRemoteRtcServer())
  {
    LOG(WARNING) << "Failed to shutdown offerer";
  }
  if (!pRtcClientAnswerer->shutdownRemoteRtcServer())
  {
    LOG(WARNING) << "Failed to shutdown answerer";
  }
}

int main(int argc, char** argv)
{
  if (!ApplicationContext::initialiseApplication(argv[0]))
  {
    LOG(WARNING) << "Failed to initialise application";
    return 0;
  }

  // start asio thread
  boost::asio::io_service ioService;
  boost::asio::io_service::work* pWork = new boost::asio::io_service::work(ioService);
  boost::thread* pIoServiceThread = new boost::thread(boost::bind(&boost::asio::io_service::run, boost::ref(ioService)));
  std::vector<Experiment_t> vExperiments;

  try
  {
    PeerState state = PS_NEW;

    RtcClientConfigOptions config;
    po::options_description cmdline_options = config.generateOptions();

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(cmdline_options).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
      std::ostringstream ostr;
      ostr << cmdline_options;
      LOG(ERROR) << ostr.str();
      return 1;
    }

    if (config.RtcParameters.logfile != "")
    {
      // update the log file: we want to be able to parse this file
      google::SetLogDestination(google::GLOG_INFO, (config.RtcParameters.logfile + ".INFO").c_str());
      google::SetLogDestination(google::GLOG_WARNING, (config.RtcParameters.logfile + ".WARNING").c_str());
      google::SetLogDestination(google::GLOG_ERROR, (config.RtcParameters.logfile + ".ERROR").c_str());
    }

    ApplicationUtil::printApplicationInfo(argc, argv);

    grpc_init();

    // first check if this is a single automated experiment, or a menu-driven one
    // handle config file
    if (!config.RtcParameters.Config.empty())
    {
      LOG(INFO) << "Starting RTC servers from config string: " << config.RtcParameters.Config;
      // check correctness of string: must be of the form: 54.152.145.175:50000 54.88.114.255:50000 60

      if (!bfs::exists(config.RtcParameters.Config))
      {
        LOG(WARNING) << "Experimental config file does not exist: " << config.RtcParameters.Config;
        return -1;
      }

      std::vector<Experiment_t> vExperimentsInCfg = readConfig(config.RtcParameters.Config);

      for (Experiment_t& ex : vExperimentsInCfg)
      {
        if (!config.RtcParameters.KillSession)
        {
          startSession(ex, ioService);
          vExperiments.push_back(ex);
        }
        else
        {
          endSession(ex);
        }
      }

      // wait for timers to finish before ending application
      LOG(INFO) << "Deleting work";
      // cleanup
      delete pWork;
      LOG(INFO) << "Joining io service thread";
      pIoServiceThread->join();
      LOG(INFO) << "Io service thread complete";
      delete pIoServiceThread;

      LOG(INFO) << "Shutting down grpc";
      grpc_shutdown();
      return 0;
    }

    // handle config string
    if (!config.RtcParameters.ConfigString.empty())
    {
      std::string sOfferer;
      std::string sAnswerer;
      uint32_t uiDurationS;
      LOG(INFO) << "Starting RTC servers from config string: " << config.RtcParameters.ConfigString;
      // check correctness of string: must be of the form: 54.152.145.175:50000 54.88.114.255:50000 60
      std::istringstream istr(config.RtcParameters.ConfigString);
      istr >> sOfferer >> sAnswerer >> uiDurationS;
      LOG(INFO) << "Read offerer: " << sOfferer << " answerer: " << sAnswerer << " duration: " << uiDurationS << "s";
      Experiment_t ex = std::make_tuple(sOfferer, sAnswerer, uiDurationS, nullptr, nullptr, false, nullptr);

      if (!config.RtcParameters.KillSession)
      {
        startSession(ex, ioService);
        vExperiments.push_back(ex);
      }
      else
      {
        endSession(ex);
      }
      // wait for timers to finish before ending application
      LOG(INFO) << "Deleting work";
      // cleanup
      delete pWork;
      LOG(INFO) << "Joining io service thread";
      pIoServiceThread->join();
      LOG(INFO) << "Io service thread complete";
      delete pIoServiceThread;

      LOG(INFO) << "Shutting down grpc";
      grpc_shutdown();
      return 0;
    }

    RtcClient rtcClientOfferer( grpc::CreateChannel(config.RtcParameters.RtcOfferer, grpc::InsecureCredentials(), ChannelArguments()));
    RtcClient* pRtcClientAnswerer = nullptr;
    if (!config.RtcParameters.RtcAnswerer.empty())
    {
      pRtcClientAnswerer = new RtcClient( grpc::CreateChannel(config.RtcParameters.RtcAnswerer, grpc::InsecureCredentials(), ChannelArguments()));
    }

    boost::optional<std::string> offer;
    boost::optional<std::string> answer;

    bool bComplete = false;
    menu();
    char c;
    std::cin >> c;
    while (!bComplete && c != 'x')
    {
      switch (c)
      {
        case '1':
        {
          offer = rtcClientOfferer.createOffer("application/sdp");
          if (offer)
          {
            LOG(INFO) << "Got offer from server: " << std::endl << *offer;
            state = PS_RECEIVED_OFFER;
          }
          break;
        }
        case '2':
        {
          if (state == PS_RECEIVED_OFFER)
          {
            assert(offer);
            answer = pRtcClientAnswerer->createAnswer(*offer);
            if (answer)
            {
              LOG(INFO) << "Got answer from server: " << std::endl << *answer;
              state = PS_RECEIVED_ANSWER;
              rtcClientOfferer.setRemoteDescription(*answer);
              state = PS_REMOTE_DESCRIPTIONS_SET;
            }
          }
          else
          {
            LOG(INFO) << "Invalid state for create answer";
          }
          break;
        }
        case '3':
        {
          if (state == PS_REMOTE_DESCRIPTIONS_SET)
          {
            if (rtcClientOfferer.startStreaming())
            {
              LOG(INFO) << "Started offerer streaming";
              if (pRtcClientAnswerer->startStreaming())
              {
                LOG(INFO) << "Started answerer streaming";
                state = PS_STREAMING;
              }
              else
              {
                LOG(WARNING) << "Failed to start answerer streaming";
              }
            }
            else
            {
              LOG(WARNING) << "Failed to start offerer streaming";
            }
          }
          else
          {
            LOG(INFO) << "Invalid state for start streaming";
          }
          break;
        }
        case '4':
        {
          // RG: commenting out to stop servers after RtcClient crash
          // if (state == PS_STREAMING)
          {
            if (rtcClientOfferer.stopStreaming())
            {
              LOG(INFO) << "Stopped offerer streaming";
            }
            else
            {
              LOG(WARNING) << "Failed to stop offerer streaming";
            }
            if (pRtcClientAnswerer->stopStreaming())
            {
              LOG(INFO) << "Stopped answerer streaming";
            }
            else
            {
              LOG(WARNING) << "Failed to stop answerer streaming";
            }
            state = PS_COMPLETE;
          }
//          else
//          {
//            LOG(INFO) << "Invalid state for stop streaming";
//          }
          break;
        }
        case '5':
        {
          if (!rtcClientOfferer.shutdownRemoteRtcServer())
          {
            LOG(INFO) << "Failed to shutdown remote offerer";
          }
          if (!pRtcClientAnswerer->shutdownRemoteRtcServer())
          {
            LOG(INFO) << "Failed to shutdown remote answerer";
          }
          bComplete = true;
          break;
        }
        case '6':
        {
          // read config file which stores experiments
          LOG(INFO) << "Enter name of configuration file";
          std::string sConfigFile;
          std::cin >> sConfigFile;
          if (!bfs::exists(sConfigFile))
          {
            LOG(WARNING) << "Experimental config file does not exist: " << sConfigFile;
            break;
          }

          std::vector<Experiment_t> vExperimentsInCfg = readConfig(sConfigFile);

          for (Experiment_t& ex : vExperimentsInCfg)
          {
            startSession(ex, ioService);
            // add to global list
            vExperiments.push_back(ex);
          }

          break;
        }
      }

      if (!bComplete)
      {
        menu();
        std::cin >> c;
      }
      else
      {
        break;
      }
    }

    LOG(INFO) << "Shutting down grpc";
    grpc_shutdown();

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

  LOG(INFO) << "Deleting work";
  // cleanup
  delete pWork;
  // cancel outstanding callbacks
  for (Experiment_t& ex: vExperiments)
  {
    if (!std::get<5>(ex))
    {
      boost::asio::deadline_timer* pTimer = std::get<6>(ex);
      if (pTimer)
      {
        LOG(INFO) << "Canceling timer";
        pTimer->cancel();
      }
      else
      {
        LOG(INFO) << "Timer null";
      }
    }
    else
    {
      LOG(INFO) << "Experiment already complete";
    }
  }

  LOG(INFO) << "Joining io service thread";
  pIoServiceThread->join();
  LOG(INFO) << "Io service thread complete";
  delete pIoServiceThread;
  return 0;
}
