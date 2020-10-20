#include "CorePch.h"
#include <rtp++/rfc3261/DialogManager.h>

namespace rtp_plus_plus
{
namespace rfc3261
{

DialogManager::DialogManager(const SipContext& sipContext)
  :m_sipContext(sipContext)
{

}

std::vector<DialogId_t> DialogManager::getActiveDialogIds() const
{
  std::vector<DialogId_t> vDialogIds;
  for (auto& pair : m_mDialogs)
  {
    if (!pair.second.isComplete())
      vDialogIds.push_back(pair.first);
  }
  return vDialogIds;
}

bool DialogManager::dialogExists(const DialogId_t& id) const
{
  return m_mDialogs.find(id) != m_mDialogs.end();
}

bool DialogManager::dialogExists(const uint32_t uiTxId) const
{
  auto it = m_mIdMap.find(uiTxId);
  if (it == m_mIdMap.end()) return false;
  return m_mDialogs.find(it->second) != m_mDialogs.end();
}

SipDialog& DialogManager::getDialog(const DialogId_t& id)
{
  assert(dialogExists(id));
  return m_mDialogs[id];
}

SipDialog& DialogManager::getDialog(const uint32_t uiTxId)
{
  auto it = m_mIdMap.find(uiTxId);
  assert(it != m_mIdMap.end());
  assert(dialogExists(it->second));
  return m_mDialogs[it->second];
}

DialogId_t DialogManager::createDialog(const uint32_t uiTxId, const SipMessage& sipRequest, const SipMessage& sipResponse, bool bIsUac, bool bEarly)
{
  boost::optional<DialogId_t> id = extractDialogId(sipResponse, bIsUac);
  assert(id);
  VLOG(5) << "Creating dialog TX_ID: " << uiTxId << " D_ID:" << *id;
  // we don't do secure for now
  if (bEarly)
  {
    m_mDialogs[*id] = SipDialog(m_sipContext, uiTxId, *id, sipRequest, sipResponse, bIsUac, false);
  }
  else
  {
    m_mDialogs[*id] = SipDialog(m_sipContext, uiTxId, *id, sipRequest, sipResponse, bIsUac, false, DS_CONFIRMED);
  }
  VLOG(5) << "Dialog count: " << m_mDialogs.size();
  m_mIdMap[uiTxId] = *id;
  return *id;
}

void DialogManager::terminateDialog(const uint32_t uiTxId, const DialogId_t& id)
{
  VLOG(5) << "Terminating dialog TX_ID: " << uiTxId << " D_ID:" << id << " Total dialogs: " << m_mDialogs.size();
  if (m_mDialogs.find(id) != m_mDialogs.end())
  {
    m_mDialogs.erase(id);
    m_mIdMap.erase(uiTxId);
  }
  else
  {
    VLOG(2) << "Couldn't find dialog for termination: " << uiTxId;
    for (auto& pair : m_mDialogs)
    {
      VLOG(5) << "Existing dialog id: " << pair.first;
    }
  }
  VLOG(5) << "Dialog count after: " << m_mDialogs.size();
}

void DialogManager::terminateDialog(const DialogId_t& id)
{
  VLOG(5) << "Terminating dialog ID:" << id << " Total dialogs: " << m_mDialogs.size();
  if (m_mDialogs.find(id) != m_mDialogs.end())
  {
    m_mDialogs.erase(id);
  }
  else
  {
    VLOG(2) << "Couldn't find dialog for termination: " << id;
    for (auto& pair : m_mDialogs)
    {
      VLOG(5) << "Existing dialog id: " << pair.first;
    }
  }
  VLOG(5) << "Dialog count after: " << m_mDialogs.size();
}

void DialogManager::handleRequest(const DialogId_t& id, const SipMessage& sipRequest, SipMessage& sipResponse)
{
  if (!dialogExists(id))
  {
    //  The UAS MUST still respond to any pending requests received for that
    //  dialog.It is RECOMMENDED that a 487 (Request Terminated) response
    //  be generated to those pending requests.
    
    // check if dialog was recently terminated
    for (size_t i = 0; i < m_qTerminatedDialogs.size(); ++i)
    {
      if (m_qTerminatedDialogs[i] == id)
      {
        VLOG(2) << "Request for recently terminated dialog";
        sipResponse.setResponseCode(REQUEST_TERMINATED);
        return;
      }
    }
    // can't locate dialog
    VLOG(2) << "Cannot find dialog " << id;
    sipResponse.setResponseCode(CALL_TRANSACTION_DOES_NOT_EXIST);
  }
  else
  {
    VLOG(5) << "Found dialog. " << id << " dialog will handle request";
    m_mDialogs[id].handleRequest(sipRequest, sipResponse);

    // finally, if it's a BYE request, we'll terminate the dialog
    if (sipRequest.getMethod() == BYE)
    {
      VLOG(2) << "BYE request - terminating dialog";
      terminateDialog(id);
      m_qTerminatedDialogs.push_back(id);
    }
  }
}

} // rfc3261
} // rtp_plus_plus
