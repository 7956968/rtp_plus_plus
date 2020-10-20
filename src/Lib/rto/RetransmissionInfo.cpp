#include "CorePch.h"
#include <rtp++/rto/RetransmissionInfo.h>
#include <cpputil/Conversion.h>

namespace rtp_plus_plus
{
namespace rto
{

RetransmissionInfo::~RetransmissionInfo()
{
  VLOG(2) << "RTO Lost [" << Lost.size() << "]: " << toString(Lost);
  VLOG(2) << "RTO Late [" << FalsePositives.size() << "]: " << toString(FalsePositives);
  VLOG(2) << "RTO RTX Received [" << ReceivedRtx.size() << "]: " << toString(ReceivedRtx);
  VLOG(2) << "RTO RTX Late [" << LateRtx.size() << "]: " << toString(LateRtx);
  VLOG(2) << "Lost after Late [" << LostAfterLate.size() << "]: " << toString(LostAfterLate);
  VLOG(2) << "Lost after Late and RTX [" << LostAfterLateAndRtx.size() << "]: " << toString(LostAfterLateAndRtx);
}

void RetransmissionInfo::finalise()
{
  // HACK for now until PacketTransmissionInfo is implemented
  // Check how many of the packets were really lost
  LostAfterLate = Lost;
  LostAfterLate.erase(std::remove_if(LostAfterLate.begin(), LostAfterLate.end(), [this](uint16_t uiSN)
  {
    return std::find(FalsePositives.begin(), FalsePositives.end(), uiSN) != FalsePositives.end();
  }), LostAfterLate.end());

  // go through RTX packets and remove from lost
  LostAfterLateAndRtx = LostAfterLate;
  LostAfterLateAndRtx.erase(std::remove_if(LostAfterLateAndRtx.begin(), LostAfterLateAndRtx.end(), [this](uint16_t uiSN)
  {
    return std::find(ReceivedRtx.begin(), ReceivedRtx.end(), uiSN) != ReceivedRtx.end();
  }), LostAfterLateAndRtx.end());

  // redundant RTX
  // TODO: FIXME: redundant should be calculated by inserting all SNs that are in FalsePositives and ReceivedRtx
  for (uint16_t uiSN: ReceivedRtx)
  {
    auto it = std::find(FalsePositives.begin(), FalsePositives.end(), uiSN);
    if (it != FalsePositives.end())
        RedundantRtx.push_back(uiSN);
  }

//    RedundantRtx.erase(std::remove_if(RedundantRtx.begin(), RedundantRtx.end(), [this](uint16_t uiSN)
//    {
//      return std::find(FalsePositives.begin(), FalsePositives.end(), uiSN) != FalsePositives.end();
//    }), RedundantRtx.end());
}

void RetransmissionInfo::assumePacketLost(const uint16_t uiSN)
{
  // RG: not removing lost packets for now so don't need lock
  // Also, the sorted insertion was for fast removal from the back
  // since we're attempting to now just keep track of losses without
  // removing them from the vector, push_back should suffice
  Lost.push_back(uiSN);
  //boost::mutex::scoped_lock l( m_lostLock );
  // find first element smaller and insert behind
//  auto it = std::find_if(Lost.rbegin(), Lost.rend(), [&uiSN](uint32_t uiSNTemp)
//  {
//     return uiSNTemp < uiSN;
//  });SimplePacketLossDetection

//  Lost.insert(it.base(), uiSN);
}

bool RetransmissionInfo::isPacketAssumedLost(const uint16_t uiSN) const
{
  // RG: not removing lost packets for now so don't need lock
  //boost::mutex::scoped_lock l( m_lostLock );
  for (auto rit = Lost.rbegin(); rit != Lost.rend(); ++rit )
  {
    if (*rit == uiSN) return true;
    if (*rit < uiSN) break;
  }
  return false;
}

void RetransmissionInfo::falsePositive(const uint16_t uiSN)
{
  FalsePositives.push_back(uiSN);
  //removeFromLost(uiSN);
}

void RetransmissionInfo::rtxReceived(const uint16_t uiSN)
{
  ReceivedRtx.push_back(uiSN);
  //removeFromLost(uiSN);
}

void RetransmissionInfo::rtxLate(const uint16_t uiSN)
{
  LateRtx.push_back(uiSN);
  //removeFromLost(uiSN);
}

void RetransmissionInfo::removedBeforeRtx(const uint16_t uiSN)
{
  CancelledRtx.push_back(uiSN);
}

} // rto
} // rtp_plus_plus
