#include "CorePch.h"
#include <rtp++/rfc3261/Registrar.h>
#include <boost/bind.hpp>

namespace rtp_plus_plus
{
namespace rfc3261
{

Registrar::Registrar(boost::asio::io_service& ioService)
  :m_ioService(ioService)
{

}

Registrar::~Registrar()
{
  // cancel outstanding timers to clean up memory
  for (auto pair : m_mContacts)
  {
    pair.second.second->cancel();
  }
}

bool Registrar::registerSipAgent(const std::string& sAddressOfRecord, const std::string& sResponsible, const std::string& sContact, uint32_t uiExpiresSeconds)
{
  boost::asio::deadline_timer* pTimer = new boost::asio::deadline_timer(m_ioService);
#if 1
  // for testing
  uiExpiresSeconds = 120;
#endif
  pTimer->expires_from_now(boost::posix_time::seconds(uiExpiresSeconds));
  pTimer->async_wait(boost::bind(&Registrar::timeoutHandler, this, _1, sAddressOfRecord, pTimer));

  auto it = m_mContacts.find(sAddressOfRecord);
  if (it != m_mContacts.end())
  {
    // examine value of uiExpires
    if (uiExpiresSeconds == 0)
    {
      // the contact is being deregistered.
      it->second.second->cancel();
      remove(sAddressOfRecord);
    }
    else
    {
      // already exists, cancel timer and start new one
      it->second.second->cancel();
      it->second.second = pTimer;
    }
  }
  else
  {
    ContactRecord contact(sAddressOfRecord, sResponsible, sContact);
    m_mContacts[sAddressOfRecord] = std::make_pair(contact, pTimer);
  }
  return true;
}

boost::optional<ContactRecord> Registrar::find(const std::string& sAddressOfRecord)
{
  auto it = m_mContacts.find(sAddressOfRecord);
  if (it != m_mContacts.end())
  {
    VLOG(2) << "Found record for " << sAddressOfRecord;
    return boost::optional<ContactRecord>(it->second.first);
  }
  VLOG(2) << "Unable to locate record for " << sAddressOfRecord;
  return boost::optional<ContactRecord>();
}

void Registrar::timeoutHandler(const boost::system::error_code& ec, const std::string& sAddressOfRecord, boost::asio::deadline_timer* pTimer)
{
  delete pTimer;
  if (!ec)
  {
    remove(sAddressOfRecord);
  }
}

void Registrar::remove(const std::string& sAddressOfRecord)
{
  auto it = m_mContacts.find(sAddressOfRecord);
  if (it != m_mContacts.end())
  {
    VLOG(2) << "Removing contact " << sAddressOfRecord;
    m_mContacts.erase(it);
  }
}

} // rfc3261
} // rtp_plus_plus
