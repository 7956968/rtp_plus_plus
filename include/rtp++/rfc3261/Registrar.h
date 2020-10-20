#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/optional.hpp>

namespace rtp_plus_plus
{
namespace rfc3261
{

struct ContactRecord
{
  ContactRecord()
  {

  }

  ContactRecord(const std::string& sAddressOfRecord, const std::string& sResponsible, const std::string& sContact)
    :AddressOfRecord(sAddressOfRecord),
    Responsible(sResponsible),
    Contact(sContact)
  {

  }

  std::string AddressOfRecord;
  std::string Responsible;
  std::string Contact;
};

class Registrar
{
public:
  /**
   * @brief Constructor
   */
  Registrar(boost::asio::io_service& ioService);
  /**
   * @brief Destructor
   */
  ~Registrar();
  /**
   * @brief Method to register SIP agent
   */
  bool registerSipAgent(const std::string& sAddressOfRecord, const std::string& sResponsible, const std::string& sContact, uint32_t uiExpiresSeconds);
  /**
   * @brief Returns contact record if available, else null pointer
   */
  boost::optional<ContactRecord> find(const std::string& sAddressOfRecord);

protected:
  void timeoutHandler(const boost::system::error_code& ec, const std::string& sAddressOfRecord, boost::asio::deadline_timer* pTimer);

private:

  void remove(const std::string& sAddressOfRecord);

private:

  /// io service
  boost::asio::io_service& m_ioService;
  /// registered contact map
  std::map<std::string, std::pair<ContactRecord, boost::asio::deadline_timer*> > m_mContacts;
};

} // rfc3261
} // rtp_plus_plus
