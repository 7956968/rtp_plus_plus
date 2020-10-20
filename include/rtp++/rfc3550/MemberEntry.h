#pragma once
#include <boost/date_time/posix_time/ptime.hpp>

#include <rtp++/LossEstimator.h>
#include <rtp++/MemberUpdate.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/RtpPacket.h>
#include <rtp++/rfc3550/Rtcp.h>
#include <rtp++/rfc3550/RtpConstants.h>
#include <rtp++/rfc3550/SdesInformation.h>
#include <rtp++/rfc3611/RtcpXr.h>

/// @def DEBUG_RTT Log debug info about RTTs
// #define DEBUG_RTT

/// @def DEBUG_RTCP Adds verbose RTCP logging info
// #define DEBUG_RTCP

/// @def DEBUG_JITTER_CALC Add jitter calculation-related logging info
// #define DEBUG_JITTER_CALC

/// @def DEBUG_RFC3550_MEMBER_VALIDATION_ALGORITHM Adds logging related to member validation
// #define DEBUG_RFC3550_MEMBER_VALIDATION_ALGORITHM

/**
 * This class stores all the information relevant to a session participant
 * There are two uses for this class: 
 * 1) store sender state i.e. the number 
 * of packets sent during the last interval, etc.
 * Needed for sender report generation
 * 2) store receiver state i.e. the number of packets
 * received, sender reports, etc.
 */

namespace rtp_plus_plus
{

// fwd
namespace test
{
class MemberEntryTest;
}

namespace rfc3550
{

/**
 * @brief The MemberEntry class
 */
class MemberEntry
{
  friend class test::MemberEntryTest;

public:
  /**
   * @brief MemberEntry Constructor
   */
  MemberEntry();
  /**
   * @brief MemberEntry Constructor
   * @param uiSSRC
   */
  MemberEntry(uint32_t uiSSRC);
  /**
   * @brief ~MemberEntry Destructor
   */
  virtual ~MemberEntry();

  /// SSRC of member
  uint32_t getSSRC() const { return m_uiSSRC; }
  /// local SSRC: the SSRC of the local participant is useful when processing SRs or DLRRs.
  void addLocalSSRC(uint32_t uiLocalSSRC) { m_localSSRCs.insert(uiLocalSSRC); }
  /// method to check if uiSSRC is one of the SSRCs of the local participant
  bool isRelevantToParticipant(uint32_t uiSSRC)
  {
    return m_localSSRCs.find(uiSSRC) != m_localSSRCs.end();
  }

  /// If MemberEntry has been intialised with SN
  bool hasBeenIntialisedWithSN() const { return m_bInit; }
  /// If RTCP has been synced
  bool isRtcpSychronised() const { return m_bIsRtcpSynchronised; }
  /// The time stored for when the last RTP packet was sent
  boost::posix_time::ptime getTimeLastRtpPacketSent() const { return m_tLastRtpPacketSent; }
  /// The time stored for when the last RTP or RTCP packet was sent
  boost::posix_time::ptime getTimeLastPacketSent() const { return m_tLastPacketSent; }
  /// Whether the participant is a sender
  bool isSender() const { return m_bIsSender; }
  /// Set the participant to be a sender
  void setSender(bool bIsSender) { m_bIsSender = bIsSender; }
  /// returns the number of RTP packets received during the last interval
  uint32_t getRtpPacketsReceivedDuringLastInterval() const { return m_uiRtpPacketsReceivedDuringLastInterval; }
  /// sets the number of RTP packets received during the last interval
  void setRtpPacketsReceivedDuringLastInterval(uint32_t val) { m_uiRtpPacketsReceivedDuringLastInterval = val; }
  /// Disables inidividual sequence number loss detection which are output to the log
  /// per RTCP interval when finaliseRRData is called
  void disableDetailedLossDetection() { m_bEnableDetailedLossDetection = false; }
  /// Enables inidividual sequence number loss detection which are output to the log
  /// per RTCP interval when finaliseRRData is called
  void enableDetailedLossDetection() { m_bEnableDetailedLossDetection = true; }
  /**
   *  updates non-empty SdesInfo fields since various
   *  fields may be sent in each SDES RTCP report.
   *
   * \param sdesInfo  Information describing the sdes.
   */
  void updateSdesInfo(const SdesInformation& sdesInfo);
  /**
   * @brief Update the default RFC3550 specified max misorder
   */
  void setMaxMisorder(uint32_t uiMaxMisorder)
  {
    m_uiMaxMisorder = uiMaxMisorder;
  }
  /**
   * @brief Gets the cumulative number of packets lost.
   *
   * \return  The cumulative number of packets lost.
   */
  int32_t getCumulativeNumberOfPacketsLost() const;
  /**
   * @brief Gets the lost fraction as per RFC3550
   */
  uint8_t getLostFraction();
  /**
   * @brief Gets the highest extended sequence number as per RFC3550
   */
  uint32_t getExtendedHighestSequenceNumber() const;
  /**
   * @brief Gets the jitter calculated as per RFC3550
   */
  uint32_t getJitter() const
  {
    return m_uiJitter;
  }
  /**
   * @brief Gets the LSR calculated as per RFC3550
   */
  uint32_t getLsr() const
  {
    return m_uiLsr;
  }
  /**
   * @brief Gets the LSR calculated as per RFC3550
   * returns the delay since the last sender report was received in units of 1/65536 seconds
   */
  uint32_t getDlsr() const;

