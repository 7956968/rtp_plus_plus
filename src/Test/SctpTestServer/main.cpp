#include "SctpTestServerPch.h"

#include <deque>
#include <fstream>
#include <sstream>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/mutex.hpp>
#include <rtp++/application/ApplicationContext.h>
#include <rtp++/application/ApplicationUtil.h>

#include "SctpAssociation.h"

using namespace std;
using namespace rtp_plus_plus;

namespace po = boost::program_options;

int g_uiMessages = 0;


boost::mutex g_mutex;
std::deque<std::string> g_qIncomingMessages;

int g_uiOutgoingMessageId = 0;
boost::mutex g_mutexOut;
std::deque<std::string> g_qOutgoingMessages;
sctp::SctpAssociation* pSctpAssociation;

bool g_ready = false;

void sendThread()
{
  VLOG(2) << "SendThread";
  while (!pSctpAssociation->isShutdown())
  {
    if (g_ready)
    {
      LOG_FIRST_N(INFO, 1) << "Send thread READY";
      boost::optional<std::string> message;
      {
        boost::mutex::scoped_lock l(g_mutexOut);
        if (!g_qOutgoingMessages.empty())
        {
          VLOG(5) << "Queue size: " << g_qOutgoingMessages.size();
          std::string sMessage = g_qOutgoingMessages.front();
          message = boost::optional<std::string>(sMessage);
        }
      }
      if (message)
      {
        if (pSctpAssociation->sendUserMessage(1, message->c_str(), message->length()))
        {
//          VLOG(5) << "Sending: " << *message
//                  << " Result of send: ";
          boost::mutex::scoped_lock l(g_mutexOut);
          g_qOutgoingMessages.pop_front();
        }
        else
        {
          VLOG(2) << "Would block, try again";
        }
      }
      else
      {
        usleep(1000);
      }

    }
    else
    {
      LOG_EVERY_N(INFO, 5) << "Send thread not ready";
    }
  }
  VLOG(2) << "End of send thread";
}

void onSctpRecv(uint32_t uiChannelId, const NetworkPacket& networkPacket)
{
  VLOG(6) << "Received SCTP from client on channel " << uiChannelId
          << " Size: " << networkPacket.getSize()
          << " Message: " << networkPacket.data();
  ++g_uiMessages;

  std::ostringstream ostr;
  ostr << "Server " << g_uiMessages;
  std::string sMessage = ostr.str();
  //LOG(INFO) << "Sending server message: " << sMessage;
//  LOG(INFO) << "A";
  boost::mutex::scoped_lock l(g_mutex);
  g_qIncomingMessages.push_back(sMessage);
//  LOG(INFO) << "B";
  // int i = pSctpAssociation->send_user_message(0, sMessage.c_str(), sMessage.length());
  // LOG(INFO) << "Result of send: " << i;
}

boost::optional<std::string> getMessageFromQueue()
{
  boost::mutex::scoped_lock l(g_mutex);

//  LOG(INFO) << "E";
  if (!g_qIncomingMessages.empty())
  {
    std::string sMessage = g_qIncomingMessages.front();
    g_qIncomingMessages.pop_front();
    return boost::optional<std::string>(sMessage);
//    LOG(INFO) << "F1";
  }
//  LOG(INFO) << "F2";
  return boost::optional<std::string>();
}

