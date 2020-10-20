#pragma once
#include <sstream>
#include <unordered_map>
#include <boost/optional.hpp>
#include <rtp++/rfc2326/MessageBody.h>
#include <rtp++/rfc2326/ResponseCodes.h>
#include <rtp++/rfc2326/RtspMethod.h>

namespace rtp_plus_plus
{
namespace rfc2326 
{

/**
 * @brief The RtspMessage class abstracts RTSP requests and responses.
 */
class RtspMessage
{
public:
  /**
   * @brief Named constructor that parses the string parameters and creates the appropriate RtspMessage object
   * @param sMethodOrResponseCode RTSP method string for RTSP requests, and response code for responses.
   * @param sRtspUriOrResponseDescription RTSP URI for RTSP requests, or otherwise the  response code description for responses.
   * @param sVersion RTSP version
   * @return A RtspMessage instance if the message was parsed successfully, and a null pointer otherwise.
   */
  static boost::optional<RtspMessage> create(const std::string& sMethodOrResponseCode,
                                             const std::string& sRtspUriOrResponseDescription,
                                             const std::string& sVersion);
  /**
   * @brief Named constructor method that parses the string parameters and creates the appropriate RtspMessage object.
   * @param sRtspMessage The string containing the RTSP message to be parsed.
   * @return A RtspMessage instance if the message was parsed successfully, and a null pointer otherwise.
   */
  static boost::optional<RtspMessage> parse(const std::string& sRtspMessage);
  /**
   * @brief Constructor
   */
  RtspMessage();
  /**
   * @brief Constructor
   * @param sMethodOrResponseCode RTSP method string for RTSP requests, and response code for responses.
   * @param sRtspUriOrResponseDescription RTSP URI for RTSP requests, or otherwise the  response code description for responses.
   * @param sVersion RTSP version
   */
  RtspMessage (const std::string& sMethod, const std::string& sRtspUri,
               const std::string& sRtspVersion = "1.0");
  /**
   * @brief Getter for RTSP version.
   * @return String representation of RTSP version.
   */
  std::string getVersion() const { return m_sVersion; }
  /**
   * @brief Setter for RTSP version.
   * @param sVersion String representation of RTSP version.
   */
  void setVersion(const std::string& sVersion) { m_sVersion = sVersion; }
  /**
   * @brief Returns if this is a request object.
   */
  bool isRequest() const { return m_eType == RTSP_REQUEST; }
  /**
   * @brief Returns if this is a response object.
   */
  bool isResponse() const { return m_eType == RTSP_RESPONSE; }
  /**
   * @brief Getter for CSeq header
   * @return Returns the CSeq header field if set in the request, otherwise an empty string
   */
  std::string getCSeq() const;
  /**
   * @brief Getter for Session header
   * @return Returns the Session header field if set in the request, otherwise an empty string
   */
  std::string getSession() const;
  /**
   * @brief Getter for header field
   * @return Returns the header field if set in the request/response, otherwise a null pointer
   */
  boost::optional<std::string> getHeaderField(const std::string& sName) const;
  /**
   * @brief Getter for method.
   *
   * Request member
   */
  RtspMethod getMethod() const { return m_eMethod; }
  /**
   * @brief Getter for RTSP URI.
   *
   * Request member.
   */
  std::string getRtspUri() const { return m_sRtspUri; }
  /**
   * @brief Getter for response code
   *
   * Response member.
   */
  ResponseCode getResponseCode() const { return m_eCode; }
  /**
   * @brief Setter for response code
   *
   * Changes the data structure to be a response.
   * Response member.
   */
  void setResponse(ResponseCode eCode);
  /**
   * @brief Getter for message body
   */
  const MessageBody& getMessageBody() const { return m_messageBody; }
  /**
   * @brief Setter for message body
   */
  void setMessageBody(const MessageBody& messageBody) { m_messageBody = messageBody; }

private:
  /// RTSP version
  std::string m_sVersion;
  /// RTSP Message type
  enum MessageType
  {
    RTSP_NOT_SET = 0,
    RTSP_REQUEST = 1,
    RTSP_RESPONSE = 2
  };
  MessageType m_eType;

  // request members
  RtspMethod m_eMethod;
  std::string m_sRtspUri;

  // response members
  ResponseCode m_eCode;

  std::unordered_map<std::string, std::string> m_mHeaderMap;

  MessageBody m_messageBody;
};

} // rfc2326
} // rtp_plus_plus
