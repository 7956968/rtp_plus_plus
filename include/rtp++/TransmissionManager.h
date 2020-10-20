#pragma once
#include <unordered_map>
#include <boost/asio/deadline_timer.hpp>
#ifdef _WIN32
#pragma warning(push)     // disable for this header only
#pragma warning(disable:4503) 
#endif
#include <boost/bimap.hpp>
#include <boost/circular_buffer.hpp>
#ifdef _WIN32
#pragma warning(pop)      // restore original warning level
#endif
#include <boost/optional.hpp>
#include <boost/thread/mutex.hpp>
#include <rtp++/MemberUpdate.h>
#include <rtp++/RtpPacket.h>
#include <rtp++/RtpSessionState.h>

// fwd
namespace boost
{
namespace asio
{
class io_service;
}
}

namespace rtp_plus_plus {

enum class TxBufferManagementMode
{
  CIRCULAR_MODE,    // in this mode the last n packets are stored in the rtx buffer
  NACK_TIMED_MODE,  // in this mode we use the rtx timeout to remove previously stored packets from the buffer
  ACK_MODE          // in this mode, packets are removed from the buffer on ACK, as well as when the difference exceeds a predefined max
};

typedef boost::function<void (const std::vector<uint16_t>&)> AckCb_t;
typedef boost::function<void (const PathInfo&)> PathInfoCb_t;
/**
 * @brief The TransmissionManager class stores meta information about
 * sent packets as well as the packets themselves for retransmission purposes.
 * To prevent the memory used from growing indefinitely we use the following
 * memory management stragies:
 * - CIRCULAR_MODE: we use a fixed size queue to store the last x sent packets.
 * - NACK_TIMED_MODE: each sent packet is stored for the retransmission time.
 * - ACK_MODE: packets that have been acked are removed from the retransmission buffer immediately.
 */
class TransmissionManager
{
public:  
  struct RtpPacketInfo
  {
    RtpPacketInfo();
    RtpPacketInfo(const RtpPacket& rtpPacket);
    ~RtpPacketInfo();

