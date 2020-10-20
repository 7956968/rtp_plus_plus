#pragma once
#include <string>

namespace rtp_plus_plus
{
namespace rfc2326
{

/**
 * @brief Message body for RTSP messages.
 *
 * @todo: Code duplication - Refactor this with SIP message body?
 */
class MessageBody
{
public:
  MessageBody(){}

  explicit MessageBody(const std::string& sMessageBody)
    :m_sMessageBody(sMessageBody)
  {

  }

  std::string getMessageBody() const { return m_sMessageBody; }
  void setMessageBody(const std::string& val) { m_sMessageBody = val; }
  std::string getContentBase() const { return m_sContentBase; }
  void setContentBase(const std::string& val) { m_sContentBase = val; }
  std::string getContentLocation() const { return m_sContentLocation; }
  void setContentLocation(const std::string& val) { m_sContentLocation = val; }
  std::string getContentType() const { return m_sContentType; }
  void setContentType(const std::string& val) { m_sContentType = val; }
  std::string getRequestUri() const { return m_sRequestUri; }
  void setRequestUri(const std::string& val) { m_sRequestUri = val; }

private:
  std::string m_sMessageBody;
  std::string m_sContentBase;
  std::string m_sContentLocation;
  std::string m_sContentType;
  std::string m_sRequestUri;
};

} // rfc2326
} // rtp_plus_plus
