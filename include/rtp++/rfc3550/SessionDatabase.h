#pragma once
#include <cstdlib>
#include <tuple>
#include <unordered_map>
#include <cpputil/Utility.h>
#include <rtp++/RtpPacket.h>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/network/EndPoint.h>
#include <rtp++/rfc3550/MemberEntry.h>
#include <rtp++/rfc3550/Rtcp.h>
#include <rtp++/rfc3611/RtcpXr.h>


/// This class must be responsible for maintaining the database
/// about RTP and RTCP packets
/// - store outgoing RTP message state
/// - process incoming RTP messages
/// - assist in processing outgoing RTCP messages
/// - process incoming RTCP messages and provide valuable statistics regarding these
/// TODO: SSRC collisions are not currently handled

// -DDEBUG_RTCP
// #define DEBUG_RTCP

// -DDEBUG_RTCP_SCHEDULING
//#define DEBUG_RTCP_SCHEDULING

// -DDEBUG_SSRC_TIMEOUT: adds logging info for debugging SSRC timeouts
//#define DEBUG_SSRC_TIMEOUT
namespace rtp_plus_plus {

// fwd
class RtpSessionState;

namespace rfc3550 {

/// we_sent, sender list, receiver list
typedef std::tuple<bool, std::vector<MemberEntry*>, std::vector<MemberEntry*> > RtcpReportData_t;

class SessionDatabase
{
  typedef std::unordered_map<uint32_t, MemberEntry*> MemberMap_t;
  typedef std::pair<const uint32_t, MemberEntry*> MemberEntry_t;

public:
  typedef std::unique_ptr<SessionDatabase> ptr;
  /**
   * @brief create
   * @param rtpSessionState
   * @param rtpParameters
   * @return
   */
  static ptr create(RtpSessionState& rtpSessionState,
                    const RtpSessionParameters& rtpParameters);
  /**
   * @brief SessionDatabase
   * @param rtpSessionState
   * @param rtpParameters
   */
  SessionDatabase( RtpSessionState& rtpSessionState,
                   const RtpSessionParameters& rtpParameters);
  /**
   * @brief ~SessionDatabase
   */
  virtual ~SessionDatabase();
  /**
   * @brief This method checks if the specified SSRC
   * has passed the validation tests
   */

    bool isSourceValid(uint32_t uiSSRC) const;
    /**
     * @brief isSender
     * @param uiSSRC
     * @return
     */
    bool isSender(const uint32_t uiSSRC) const;
    /**
     * @brief isSourceSynchronised
     * @param uiSSRC
     * @return
     */
    bool isSourceSynchronised(const uint32_t uiSSRC) const;
    /**
     * @brief checkMemberDatabase This method checks for timed out members as well
     * as senders, that are no longer senders.
     * This method must be called once per RTCP interval.
     */
    void checkMemberDatabase();
    /**
     * This method completes the current RTCP interval, gather report data necessary for creating the RTCP report
     * and begins the next RTCP interval
     * @return A tuple consisting of whether the local participant was a sender, and a vector of member entries containing
     *          data about other senders, as well as a vector about other participants (receivers)
     */
    RtcpReportData_t gatherRTCPReportDataAndBeginNewInterval();
    /**
     * @brief getAverageRtcpSize
     * @return
     */
    double getAverageRtcpSize() const { return m_avg_rtcp_size; }
    /**
     * @brief lookupMemberEntryInSessionDb returns a pointer to the member entry if contained in the session database.
     * @param uiSSRC
     * @return
     */
    MemberEntry* lookupMemberEntryInSessionDb(uint32_t uiSSRC);
    /**
     * @fn  uint32_t SessionDatabase::getActiveMemberCount() const
     *
     * @brief Returns a count of the session members that are active. This excludes
     * 				participants that have not been validated and inactive participants.
     *
     * @return  The active member count.
     */
    uint32_t getActiveMemberCount() const;
    /**
     * @brief getInactiveMemberCount
     * @return
     */
    uint32_t getInactiveMemberCount() const;
    /**
     * @brief getUnvalidatedMemberCount
     * @return
     */
    uint32_t getUnvalidatedMemberCount() const;
    /**
     * @brief getTotalMemberCount
     * @return
     */
    uint32_t getTotalMemberCount() const;
    /**
     * Gets the count of participants that sent RTP packets during the last interval.
     *
     * @return  The sender count.
     */
    uint32_t getSenderCount() const;

