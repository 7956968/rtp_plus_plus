#include "SctpTestClientPch.h"

#include <fstream>
#include <sstream>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <cpputil/Conversion.h>
#include <rtp++/application/ApplicationContext.h>
#include <rtp++/application/ApplicationUtil.h>
#include "../SctpTestServer/SctpAssociation.h"

using namespace std;
using namespace rtp_plus_plus;

namespace po = boost::program_options;

int g_uiMessages = 0;
int g_uiRecvMessages = 0;

void onSctpRecv(uint32_t uiChannelId, const NetworkPacket& networkPacket)
{
  VLOG(2) << "Received SCTP from server on channel " << uiChannelId
          << " Size: " << networkPacket.getSize()
          << " Message: " << networkPacket.data();
  ++g_uiRecvMessages;
}

int main(int argc, char** argv)
{
  if (argc != 5)
  {
    LOG(WARNING) << "Usage: " << argv[1] << " <<host>> <<sctp_port>> <<local_udp_encaps_port>> <<remote_udp_encaps_port>>";
    return -1;
  }

  if (!app::ApplicationContext::initialiseApplication(argv[0]))
  {
    LOG(WARNING) << "Failed to initialise application";
    return -1;
  }

  bool bOk = false;
  const char* szHost = argv[1];
  uint16_t uiSctpPort = convert<uint16_t>(argv[2], bOk);
  if (!bOk)
  {
    LOG(WARNING) << "Invalid SCTP port: " << argv[2];
    return -1;
  }
  uint16_t uiLocalUdpPort = convert<uint16_t>(argv[3], bOk);
  if (!bOk)
  {
    LOG(WARNING) << "Invalid local UDP port: " << argv[3];
    return -1;
  }
  uint16_t uiRemoteUdpPort = convert<uint16_t>(argv[4], bOk);
  if (!bOk)
  {
    LOG(WARNING) << "Invalid remote UDP port: " << argv[4];
    return -1;
  }

  sctp::SctpAssociation* pSctpAssociation = new sctp::SctpAssociation(uiSctpPort, uiLocalUdpPort, uiRemoteUdpPort);
  pSctpAssociation->setRecvCallback(boost::bind(&onSctpRecv, _1, _2));

  pSctpAssociation->bind();

  // BLOCKING?
  EndPoint endPoint(szHost, uiRemoteUdpPort);
  if (!pSctpAssociation->connect(endPoint))
  {
    LOG(WARNING) << "Failed to connect to " << endPoint;
  }
  else
  {
#if 1
    sctp::channel* pChannel = pSctpAssociation->openChannel(true, sctp::SCTP_PR_POLICY_SCTP_NONE, 0);
    VLOG(2) << "Opening channel " << pChannel->id;
#endif
  }

  if (!pSctpAssociation->setNonBlocking())
  {
    LOG(WARNING) << "Failed to set non-blocking";
  }
  else
  {
    VLOG(5) << "SCTP socket set to non-blocking";
  }

  g_uiMessages = 0;
  while (true)
  {
    if (g_uiMessages < 100)
    // if (g_uiMessages == 0)
    {
      if (g_uiMessages == 25)
      {
        int res = 1;
        int uiMessage = 0;
        while (res)
        {
          std::ostringstream ostr;
          ostr << "Client " << g_uiMessages << " " << uiMessage++;
          std::string sMessage = ostr.str();
          const int uiBufSize = 10000;
          char pLargeMessage[uiBufSize];
          memset(pLargeMessage, 0, uiBufSize);
          memcpy(pLargeMessage, sMessage.c_str(), sMessage.length());
          res = pSctpAssociation->sendUserMessage(0, pLargeMessage, uiBufSize);
          VLOG(5) << "Sending message " << g_uiMessages << " Message: " << sMessage
                  << " Result of send: " << res;
        }
      }
      else
      {
        std::ostringstream ostr;
        ostr << "Client " << g_uiMessages;
        std::string sMessage = ostr.str();
        int res = pSctpAssociation->sendUserMessage(0, sMessage.c_str(), sMessage.length());
        LOG(INFO) << "Sending message " << g_uiMessages << " Message: " << sMessage
                  << " Result of send: " << res;
      }
    }

    if (g_uiRecvMessages == 100)
      break;

#if 0
    sleep(1);
#else
    usleep(10000);
#endif
    // LOG(INFO) << "Done Sleeping";
    ++g_uiMessages;
  }

  // without this sleep, the server doesn't seem to get the shutdown signal
  sleep(1);
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