  /**
   * @brief Initialises the member entry with the info of the first received RTP packet
   * This method is virtual so that we can override it for MPRTP initialisation
   */
  virtual void init(const RtpPacket& rtpPacket);
  /**
   * @brief initialises the local sequence number space
   */
  void initSequence(uint16_t uiSequenceNumber);
  /**
   * @brief This method should be called for subsequent RTP packets: initially this method took an RtpPacket
   * as a parameter. For MPRTP purposes the signature was changed so that we could call this method
   * with the flow specific sequence number
   */
  void processRtpPacket (uint16_t uiSequenceNumber, uint32_t uiRtpTs, uint32_t uiRtpArrivalTs);

  /// This method can be used to insert the own SSRC into db
  void setValidated() { m_uiProbation = 0; }

  // A source is valid once RFC3550_MIN_SEQUENTIAL sequential packets have been received
  bool isValid() const { return m_uiProbation == 0 && !m_bInactive; }
  bool isInactive() const { return m_bInactive; }
  bool isUnvalidated() const { return m_uiProbation > 0; }

  // TODO: can we remove this method
  void newReportingInterval();

  /**
   * @brief A receiver should call this method so that we can update the state according to the
   * RTP packet received. Subclasses can change behaviour by overriding doReceiveRtpPacket.
   * The base class method also sets the extended RTP sequence number on the RTP packet.
   */
  void onReceiveRtpPacket(const RtpPacket& packet, uint32_t uiRtpTimestampFrequency, bool &bIsRtcpSyncronised, boost::posix_time::ptime& tPresentation);


  /// Method to update local SSRC
  void onSendRtpPacket(const RtpPacket& packet);

  /**
   * @brief This method is called by onReceiveRtpPacket. This method is virtual so that subclasses can change
   * the behaviour e.g. parsing extension headers, etc.
   * Subclasses SHOULD call the base class method since this sets the extended RTP
   * sequence number of the packet
   */
  virtual void doReceiveRtpPacket(const RtpPacket& packet)
  {
    processRtpPacket(packet.getSequenceNumber(), packet.getRtpTimestamp(), packet.getRtpArrivalTimestamp());
  }

  // should be called once per compound packet using the SSRC of the SR or RR
  virtual void onReceiveRtcpPacket();
  /// 6.2.1 Entries MAY be deleted from
  // the table when an RTCP BYE packet with the corresponding SSRC
  // identifier is received, except that some straggler data packets might
  // arrive after the BYE and cause the entry to be recreated.  Instead,
  // the entry SHOULD be marked as having received a BYE and then deleted
  // after an appropriate delay ( Perkins recommends a fixed 2 second delay ).
  // Passing in current time so that it does not have to be recalculate for all members
  // Since a slight variation in duration is not critical
  bool isInactiveAndCanBeRemoved(boost::posix_time::ptime tNow) const;

  virtual void onSrReceived(const rfc3550::RtcpSr& rSr);
  virtual void onRrReceived(const rfc3550::RtcpRr& rRr);
  virtual void onByeReceived();
  virtual void onXrReceived(const rfc3611::RtcpXr& rXr);

  /**
   * This method searches for report blocks that are relevant to the local SSRC such as when an RR block
   * describes the path characteristics of the local sender. If it finds relevant blocks, it calls processReceiverReportBlock
   */
  void processReceiverReportBlocks(const uint32_t uiSSRC, uint64_t uiNtpArrivalTs, const std::vector<rfc3550::RtcpRrReportBlock>& vReports);
  void processReceiverReportBlock(const uint32_t uiReporterSSRC, uint64_t uiNtpArrivalTs, const rfc3550::RtcpRrReportBlock& rrBlock);