    /// RTP and RTCP Events
    /**
     * @brief onSendRtpPacket
     * @param packet
     * @param ep
     */
    void onSendRtpPacket(const RtpPacket& packet, const EndPoint& ep);
    /**
     * @brief onSendRtcpPacket
     * @param compoundPacket
     * @param ep
     */
    void onSendRtcpPacket(const CompoundRtcpPacket& compoundPacket, const EndPoint& ep);
    /**
     * @brief processIncomingRtpPacket
     * @param packet
     * @param ep
     * @param uiRtpTimestampFrequency
     * @param bIsSSRCValid
     * @param bIsRtcpSyncronised
     * @param tPresentation
     */
    void processIncomingRtpPacket(const RtpPacket& packet, const EndPoint& ep,
                                  uint32_t uiRtpTimestampFrequency, bool& bIsSSRCValid,
                                  bool& bIsRtcpSyncronised, boost::posix_time::ptime& tPresentation);

    /**
     * @fn  void SessionDatabase::processIncomingRtcpPacket(const CompoundRtcpPacket& compoundPacket);
     *
     * @brief Process the incoming compound rtcp packet.
     * 				Pre-conditions:
     * 				- The incoming RTCP packet MUST have been validated according to the rules of the corresponding RFCs
     *        Post-conditions:
     *        - The associated SSRC of the sender attains a 'valid' status in the member database.
     *          This means that further RTP packets will consume system resources.
     * @param compoundPacket  A compound RTCP packet.
     */
    void processIncomingRtcpPacket(const CompoundRtcpPacket& compoundPacket, const EndPoint& ep);
    /**
     * @brief processOutgoingRtpPacket
     * @param packet
     */
    void processOutgoingRtpPacket(const RtpPacket& packet);
    /**
     * @fn  void SessionDatabase::processIncomingRtcpPacket(const RtcpPacketBase& packet);
     *
     * @brief Processes the incoming rtcp packet according to the packet type.
     * 				Pre-conditions:
     * 				- The incoming RTCP packet MUST have been validated according to the rules of the corresponding RFCs
     *        Post-conditions:
     *        - The associated SSRC of the sender attains a 'valid' status in the member database.
     *        - This method also sets the extended RTP sequence number field on the packet
     *
     * @param packet  The RTCP packet.
     */
    void processIncomingRtcpPacket(const RtcpPacketBase& packet, const EndPoint& ep);
    /**
     * @brief setUpdateCallback Setter for update callback
     * @param onUpdate Callback to be called when member updates occur
     */
    void setMemberUpdateCallback(MemberUpdateCB_t onUpdate) { m_onUpdate = onUpdate; }
    /**
     * @brief onMemberUpdate
     * @param memberUpdate
     */
    void onMemberUpdate(const MemberUpdate& memberUpdate)
    {
      if (m_onUpdate) m_onUpdate(memberUpdate);
    }
    /**
     * @brief updateSSRC This method should be called when the SSRC of the host
     * is updated. This could occur if an SSRC collision is detected, or in the case
     * of a translator/mixer when the SSRC is extracted from the 1st incoming packet
     * @param uiNewSSRC: the new SSRC
     */
    void updateSSRCs();

protected:

    /**
     * @brief initialiseLocalSSRCs: initialise local SSRCs that don't need to be validated.
     */
    void initialiseLocalSSRCs();
    /**
     * @brief This method checks whether the SSRC exists in the member database
     * @param uiSSRC The SSRC to be checked
     */
    bool isNewSSRC(uint32_t uiSSRC) const;
    /**
     * @brief This method determines if the passed in SSRC is from the local source
     * There might be multiple local SSRCs if e.g. retransmission via SSRC-multiplexing
     * is used
     */
    bool isOurSSRC(uint32_t uiSSRC) const;
    /**
     * @brief The SSRC is passed in as a parameter since we process SSRCs and CSRCS
     * @param uiSSRC The SSRC of the participant to be processed. This might differ from the SSRC in the RTP packet header.
     */
    void processParticipant(uint32_t uiSSRC, const RtpPacket& packet);

