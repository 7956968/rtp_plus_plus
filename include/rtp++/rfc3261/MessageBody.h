#pragma once
#include <string>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @brief Message body for SIP messages.
 *
 * @todo: Cod duplication - Refactor this with RTSP message body?
 */
class MessageBody
{
public:
  MessageBody(){}

  explicit MessageBody(const std::string& sMessageBody)
    :m_sMessageBody(sMessageBody)
  {

  }

  explicit MessageBody(const std::string& sMessageBody, const std::string& sContentType)
    :m_sMessageBody(sMessageBody),
    m_sContentType(sContentType)
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
  size_t getContentLength() const { return m_sMessageBody.length(); }
private:
  std::string m_sMessageBody;
  std::string m_sContentBase;
  std::string m_sContentLocation;
  std::string m_sContentType;
  std::string m_sRequestUri;
};

} // rfc3261
} // rtp_plus_plus
