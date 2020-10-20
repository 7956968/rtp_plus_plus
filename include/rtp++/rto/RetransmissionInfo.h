#pragma once
#include <cstdint>
#include <vector>

namespace rtp_plus_plus
{
namespace rto
{

// once packets are assumed lost, we need to count
// - successful retransmissions
// - false positives i.e. packet still arrives
class RetransmissionInfo
{
public:
  ~RetransmissionInfo();

  void assumePacketLost(const uint16_t uiSN);
  bool isPacketAssumedLost(const uint16_t uiSN) const;
  void falsePositive(const uint16_t uiSN);
  void rtxReceived(const uint16_t uiSN);
  void rtxLate(const uint16_t uiSN);
  void removedBeforeRtx(const uint16_t uiSN);

  uint32_t getAssumedLostCount() const
  {
    return Lost.size();
  }

  uint32_t getFalsePositiveCount() const
  {
    return FalsePositives.size();
  }

  uint32_t getReceivedRtxCount() const
  {
    return ReceivedRtx.size();
  }

  uint32_t getLateRtxCount() const
  {
    return LateRtx.size();
  }

  uint32_t getLostAfterLateCount() const
  {
    return LostAfterLate.size();
  }

  uint32_t getLostAfterRtxCount() const
  {
    return LostAfterLateAndRtx.size();
  }

  uint32_t getRedundantRtxCount() const
  {
    return RedundantRtx.size();
  }

  uint32_t getCancelledRtxCount() const
  {
    return CancelledRtx.size();
  }

  void finalise();
private:

  // vector of extended sequence numbers
  std::vector<uint16_t> Lost;
  // inserted once a packet is received that was assumed lost
  std::vector<uint16_t> FalsePositives;
  // inserted once an RTX packet is received of one that was assumed lost
  std::vector<uint16_t> ReceivedRtx;
  // inserted once an RTX packet is deemed to be too late
  std::vector<uint16_t> LateRtx;
  // inserted if a packet could be removed before an RTX could be sent
  std::vector<uint16_t> CancelledRtx;

  // Lost packets after false positives have been subtracted
  std::vector<uint16_t> LostAfterLate;
  // Lost packets after false positives have been subtracted and RX have been received
  std::vector<uint16_t> LostAfterLateAndRtx;
  std::vector<uint16_t> RedundantRtx;
};

} // rto
} // rtp_plus_plus