  /**
   * this MUST be called prior to RTCP RR generation to finalize RR fields
   */
  virtual void finaliseRRData();

  /**
   * finalise method for other data such as XR information
   */
  virtual void finaliseData();

  /**
   * @brief setUpdateCallback Setter for update callback
   * @param onUpdate Callback to be called when member updates occur
   */
  void setUpdateCallback(MemberUpdateCB_t onUpdate) { m_onUpdate = onUpdate; }

protected:

  /**
   * @brief Sets the participant to inactive. This will cause the participant to be
   * removed once the configured timeout has elapsed.
   *
   */
  void setInactive();

  /// RFC3550
  void init_seq(uint16_t seq);
  /// RFC3550
  int update_seq(uint16_t seq);

protected:

  /**
    * The local SSRC is needed to determine if a report from m_uiSSRC is relevant
    * to the local SSRC.
    */
  std::set<uint32_t> m_localSSRCs;
  /// SSRC of this member entry
  uint32_t m_uiSSRC;
  // Time the entry was created in the database
  boost::posix_time::ptime m_tEntryCreated;
  // Time BYE was received: once a BYE is received, wait a period before removing the record from the database
  boost::posix_time::ptime m_tMarkedInactive;
  // flag that stores whether init has been called
  bool m_bInit;
  // This variable stores how many sequential packets must be received before a source is regarded as valid
  uint32_t m_uiProbation;

  /// Stores whether the participant sent an RTP packet during the last two intervals
  bool m_bIsSender;

  /// RTCP Sync
  bool m_bIsRtcpSynchronised;

  bool m_bInactive; ///< Set if a BYE packet is received or if no packets have been received from partipants during the elapsed intervals
  /**
   * @summary This field is populated once an SDES packet is received
   */
  SdesInformation m_sdesInfo;

  uint32_t m_uiNoPacketReceivedIntervalCount; ///< Number of intervals in which no RTP or RTCP packets have been received.
  uint32_t m_uiRtpPacketsReceivedDuringLastInterval;	///< The rtp packets received during last interval
  uint32_t m_uiRtcpPacketsReceivedDuringLastInterval; ///< The rtcp packets received during last interval

  // From RFC3550
  uint16_t max_seq;        /* highest seq. number seen */
  uint32_t cycles;         /* shifted count of seq. number cycles */
  uint32_t base_seq;       /* base seq number */
  uint32_t bad_seq;        /* last 'bad' seq number + 1 */
  //uint32_t probation;      /* sequ. packets till source is valid */
  uint32_t received;       /* packets received */
  uint32_t expected_prior; /* packet expected at last interval */
  uint32_t received_prior; /* packet received at last interval */
  uint32_t transit;        /* relative trans time for prev pkt */
  uint32_t m_uiJitter;     /* estimated jitter */

  uint32_t m_uiMaxMisorder;

  // previous difference between RTP timestamp and RTP arrival timestamp
  int m_iPreviousDiff;
  // previous RTP timestamp
  uint32_t m_uiPreviousRtpTs;
  // previous RTP arrival timestamp
  uint32_t m_uiPreviousArrival;

  // in the case of a sender store the last SR extracted from the RR
  uint32_t m_uiLsr;
  // In the case of a sender, the last DLSR that was extracted from the RR
  uint32_t m_uiDlsr;
  // Store middle 32 bits of last sender report received
  uint32_t m_uiLsrRR;
  // For a receiver, this stores the last DLSR calculated
  uint32_t m_uiDlsrRR;

  uint32_t m_uiNtpArrival;

  // Number receiver has reported as lost
  uint32_t m_uiLost;
  // Loss fraction of receiver
  uint32_t m_uiLossFraction;
  double m_dRtt;

  /// This member stores the time at which the LSR was received
  boost::posix_time::ptime m_tLsr;
  boost::posix_time::time_duration m_tDlsr;

  /// Stores the time at which the last RTP packet was
  /// sent by the member (that was received successfully)
  /// and is used for tracking the number of senders
  boost::posix_time::ptime m_tLastRtpPacketSent;

  /// Stores the time at which the last RTP or RTCP packet
  /// was sent by the member (that was received successfully)
  /// and is used for timing out participants
  boost::posix_time::ptime m_tLastPacketSent;

  /// Reference times for RTCP synchronisation
  uint32_t m_uiSyncRtpRef;
  boost::posix_time::ptime m_tSyncRef;

  // for enabling loss reporting
  bool m_bEnableDetailedLossDetection;
  // loss estimator
  LossEstimator m_lossEstimator;

  MemberUpdateCB_t m_onUpdate;
};

}
}
