#include "CorePch.h"
#include <rtp++/rfc3261/UserAgentBase.h>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 
#include <rtp++/rfc3261/SipErrorCategory.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

UserAgentBase::UserAgentBase(boost::asio::io_service& ioService, const std::string& sFQDN, uint16_t uiSipPort)
  :m_ioService(ioService),
  m_transportLayer(ioService, boost::bind(&UserAgentBase::handleSipMessageFromTransportLayer, this, _1, _2, _3, _4), sFQDN, uiSipPort)
{

}

UserAgentBase::~UserAgentBase()
{

}

boost::system::error_code UserAgentBase::start()
{
  // start transport layer first
  boost::system::error_code ec = m_transportLayer.start();
  if (ec)
  {
    VLOG(5) << "Failed to start transport layer";
    return ec;
  }

  return doStart();
}

boost::system::error_code UserAgentBase::stop()
{
  VLOG(6) << "UserAgentBase::stop()";

  boost::system::error_code ec = doStop();
  if (ec)
  {
    VLOG(5) << "Failed doStop";
  }

  // Should this happen after the UAC/UAS send final messages?
  // stop transport layer last
  ec = m_transportLayer.stop();
  if (ec)
  {
    VLOG(5) << "Failed to stop transport layer";
  }

  return boost::system::error_code();
}

void UserAgentBase::handleSipMessageFromTransportLayer(const SipMessage& sipMessage, const std::string& sSource, const uint16_t uiPort, TransactionProtocol eProtocol)
{
  doHandleSipMessageFromTransportLayer(sipMessage, sSource, uiPort, eProtocol);
}

} // rfc3261
} // rtp_plus_plus