    RtpPacket originalRtpPacket;
    uint32_t rtpPacketSize;
    uint32_t rtpPayloadSize;
    boost::posix_time::ptime tStored;
    boost::posix_time::ptime tAcked;
    boost::posix_time::ptime tNacked; // only storing one nack time for now
    boost::posix_time::ptime tReceived;
    bool bProcessedByScheduler;
  };
  /**
   * @brief TransmissionManager Constructor
   * @param rtpSessionState
   * @param ioService
   * @param uiRtxPayloadType
   * @param uiRtxTimeMs
   * @param uiRecvBufferSize Size of circular buffer to store recently received packets. For now 64 packets should be enough for acking.
   */
  explicit TransmissionManager(RtpSessionState& rtpSessionState, boost::asio::io_service& ioService, TxBufferManagementMode eMode, uint32_t uiRtxTimeMs, uint32_t uiRecvBufferSize = 64);
  /**
   * @brief lookupRtpPacketInfo returns the info to the RtpPacket info if it exists.
   * @param uiSN
   * @return
   */
  RtpPacketInfo* lookupRtpPacketInfo(uint16_t uiSN);
  /**
   * @brief getLatestPathInfo Getter for path info
   * @return
   */
  const PathInfo& getLatestPathInfo(){ return m_pathInfo; }
  /**
   * @brief returns a vector containing the last up to N received sequence numbers
   */
  std::vector<uint32_t> getLastNReceivedSequenceNumbers(uint32_t uiN);
  /**
   * @brief storePacketForRetransmission
   * @param rtpPacket
   */
  void storePacketForRetransmission(const RtpPacket& rtpPacket);
  /**
   * @brief generateRetransmissionPacket generates a retransmission packet if the original packet
   * with sequence number uiSN is still in the buffer.
   * @param uiSN
   * @return
   */
  boost::optional<RtpPacket> generateRetransmissionPacket(uint16_t uiSN);
  /**
   * @brief generateRetransmissionPacket generates a retransmission packet if the original packet
   * with sequence number uiSN is still in the buffer.
   * @param uiFlowId
   * @param uiFSSN
   * @return
   */
  boost::optional<RtpPacket> generateRetransmissionPacket(uint16_t uiFlowId, uint16_t uiFSSN);
  /**
   * @brief lookupSequenceNumber Looks up the global sequence number given the flow id and FSSN.
   * @param[out] uiSN The global sequence number of the original packet
   * @param[in] uiFlowId The flow Id
   * @param[in] uiFSSN The flow specific sequence number
   * @return true if the manager was able to determine the global sequence number associated with the flow id and FSSN, false otherwise.
   */
  bool lookupSequenceNumber(uint16_t& uiSN, uint16_t uiFlowId, uint16_t uiFSSN) const;
  /**
   * @brief processRetransmission processes a received retransmission and restores the original RTP packet.
   * @param rtpPacket The retransmitted RTP packet
   * @return The original RTP packet
   */
  RtpPacket processRetransmission(const RtpPacket& rtpPacket);
  /**
   *  @brief onPacketReceived should be called when each packet arrives
   */
  void onPacketReceived(const RtpPacket& rtpPacket);
  /**
   * @brief updatePathChar
   * @param pathInfo
   */
  void updatePathChar(const PathInfo& pathInfo);
  /**
   * @brief ack
   * @param acks
   */
  void ack(const std::vector<uint16_t>& acks);
  /**
   * @brief nack
   * @param acks
   */
  void nack(const std::vector<uint16_t>& nacks);
  /**
   * @brief stop should be called before destruction to cleanup state e.g. cancel timers.
   */
  void stop();
  /**
   * @brief setAckCb
   * @param onAck
   */
  void setAckCb(AckCb_t onAck) { m_onAck = onAck; }
  /**
   * @brief setNackCb
   * @param onNack
   */
  void setNackCb(AckCb_t onNack) { m_onNack = onNack; }
  /**
   * @brief setPathInfoCb
   * @param onPathInfo
   */
  void setPathInfoCb(PathInfoCb_t onPathInfo) { m_onPathInfo = onPathInfo; }
  /**
   * @brief getInitialPacketArrivalTime
   * @return
   */
  boost::posix_time::ptime getInitialPacketArrivalTime() const { return m_tFirstPacketReceived; }
  /**
   * @brief getPacketArrivalTime
   * @param uiSequenceNumber
   * @return
   */
  boost::posix_time::ptime getPacketArrivalTime(uint32_t uiSequenceNumber) const;
  /**
   * @brief getLastReceivedExtendedSN
   * @return
   */
  uint32_t getLastReceivedExtendedSN() const { return m_uiLastReceivedExtendedSN; }

private:
  void onRemoveRtpPacketTimeout(const boost::system::error_code& ec, boost::asio::deadline_timer* pTimer, uint16_t uiSN);
  RtpPacket generateRtxPacketSsrcMultiplexing(const RtpPacket& rtpPacket);
  RtpPacket generateRtxPacketSsrcMultiplexing(const RtpPacket& rtpPacket, uint16_t uiFlowId, uint16_t uiFSSN);
  RtpPacket generateRtxPacket(const RtpPacket& rtpPacket);
  void insertPacketIntoTxBuffer(const RtpPacket& rtpPacket);
  void removePacketFromTxBuffer(uint16_t uiSN);

private:

  /// Mode of buffer management
  TxBufferManagementMode m_eMode;
  /// RTP session state
  RtpSessionState& m_rtpSessionState;
  /// asio service
  boost::asio::io_service& m_rIoService;
  /// RTX time in milliseconds
  uint32_t m_uiRtxTimeMs;
  /// Lock for RTX data structures
  mutable boost::mutex m_rtxLock;
  /// RTX

  /// outgoing stats
  std::unordered_map<uint16_t, RtpPacketInfo> m_mTxMap;
  /// Timers for RTX
  std::unordered_map<boost::asio::deadline_timer*, boost::asio::deadline_timer*> m_mRtxTimers;
  /// for MPRTP RTX
  bool m_bIsMpRtpSession;
  /// map SN to FLowID + FSSN
  typedef boost::bimap< uint16_t, uint32_t > bm_type;
  typedef bm_type::value_type sn_mapping;
  bm_type m_mSnMap;
  // SP: MP woukd require map
  PathInfo m_pathInfo;
  /// buffer for quick access to last received sequence numbers
  mutable boost::mutex m_bufferLock;
  boost::circular_buffer<uint32_t> m_qRecentArrivals;
  /// incoming stats
  std::unordered_map<uint32_t, RtpPacketInfo> m_mIncoming;
  boost::posix_time::ptime m_tFirstPacketReceived;
  uint32_t m_uiLastReceivedExtendedSN;
  std::deque<uint16_t> m_qCircularBuffer;

  PathInfoCb_t m_onPathInfo;
  AckCb_t m_onNack;
  AckCb_t m_onAck;
};

} // rtp_plus_plus
