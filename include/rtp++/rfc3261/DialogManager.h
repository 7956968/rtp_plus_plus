#pragma once
#include <cstdint>
#include <rtp++/rfc3261/SipContext.h>
#include <rtp++/rfc3261/SipDialog.h>
#include <rtp++/rfc3261/SipMessage.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

class DialogManager
{
public:
  /**
   * @brief Constructor
   */
  DialogManager(const SipContext& sipContext);
  /**
   * @brief Gets a count of the active dialogs
   */
  size_t getDialogCount() const { return m_mDialogs.size();  }
  /**
   * @brief Gets a vector containing active dialog ids
   */
  std::vector<DialogId_t> getActiveDialogIds() const;
    /**
   * @brief returns if the dialog exists
   * @param[in] id Dialog id
   */
  bool dialogExists(const DialogId_t& id) const;
  /**
   * @brief returns if the dialog exists
   * @param[in] uiTxId tx id
   */
  bool dialogExists(const uint32_t uiTxId) const;
    /**
   * @brief accessor for dialog: this must only be called if the dialog exist!
   */
  SipDialog& getDialog(const DialogId_t& id);
  /**
   * @brief accessor for dialog: this must only be called if the dialog exist!
   */
  SipDialog& getDialog(const uint32_t uiTxId);
  /**
   * @brief Creates dialog
   * @param[in] uiTxId Transaction ID of the INVITE transaction
   * @param[in] sipRequest request that created the dialog
   * @param[in] sipResponse response that created the dialog
   * @param[in] bIsUac if the creator of the dialog is the UAC
   * @param[in] bEarly if the dialog is early
   * @return The id of the created dialog
   */
  DialogId_t createDialog(const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse, bool bIsUac, bool bEarly);
  /**
   * @brief Terminates dialog
   * @TODO: need to get rid of this version, the txid changes for each request in the dialog so we can't supply this value
   */
  void terminateDialog(const uint32_t uiTxId, const DialogId_t& id);
  /**
   * @brief Terminates dialog
   */
  void terminateDialog(const DialogId_t& id);
  /**
   * @brief Handles the request
   */
  void handleRequest(const DialogId_t& id, const SipMessage& sipRequest, SipMessage& sipResponse);
private:
  /// Existing SIP dialogs
  std::map<DialogId_t, SipDialog> m_mDialogs;
  /// map from tx id to dialog id
  std::map<uint32_t, DialogId_t> m_mIdMap;
  /// SIP context
  SipContext m_sipContext;
  /// Terminated dialogs
  std::deque<DialogId_t> m_qTerminatedDialogs;
};

} // rfc3261
} // rtp_plus_plus