    virtual void doProcessIncomingRtcpPacket(const RtcpPacketBase& packet, const EndPoint &ep);

    void RFC3550_initialization();

    uint32_t calculateEstimatedRtcpSize() const;

    /**
     * Inserts a member entry into the database if the ssrc described by uiSSRC is new.
     *
     * @param uiSSRC  The SSRC of the new member entry
     * @return A pointer to the entry for that SSRC
     *
     */
    MemberEntry* insertSSRCIfNotInSessionDb(uint32_t uiSSRC);
    /**
     * @brief createMemberEntry Subclasses can override this to change the member entry type
     * @param uiSSRC
     * @return
     */
    virtual MemberEntry* createMemberEntry(uint32_t uiSSRC);
    /**
     * @brief handleOtherRtcpPacketTypes This virtual method allows new RTCP packet types to be handled by the session database
     * Subclasses should override this method to add new information to the RTCP database
     * @param rtcpPacket
     * @param ep
     */
    virtual void handleOtherRtcpPacketTypes(const RtcpPacketBase& rtcpPacket, const EndPoint& ep);

    void processSenderReport(const rfc3550::RtcpSr& sr);
    void processReceiverReport(const rfc3550::RtcpRr& rr);
    void processReceiverReportBlocks(MemberEntry *pEntry, uint32_t uiReporterSSRC, uint64_t uiNtpArrivalTs, const std::vector<rfc3550::RtcpRrReportBlock>& vReports);
    void processSdesReportBlocks(const std::vector<rfc3550::RtcpSdesReportBlock>& vReports);
    void processBye(const rfc3550::RtcpBye& bye);
    void processXrReport(const rfc3611::RtcpXr& xr);

protected:

    /// Session parameters
    RtpSessionState& m_rtpSessionState;
    const RtpSessionParameters& m_rtpParameters;
    // fraction of bandwidth used by RTCP
    uint32_t m_uiRtcpBandwidthFraction;

    // Tables for storing participants
    MemberMap_t m_memberDb;

    /* 6.3 */
    // Time the last RTCP report was sent
    boost::posix_time::ptime m_tp;
    // Time the 2nd previous RTCP report was sent
    boost::posix_time::ptime m_tp_prev;
    // Next scheduled transmission time
    boost::posix_time::ptime m_tn;
    // the estimated number of session members at the time m_tn
    // was last recomputed;
    uint32_t m_uiPrevMembers;
    // the most current estimate for the number of session
    // members
    uint32_t m_members;
    // the most current estimate for the number of senders in
    // the session
    uint32_t m_senders;
    // The target RTCP bandwidth, i.e., the total bandwidth
    // that will be used for RTCP packets by all members of this session,
    // in octets per second.  This will be a specified fraction of the
    // "session bandwidth" parameter supplied to the application at
    // startup.
    double m_rtcp_bw;
    // Flag that is true if the application has sent data
    // since the 2nd previous RTCP report was transmitted
    bool m_we_sent;
    // The average compound RTCP packet size, in octets,
    // over all RTCP packets sent and received by this participant.  The
    // size includes lower-layer transport and network protocol headers
    // (e.g., UDP and IP) as explained in Section 6.2.
    double m_avg_rtcp_size;
    // Flag that is true if the application has not yet sent
    // an RTCP packet
    bool m_initial;

    // transmission interval based on RTP session state
    double m_dTransmissionInterval;
    // configuration whether reduced min is allowed
    bool m_bUseReducedMinimum;

    bool m_bXrEnabled;

    // Callback for member updates
    MemberUpdateCB_t m_onUpdate;
};

}
}
