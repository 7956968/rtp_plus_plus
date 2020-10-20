#include "CorePch.h"
#include <rtp++/TransmissionManager.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
#include <rtp++/rfc3550/RtpHeader.h>
#include <rtp++/mprtp/MpRtpHeader.h>

using boost::asio::deadline_timer;

#define COMPONENT_LOG_LEVEL 10

namespace rtp_plus_plus
{

TransmissionManager::TransmissionManager(RtpSessionState& rtpSessionState, boost::asio::io_service& ioService,
  TxBufferManagementMode eMode, uint32_t uiRtxTimeMs, uint32_t uiRecvBufferSize)
  :m_eMode(eMode),
    m_rtpSessionState(rtpSessionState),
    m_rIoService(ioService),
    m_uiRtxTimeMs(uiRtxTimeMs),
    m_qRecentArrivals(uiRecvBufferSize),
    m_uiLastReceivedExtendedSN(0),
    m_qCircularBuffer(30) // store 30 packets in circular mode?
{
  VLOG(2) << "Transmission manager: "
          << " Mode: " << ((eMode== TxBufferManagementMode::CIRCULAR_MODE) ? "circular" : ((eMode == TxBufferManagementMode::NACK_TIMED_MODE) ? "nack" : "ack"))
          << " SSRC: " << m_rtpSessionState.getSSRC()
          << " Start SN: " << m_rtpSessionState.getCurrentSequenceNumber()
          << " Payload Type: " << (int)m_rtpSessionState.getCurrentPayloadType()
          << " RTX Payload Type: " << (int)m_rtpSessionState.getRtxPayloadType();
}

void TransmissionManager::storePacketForRetransmission(const RtpPacket& rtpPacket)
{
  // don't store retransmission packets
  if (rtpPacket.getHeader().getPayloadType() != m_rtpSessionState.getRtxPayloadType())
  {
    switch (m_eMode)
    {
      case TxBufferManagementMode::CIRCULAR_MODE:
      {
#if 1
        m_qCircularBuffer.push_back(rtpPacket.getSequenceNumber());

        boost::mutex::scoped_lock l( m_rtxLock );
        insertPacketIntoTxBuffer(rtpPacket);

        const int MAX_CIRCULAR = 30;
        if (m_qCircularBuffer.size() > MAX_CIRCULAR)
        {
          uint16_t uiOldestSN = m_qCircularBuffer.front();
          m_qCircularBuffer.pop_front();
          removePacketFromTxBuffer(uiOldestSN);
        }
#else
        // TODO: circular buffer that rotates out packets
        assert(false);
#endif
        break;
      }
      case TxBufferManagementMode::NACK_TIMED_MODE:
      {
        VLOG(15) << "Storing and scheduling " << rtpPacket.getSequenceNumber() << " for removal in " << m_uiRtxTimeMs << "ms";
        deadline_timer* pTimer = new deadline_timer(m_rIoService);
        pTimer->expires_from_now(boost::posix_time::milliseconds(m_uiRtxTimeMs));

        boost::mutex::scoped_lock l( m_rtxLock );
        insertPacketIntoTxBuffer(rtpPacket);
        m_mRtxTimers.insert(std::make_pair(pTimer, pTimer));

        pTimer->async_wait(boost::bind(&TransmissionManager::onRemoveRtpPacketTimeout, this, _1, pTimer, rtpPacket.getSequenceNumber()) );
        break;
      }
      case TxBufferManagementMode::ACK_MODE:
      {
        boost::mutex::scoped_lock l( m_rtxLock );
        insertPacketIntoTxBuffer(rtpPacket);
        break;
      }
    }
  }
}

void TransmissionManager::stop()
{
  // RTX
  VLOG(10) << "[" << this << "] Shutting down RTX timers";
  switch (m_eMode)
  {
    case TxBufferManagementMode::CIRCULAR_MODE:
    {
      break;
    }
    case TxBufferManagementMode::NACK_TIMED_MODE:
    {
      std::for_each(m_mRtxTimers.begin(), m_mRtxTimers.end(),
                    [this](const std::pair<deadline_timer*, deadline_timer*>& pair)
      {
        pair.second->cancel();
      });
      break;
    }
    case TxBufferManagementMode::ACK_MODE:
    {
      break;
    }
  }
}

boost::optional<RtpPacket> TransmissionManager::generateRetransmissionPacket(uint16_t uiSN)
{
  // check if we still have the packet in our retransmission buffer
  boost::mutex::scoped_lock l( m_rtxLock );
  auto it = m_mTxMap.find(uiSN);
  if (it != m_mTxMap.end())
  {
    // we still have the packet in our retransmission buffer
    auto it2 = m_mSnMap.left.find(uiSN);
    if (it2 == m_mSnMap.left.end())
    {
      // RTP
      VLOG(6) << "Sending RTX for SN " << uiSN;
      RtpPacket rtxRtpPacket = generateRtxPacketSsrcMultiplexing(it->second.originalRtpPacket);
      return boost::optional<RtpPacket>(rtxRtpPacket);
    }
    else
    {
      // MPRTP
      uint32_t uiFlowFssn = m_mSnMap.left.at(uiSN);
      uint16_t uiFSSN = uiFlowFssn & 0xFF;
      uint16_t uiFlowId = uiFlowFssn >> 16;
      VLOG(2) << "Sending RTX for SN: " << uiSN << " Flow Id: " << uiFlowId << " FSSN " << uiFSSN;
      RtpPacket rtxRtpPacket = generateRtxPacketSsrcMultiplexing(it->second.originalRtpPacket, uiFlowId, uiFSSN);
      return boost::optional<RtpPacket>(rtxRtpPacket);
    }
  }
  else
  {
    VLOG(3) << "Couldn't find RTX " << uiSN << " in buffer";
    return boost::optional<RtpPacket>();
  }
}

TransmissionManager::RtpPacketInfo* TransmissionManager::lookupRtpPacketInfo(uint16_t uiSN)\
{
  auto it = m_mTxMap.find(uiSN);
  if (it == m_mTxMap.end()) return nullptr;
  else
    return &(it->second);
}

boost::optional<RtpPacket> TransmissionManager::generateRetransmissionPacket(uint16_t uiFlowId, uint16_t uiFSSN)
{
  uint32_t uiFlowIdFssn = (uiFlowId << 16) | uiFSSN;

  // check if we still have the packet in our retransmission buffer
  boost::mutex::scoped_lock l( m_rtxLock );
  auto it = m_mSnMap.right.find(uiFlowIdFssn);
  if (it != m_mSnMap.right.end())
  {
    RtpPacket& rtpPacket = m_mTxMap.at(it->second).originalRtpPacket;
    // we still have the packet in our retransmission buffer
#if 0
    VLOG(2) << "Sending RTX for SN: " << it->second << " Flow Id: " << uiFlowId << " FSSN " << uiFSSN;
#endif
    RtpPacket rtxRtpPacket = generateRtxPacketSsrcMultiplexing(rtpPacket, uiFlowId, uiFSSN);
    return boost::optional<RtpPacket>(rtxRtpPacket);
  }
  else
  {
    VLOG(3) << "Couldn't find RTX Flow: " << uiFlowId << " FSSN: " << uiFSSN << " in buffer";
    return boost::optional<RtpPacket>();
  }
}

bool TransmissionManager::lookupSequenceNumber(uint16_t& uiSN, uint16_t uiFlowId, uint16_t uiFSSN) const
{
  uint32_t uiFlowIdFssn = (uiFlowId << 16) | uiFSSN;
  boost::mutex::scoped_lock l( m_rtxLock );
  auto it = m_mSnMap.right.find(uiFlowIdFssn);
  if (it != m_mSnMap.right.end())
  {
    uiSN = it->second;
    return true;
  }
  else
  {
    return false;
  }
}

void TransmissionManager::onRemoveRtpPacketTimeout(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, uint16_t uiSN)
{
  assert(m_eMode == TxBufferManagementMode::NACK_TIMED_MODE);
  VLOG(15) << "Removing " << uiSN << " from rtx";
  // remove timer from map
  {
    boost::mutex::scoped_lock l( m_rtxLock );
    std::size_t uiErased = m_mRtxTimers.erase(pTimer);
    assert(uiErased == 1);
    removePacketFromTxBuffer(uiSN);
  }
  delete pTimer;
}

void TransmissionManager::insertPacketIntoTxBuffer(const RtpPacket& rtpPacket)
{
  VLOG(COMPONENT_LOG_LEVEL) << "Inserting packet SN " << rtpPacket.getSequenceNumber() << " into TX buffer";
  RtpPacketInfo entry(rtpPacket);
  m_mTxMap.insert(std::make_pair(rtpPacket.getSequenceNumber(), entry ));
  boost::optional<mprtp::MpRtpSubflowRtpHeader> pSubflowHeader = rtpPacket.getMpRtpSubflowHeader();
  if (pSubflowHeader)
  {
    uint32_t uiFlowIdFssn = (pSubflowHeader->getFlowId() << 16) | pSubflowHeader->getFlowSpecificSequenceNumber();
    m_mSnMap.insert(sn_mapping(rtpPacket.getSequenceNumber(), uiFlowIdFssn));
  }
}

void TransmissionManager::removePacketFromTxBuffer(uint16_t uiSN)
{
  VLOG(COMPONENT_LOG_LEVEL) << "Removing packet SN " << uiSN << " from TX buffer";
  m_mTxMap.erase(uiSN);
  if (m_bIsMpRtpSession)
  {
    m_mSnMap.left.erase(uiSN);
  }
}

RtpPacket TransmissionManager::generateRtxPacket(const RtpPacket& rtpPacket)
{
  // copy original RTP header
  rfc3550::RtpHeader header = rtpPacket.getHeader();
  // update PT to be RTX PT
  header.setPayloadType( m_rtpSessionState.getRtxPayloadType() );
  // update PT to be RTX SN
  header.setSequenceNumber(m_rtpSessionState.getNextRtxSequenceNumber());
  // update RTX SSRC
  header.setSSRC(m_rtpSessionState.getRtxSSRC());
  // RTP timestamp must be set equal to TS of original packet
  header.setRtpTimestamp(rtpPacket.getRtpTimestamp());
  // create new RTP packet
  RtpPacket copy(header);
  if (rtpPacket.getMpRtpSubflowHeader())
    copy.setMpRtpSubflowHeader(*rtpPacket.getMpRtpSubflowHeader());
  return copy;
}

RtpPacket TransmissionManager::generateRtxPacketSsrcMultiplexing(const RtpPacket& rtpPacket)
{
  // Store original SN
  uint16_t uiOriginalSequenceNumber = rtpPacket.getSequenceNumber();

  RtpPacket packet = generateRtxPacket(rtpPacket);

  // use original SN as first 2 bytes of payload
  Buffer newPayload;
  uint32_t uiNewSize = rtpPacket.getPayload().getSize() + 2;
  uint8_t* pNewPayload = new uint8_t[uiNewSize];
  newPayload.setData(pNewPayload, uiNewSize);
  OBitStream out(newPayload);
  out.write(uiOriginalSequenceNumber, 16);
  const uint8_t* pData = rtpPacket.getPayload().data();
  bool bRes = out.writeBytes(pData, rtpPacket.getPayload().getSize());
  assert(bRes);
  packet.setPayload(newPayload);

  VLOG(5) << "Generated RTX packet for SN " << rtpPacket.getSequenceNumber()
          << " SSRC :" << rtpPacket.getSSRC()
          << " old payload size: " << rtpPacket.getPayload().getSize()
          << " RTX SN: " << packet.getSequenceNumber()
          << " RTX SSRC: " << packet.getSSRC()
          << " new payload size: " << packet.getPayloadSize();
  return packet;
}

RtpPacket TransmissionManager::generateRtxPacketSsrcMultiplexing(const RtpPacket& rtpPacket,
                                                                   uint16_t uiFlowId,
                                                                   uint16_t uiFSSN)
{
  // PROBLEM: How to handle multiple MPRTP channels RTX state?
  // How to handle scheduling of RTX packet: we don't know here on which flow it will be sent
  // Store original SN
  uint16_t uiOriginalSequenceNumber = rtpPacket.getSequenceNumber();
  RtpPacket packet = generateRtxPacket(rtpPacket);

  // use original SN as first 2 bytes of payload
  Buffer newPayload;
  uint32_t uiNewSize = rtpPacket.getPayload().getSize() + 6;
  uint8_t* pNewPayload = new uint8_t[uiNewSize];
  newPayload.setData(pNewPayload, uiNewSize);
  OBitStream out(newPayload);
  out.write(uiOriginalSequenceNumber, 16);
  out.write(uiFlowId, 16);
  out.write(uiFSSN, 16);
  const uint8_t* pData = rtpPacket.getPayload().data();
  bool bRes = out.writeBytes(pData, rtpPacket.getPayload().getSize());
  assert(bRes);
  packet.setPayload(newPayload);

#if 0
  VLOG(2) << "Generated RTX packet for SN " << uiOriginalSequenceNumber
          << " Flow: " << uiFlowId
          << " FSSN: " << uiFSSN
          << " old payload size: " << rtpPacket.getPayloadSize()
          << " new payload size: " << packet.getPayloadSize();
#endif
  return packet;
}

RtpPacket TransmissionManager::processRetransmission(const RtpPacket& rtpPacket)
{
  // get original sequence number
  IBitStream in(rtpPacket.getPayload());

  boost::optional<mprtp::MpRtpSubflowRtpHeader> pSubflowHeader = rtpPacket.getMpRtpSubflowHeader();
  if (!pSubflowHeader)
  {
    // RTP
    uint16_t uiOriginalSN = 0;
    in.read(uiOriginalSN, 16);

    // strip original SN
    // TODO: we could get rid of this extra unnecessary data copy
    uint32_t uiNewSize = rtpPacket.getSize() - 2;
    uint8_t* pNewBuffer = new uint8_t[uiNewSize];
    Buffer newPayload(pNewBuffer, uiNewSize);
    memcpy(pNewBuffer, rtpPacket.getPayload().data() + 2, uiNewSize);
    // update payload
    RtpPacket packet = rtpPacket;
    // update original sequence number!!
    // packet.getHeader().setSequenceNumber(uiOriginalSN);
    // FIXME: The problem with this code is that it cannot account for wrap around
    // TODO: search for all places in the code where wrap around is used
    packet.setExtendedSequenceNumber(uiOriginalSN);
    packet.setPayload(newPayload);
    return packet;
  }
  else
  {
    // MPRTP
    uint16_t uiOriginalSN = 0;
    uint16_t uiOriginalFlowId = 0;
    uint16_t uiOriginalFSSN = 0;

    in.read(uiOriginalSN, 16);
    in.read(uiOriginalFlowId, 16);
    in.read(uiOriginalFSSN, 16);

    // strip original SN
    // TODO: we could get rid of this extra unnecessary data copy
    uint32_t uiNewSize = rtpPacket.getSize() - 6;
    uint8_t* pNewBuffer = new uint8_t[uiNewSize];
    Buffer newPayload(pNewBuffer, uiNewSize);
    memcpy(pNewBuffer, rtpPacket.getPayload().data() + 6, uiNewSize);
    // update payload
    RtpPacket packet = rtpPacket;
    // update original sequence number!!
    // packet.getHeader().setSequenceNumber(uiOriginalSN);
    // FIXME: The problem with this code is that it cannot account for wrap around
    // TODO: search for all places in the code where wrap around is used
    packet.setExtendedSequenceNumber(uiOriginalSN);
    // recreate original header
    mprtp::MpRtpSubflowRtpHeader originalSubflowHeader(uiOriginalFlowId, uiOriginalFSSN);
    // replace original subflow header
    packet.setMpRtpSubflowHeader(originalSubflowHeader);
    packet.setPayload(newPayload);
    return packet;
  }
}

void TransmissionManager::onPacketReceived(const RtpPacket& rtpPacket)
{
  m_mIncoming[rtpPacket.getExtendedSequenceNumber()].tReceived = rtpPacket.getArrivalTime();
  // TODO: has the extended SN been process at this point?
  m_uiLastReceivedExtendedSN = rtpPacket.getExtendedSequenceNumber();
  boost::mutex::scoped_lock l( m_bufferLock );
  m_qRecentArrivals.push_back(rtpPacket.getExtendedSequenceNumber());

  if (m_tFirstPacketReceived.is_not_a_date_time())
    m_tFirstPacketReceived = rtpPacket.getArrivalTime();
}

boost::posix_time::ptime TransmissionManager::getPacketArrivalTime(uint32_t uiSequenceNumber) const
{
  auto it = m_mIncoming.find(uiSequenceNumber);
  if (it != m_mIncoming.end())
  {
    return it->second.tReceived;
  }
  return boost::posix_time::not_a_date_time;
}

std::vector<uint32_t> TransmissionManager::getLastNReceivedSequenceNumbers(uint32_t uiN)
{
  std::vector<uint32_t> vSNs;
  if (uiN == 0) return vSNs;
  vSNs.reserve(uiN);
  boost::mutex::scoped_lock l( m_bufferLock );
  for (auto rit = m_qRecentArrivals.rbegin(); rit != m_qRecentArrivals.rend() && vSNs.size() < uiN; ++rit)
  {
    vSNs.insert(vSNs.begin(), *rit);
  }
  return vSNs;
}

void TransmissionManager::updatePathChar(const PathInfo& pathInfo)
{
  m_pathInfo = pathInfo;
}

void TransmissionManager::ack(const std::vector<uint16_t>& acks)
{
  std::vector<uint16_t> newlyAcked;
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  std::ostringstream ostr;
  for (auto ack : acks)
  {
    auto it = m_mTxMap.find(ack);
    // update time
    if (it != m_mTxMap.end())
    {
      // only update newly acked
      if (it->second.tAcked.is_not_a_date_time())
      {
        it->second.tAcked = tNow;
        newlyAcked.push_back(ack);
      }
      // remove
      int32_t iAckMs = -1;
      if (!it->second.tAcked.is_not_a_date_time())
      {
        iAckMs = (it->second.tAcked - it->second.tStored).total_milliseconds();
      }

      ostr << it->second.originalRtpPacket.getSequenceNumber()
           << " (" << iAckMs << " ms) ";
      // we don't want to delete the meta info: we could just delete the payload?
#if 0
      m_mRtxMap.erase(it);
#else
      VLOG(COMPONENT_LOG_LEVEL) << "Should remove SN " << ack << " from TX buffer";
#endif
    }
  }
  VLOG(2) << "Ack times: " << ostr.str();

  if (m_onAck) m_onAck(newlyAcked);
}

void TransmissionManager::nack(const std::vector<uint16_t>& nacks)
{
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  for (auto nack : nacks)
  {
    auto it = m_mTxMap.find(nack);
    if (it != m_mTxMap.end())
      it->second.tNacked = tNow;
  }

  if (m_onNack) m_onNack(nacks);
}

TransmissionManager::RtpPacketInfo::RtpPacketInfo()
  :rtpPacketSize(0),
    rtpPayloadSize(0),
    bProcessedByScheduler(false)
{

}

TransmissionManager::RtpPacketInfo::RtpPacketInfo(const RtpPacket& rtpPacket)
  :rtpPacketSize(rtpPacket.getSize()),
    rtpPayloadSize(rtpPacket.getPayloadSize()),
    originalRtpPacket(rtpPacket),
    tStored(boost::posix_time::microsec_clock::universal_time()),
    bProcessedByScheduler(false)
{

}

TransmissionManager::RtpPacketInfo::~RtpPacketInfo()
{
}

} // rtp_plus_plus
