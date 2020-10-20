#pragma once
#include <map>
#include <string>
#include <boost/optional.hpp>
#include <rtp++/rfc3261/MessageBody.h>
#include <rtp++/rfc3261/Methods.h>
#include <rtp++/rfc3261/ResponseCodes.h>

namespace rtp_plus_plus
{
namespace rfc3261
{
// forward
class SipMessage;

/**
 * @brief checks if the mandatory fields for a request are populated.
 */
bool isValidRequest(const SipMessage& sipMessage);

/**
 * @brief checks if the mandatory fields for a response are populated.
 */
bool isValidResponse(const SipMessage& sipMessage);

/**
 * @brief Breaks the "From", "To" or "Contact" header into display name and URI
 */
extern void getUriAndDisplayName(const std::string& sHeader, std::string& sUri, std::string& sDisplayName);

/**
 * @brief The SipMessage class abstracts SIP requests and responses.
 *
 * @todo: add basic SIP message validation (fields such as to, from, etc)
 * MIN REQ fields: To, From, CSeq, Call-ID, Max-Forwards, and Via
 */
class SipMessage
{
public:
  /**
   * @brief named constructor that creates a SIP message from SIP message string.
   * @return A SipMessage instance if the message was parsed successfully, and a null pointer otherwise.
   */
  static boost::optional<SipMessage> create(const std::string& sSipMessage);
  /**
   * @brief named constructor that creates a SIP response from SIP request if the request is valid.
   * @return A response SipMessage that has fields that match the SIP request.
   */
  static boost::optional<SipMessage> createResponse(const SipMessage& request);
  /**
   * @brief Constructor
   */
  SipMessage();
  /**
   * @brief Constructor for SIP request
   */
  SipMessage(const Method eMethod, const std::string& sRequestUri);
  /**
   * @brief Constructor for SIP request (useful for custom strings)
   */
  SipMessage(const std::string& sMethod, const std::string& sRequestUri);
  /**
   * @brief Constructor for SIP response
   */
  SipMessage(const ResponseCode& eCode);  
  /**
   * @brief Returns if this is a request object.
   */
  bool isRequest() const { return m_eType == SIP_REQUEST; }
  /**
   * @brief Returns if this is a response object.
   */
  bool isResponse() const { return m_eType == SIP_RESPONSE; }
  /**
   * @brief Getter for SIP version.
   * @return String representation of SIP version.
   */
  std::string getVersion() const { return m_sVersion; }
  /**
   * @brief Configures the SipMessage to be a request
   */
  void setRequest(const std::string& sMethod, const std::string& sSipUriString);
  /**
   * @brief Configures the SipMessage to be a response
   */
  void setResponse(const std::string& sResponseCode, const std::string& sResponseDescription);
  /**
   * @brief Getter for method. This will only be populated for known methods.
   * In the method is not known by this lib, getMethodString should be called
   *
   * Request member
   */
  Method getMethod() const { return m_eMethod; }
  /**
   * @brief Getter for SIP method string.
   * @return String representation of SIP method.
   */
  std::string getMethodString() const { return m_sMethod; }
  /**
   * @brief Setter for method
   */
  void setMethod(const std::string& sMethod);
  /**
   * @brief Getter for response code
   *
   * Response member.
   */
  ResponseCode getResponseCode() const { return m_eResponseCode; }
  /**
   * @brief Setter for response code
   *
   * Changes the data structure to be a response.
   * Response member.
   */
  void setResponseCode(ResponseCode eCode);
  /**
   * @brief Getter for Request-URI
   */
  std::string getRequestUri() const { return m_sRequestUriString; }
  /**
   * @brief Setter for request URI
   */
  void setRequestUri(const std::string& sRequestUri) { m_sRequestUriString = sRequestUri; }
  /**
   * @brief Getter for from URI
   */
  std::string getFromUri() const;
  /**
   * @brief Getter for from display name
   */
  std::string getFromDisplayName() const;
  /**
   * @brief Getter for to URI
   */
  std::string getToUri() const;
  /**
   * @brief Getter for to display name
   */
  std::string getToDisplayName() const;
  /**
   * @brief Getter for Contact
   */
  std::string getContact() const;
  /**
   * @brief Getter for Contact URI
   */
  std::string getContactUri() const;
  /**
   * @brief Getter for Contact display name
   */
  std::string getContactDisplayName() const;
  /**
   * @brief Getter for Contact
   */
  void setContact(const std::string& sContact);
  /**
   * @brief Getter for top most via
   */
  std::string getTopMostVia() const;
  /**
   * @brief Setter for top most via
   */
  void setTopMostVia(const std::string& sVia);
  /**
   * @brief Inserts top most via
   */
  void insertTopMostVia(const std::string& sVia);
  /**
   * @brief removes top most via
   */
  std::string removeTopMostVia();
  /**
   * @brief Getter for sent-by in top most via
   */
  std::string getSentByInTopMostVia() const;
  /**
   * @brief Setter for header field. Adds the header field possibly resulting in multiple values per field.
   */
  void addHeaderField(const std::string& sName, const std::string& sValue);
  /**
   * @brief Setter for header field
   */
  void setHeaderField(const std::string& sName, const std::string& sValue);
  /**
   * @brief Returns if the SIP message has the header
   */
  bool hasHeader(const std::string& sAttributeName) const;
  /**
   * @brief Getter for header field
   * @return Returns the header field if set in the request/response, otherwise a null pointer
   */
  boost::optional<std::string> getHeaderField(const std::string& sName) const;
  /**
   * @brief Getter for header fields
   * @return Returns all header fields if set in the request/response, otherwise an empty vector
   */
  std::vector<std::string> getHeaderFields(const std::string& sName) const;
  /**
   * @brief Copies the first header field of passed in request
   */
  bool copyHeaderField(const std::string& sHeaderField, const SipMessage& sipMessage);
  /**
   * @brief Copies all header fields of passed in request
   */
  bool copyHeaderFields(const std::string& sHeaderField, const SipMessage& sipMessage);
  /**
   * @brief Appends to header field
   */
  bool appendToHeaderField(const std::string& sHeaderField, const std::string& sValue);
  /**
   * @brief Checks if a header fields attribute matches
   */
  bool matchHeaderFieldAttribute(const std::string& sHeaderField, const std::string& sAttribute, const SipMessage& sipMessage) const;
  /**
   * @brief Getter for message body
   */
  const MessageBody& getMessageBody() const { return m_messageBody; }
  /**
   * @brief Setter for message body
   */
  void setMessageBody(const MessageBody& messageBody) { m_messageBody = messageBody; }
  /**
   * @brief returns a string representation of the SIP message
   */
  std::string toString() const;
  /**
   * @brief Returns branch parameter in top most Via header
   */
  boost::optional<std::string> getBranchParameter() const;
  /**
   * @brief Returns request sequence number without method string
   */
  uint32_t getRequestSequenceNumber() const;
  /**
   * @brief Utility method to set CSeq header. Uses the current value of the method string
   */
  void setCSeq(uint32_t uiSn);
  /**
   * @brief Utility method to set CSeq header
   */
  void setCSeq(uint32_t uiSn, const std::string& sMethod);
  /**
   * @brief extracts attribute if it exists
   */
  boost::optional<std::string> getAttribute(const std::string& sHeader, const std::string& sAttribute) const;
  /**
   * @brief returns if attribute exists
   */
  bool hasAttribute(const std::string& sHeader, const std::string& sAttribute) const;
  /**
   * @brief Setter for header attribute
   */
  bool setAttribute(const std::string& sHeader, const std::string& sAttribute, const std::string& sValue);
  /**
   * @brief decrements max forwards if > 0
   */
  void decrementMaxForwards();

private:

  /// SIP Message type enum
  enum MessageType
  {
    SIP_NOT_SET = 0,
    SIP_REQUEST = 1,
    SIP_RESPONSE = 2
  };
  /// SIP message type
  MessageType m_eType;
  /// SIP version
  std::string m_sVersion;
  /// Method
  Method m_eMethod;
  /// Method string for extensibility
  std::string m_sMethod;
  /// SIP Request-URI string
  std::string m_sRequestUriString;
  /// Response code
  ResponseCode m_eResponseCode;
  /// Response code string
  std::string m_sResponseCodeString;
  /// Response code description string
  std::string m_sResponseCodeDescriptionString;
  /// header map
  std::multimap<std::string, std::string> m_mHeaderMap;
  /// SIP message body
  MessageBody m_messageBody;
};

} // rfc3261
} // rtp_plus_plus
