#pragma once
#include <cstdint>
#include <list>
#include <string>
#include <tuple>
#include <boost/optional.hpp>
#include <rtp++/rfc3261/SipContext.h>
#include <rtp++/rfc3261/SipMessage.h>
#include <rtp++/rfc3261/SipUri.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

/**
 * @typedef Call-ID, local tag, remote tag
 */
typedef std::tuple<std::string, std::string, std::string> DialogId_t;

/**
 * @brief creates a Dialog ID based on the SIP message headers
 */
extern boost::optional<DialogId_t> extractDialogId(const SipMessage& sipMessage, bool bIsUac);

extern std::ostream& operator<<(std::ostream& ostream, const DialogId_t& dialogId);

/**
 * @brief enum for dialog states
 */
enum DialogState
{
  DS_EARLY,
  DS_CONFIRMED,
  DS_COMPLETE
};

/**
 * @brief
 *
 // SipDialog: ID =  Call-ID + local tag + remote tag, a local sequence number, a remote sequence number,
 // a local URI, a remote URI, remote target, a boolean
 //  flag called "secure", and a route set, which is an ordered list of
 //  URIs
 // Session States: initial, early, confirmed, terminated
 */
class SipDialog
{
  static const uint32_t EMPTY = UINT32_MAX;
public:

  SipDialog(DialogState eState = DS_EARLY);

  SipDialog(const SipContext& sipContext, DialogState eState = DS_EARLY);

  SipDialog(const SipContext& sipContext, const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse, bool bIsUAC, bool bSecure, DialogState eState = DS_EARLY);

  SipDialog(const SipContext& sipContext, const uint32_t uiTxId, DialogId_t id, const SipMessage& sipRequest, const SipMessage& sipResponse, bool bIsUAC, bool bSecure, DialogState eState = DS_EARLY);

  ~SipDialog();
  /**
   * @brief places the dialog to a confirmed state
   */
  void confirm();
  /**
   * @brief Getter for if dialog is confirmed
   */
  bool isConfirmed() const { return m_eState == DS_CONFIRMED; }
  /**
   * @brief Getter for if dialog is complete
   */
  bool isComplete() const { return m_eState == DS_COMPLETE; }
  /**
   * @brief generates a SIP message inside the dialog context
   */
  SipMessage generateSipRequest(const std::string& sMethod);
  /**
   * @brief handles request in dialog
   */
  void handleRequest(const SipMessage& sipRequest, SipMessage& sipResponse);
  /**
   * @brief Should be called when the UAS sends a 200 OK to the client.
   * The subsequent ACK should be handled in the dialog
   */
  void onSend200Ok();
  /**
   * @brief terminates dialog
   */
  void terminateDialog();

private:
  void intialiseDialogState(bool bIsUac, const SipMessage& sipMessage);

private:
  /// Dialog state
  DialogState m_eState;
  /// Context
  SipContext m_sipContext;
  /// tx ID, allows us to find out which dialogs belong to the same INVITE
  uint32_t m_uiTxId;
  /// dialog id
  DialogId_t m_dialogId;
  /// is UAC
  bool m_bIsUAC;
  /// secure dialog flag
  bool m_bSecure;
  /// local sequence number
  uint32_t m_uiLocalSN;
  /// remote sequence number
  uint32_t m_uiRemoteSN;
  /// local URI
  SipUri m_localUri;
  /// remote URI
  SipUri m_remoteUri;
  /// remote target?

  /// route set
  std::list<SipUri> m_routeSet;
};

} // rfc3261
} // rtp_plus_plus