/**
 * @brief main SCTP Test Application.
 *
 * Goals: test non-blocking communication
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv)
{
  if (!app::ApplicationContext::initialiseApplication(argv[0]))
  {
    LOG(WARNING) << "Failed to initialise application";
    return -1;
  }

  sctp::SctpOptions options;
  options.ActivateSctp = true;
  options.NonBlocking = false;

  pSctpAssociation = new sctp::SctpAssociation(5004, 5004, 49170, options);
  pSctpAssociation->setRecvCallback(boost::bind(&onSctpRecv, _1, _2));

  std::vector<std::tuple<sctp::PrPolicy, int> > vPR;
  vPR.push_back(std::make_tuple(sctp::SCTP_PR_POLICY_SCTP_RTX, 0));
  vPR.push_back(std::make_tuple(sctp::SCTP_PR_POLICY_SCTP_NONE, 0));

  boost::thread* pSendThread = NULL;
  pSctpAssociation->bind();

  if (pSctpAssociation->listen())
  {
    LOG(INFO) << "Passive host: after listen";

    if (!pSctpAssociation->setNonBlocking())
    {
      LOG(WARNING) << "Failed to set non-blocking";
    }
    else
    {
      VLOG(5) << "SCTP socket set to non-blocking";
    }

    // open more channels
#if 1
    for (int i = 0; i < vPR.size(); ++i)
    {
      sctp::channel* pChannel = pSctpAssociation->openChannel(true, std::get<0>(vPR[i]), std::get<1>(vPR[i]));
      LOG(INFO) << "Opening channel " << pChannel->id;
    }
    pSctpAssociation->print_status();

    VLOG(2) << "Starting send thread";
    pSendThread = new boost::thread(&sendThread);
#endif
  }
  else
  {
    LOG(WARNING) << "Failed to listen";
  }

  int currentPacketCount = 0;
  std::vector<int> vPacketCount;
  vPacketCount.push_back(40);
  vPacketCount.push_back(20);
  vPacketCount.push_back(30);
  vPacketCount.push_back(10);
  vPacketCount.push_back(10);
  vPacketCount.push_back(30);
  vPacketCount.push_back(10);
  while (true)
  {
#if 1
    {
//      LOG(INFO) << "C";

      static int t = 0;
      if (t == 0)
        pSctpAssociation->print_status();
      ++t;

//      LOG(INFO) << "D";
      boost::optional<std::string> message = getMessageFromQueue();

//      LOG(INFO) << "G";
      if (message)
      {
//        LOG(INFO) << "H";
        LOG_FIRST_N(INFO, 1) << "Ready to send messages";
        g_ready = true;
        //int i = pSctpAssociation->sendUserMessage(1, message->c_str(), message->length());
        //VLOG(5) << "Sending: " << *message
        //          << " Result of send: " << i;
      }
//      LOG(INFO) << "I";

      // produce messages
      std::vector<std::string> vPackets;
      int iPackets = vPacketCount[currentPacketCount];
      // VLOG(2) << "Current packets: " << iPackets;
      for (int i = 0; i < iPackets; ++i)
      {
        std::ostringstream ostr;
        ostr << "Server " << g_uiOutgoingMessageId++;
        std::string sMessage = ostr.str();
        const int uiBufSize = 10000;
        char pLargeMessage[uiBufSize];
        memset(pLargeMessage, 0, uiBufSize);
        memcpy(pLargeMessage, sMessage.c_str(), sMessage.length());
        std::string sLargeMessage(pLargeMessage, uiBufSize);
        vPackets.push_back(sLargeMessage);
      }
      currentPacketCount = (currentPacketCount + 1)%vPacketCount.size();
      //VLOG(5) << "Packet Queue size: " << vPackets.size();
      {
        //VLOG(2) << "Adding message to outgoing queue";
        boost::mutex::scoped_lock l(g_mutexOut);
        for (std::string& sLargeMessage: vPackets)
          g_qOutgoingMessages.push_back(sLargeMessage);
      }
    }
#endif

    if (pSctpAssociation->isShutdown())
      break;

    // LOG(INFO) << "Sleeping";
#if 0
    sleep(1);
#else
    usleep(10000);
#endif
  }

//  LOG(INFO) << "J";

  VLOG(2) << "Joining send thread";
  pSendThread->join();
  VLOG(2) << "Joining send thread done";

  // close all associations
  if (!pSctpAssociation->shutdown())
  {
    LOG(WARNING) << "Failed to shutdown socket";
  }
  else
  {
    VLOG(5) << "Shutdown SCTP socket";
  }

  delete pSctpAssociation;

  if (!app::ApplicationContext::cleanupApplication())
  {
    LOG(WARNING) << "Failed to cleanup application cleanly";
  }

  return 0;
}

