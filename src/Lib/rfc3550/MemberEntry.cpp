#include "CorePch.h"
#include <rtp++/rfc3550/MemberEntry.h>
#include <cpputil/Utility.h>
#include <rtp++/RtpTime.h>
#include <rtp++/rfc3550/Rtcp.h>

namespace rtp_plus_plus
{
namespace rfc3550
{

MemberEntry::MemberEntry()
  :m_uiSSRC(0),
  m_tEntryCreated(boost::posix_time::microsec_clock::universal_time()),
  m_bInit(false),
  m_uiProbation(MIN_SEQUENTIAL),
  m_bIsSender(false),
  m_bIsRtcpSynchronised(false),
  m_bInactive(false),
  m_uiNoPacketReceivedIntervalCount(0),
  m_uiRtpPacketsReceivedDuringLastInterval(0),
  m_uiRtcpPacketsReceivedDuringLastInterval(0),
  max_seq(0),
  cycles(0),
  base_seq(0),
  bad_seq(0),
  received(0),
  expected_prior(0),
  received_prior(0),
  transit(0),
  m_uiJitter(0),
  m_uiMaxMisorder(MAX_MISORDER),
  m_iPreviousDiff(0),
  m_uiPreviousRtpTs(0),
  m_uiPreviousArrival(0),
  m_uiLsr(0),
  m_uiDlsr(0),
  m_uiLsrRR(0),
  m_uiDlsrRR(0),
  m_uiNtpArrival(0),
  m_uiLost(0),
  m_uiLossFraction(0),
  m_dRtt(0.0),
  m_uiSyncRtpRef(0),
  m_bEnableDetailedLossDetection(true)
{

}

MemberEntry::MemberEntry(uint32_t uiSSRC)
  :m_uiSSRC(uiSSRC),
  m_tEntryCreated(boost::posix_time::microsec_clock::universal_time()),
  m_bInit(false),
  m_uiProbation(MIN_SEQUENTIAL),
  m_bIsSender(false),
  m_bIsRtcpSynchronised(false),
  m_bInactive(false),
  m_uiNoPacketReceivedIntervalCount(0),
  m_uiRtpPacketsReceivedDuringLastInterval(0),
  m_uiRtcpPacketsReceivedDuringLastInterval(0),
  max_seq(0),
  cycles(0),
  base_seq(0),
  bad_seq(0),
  received(0),
  expected_prior(0),
  received_prior(0),
  transit(0),
  m_uiJitter(0),
  m_uiMaxMisorder(MAX_MISORDER),
  m_iPreviousDiff(0),
  m_uiPreviousRtpTs(0),
  m_uiPreviousArrival(0),
  m_uiLsr(0),
  m_uiDlsr(0),
  m_uiLsrRR(0),
  m_uiDlsrRR(0),
  m_uiNtpArrival(0),
  m_uiLost(0),
  m_uiLossFraction(0),
  m_dRtt(0.0),
  m_uiSyncRtpRef(0),
  m_bEnableDetailedLossDetection(true)
{

}

MemberEntry::~MemberEntry()
{
  double dLossPercentage = ( received > 0 ) ? 100*getCumulativeNumberOfPacketsLost()/static_cast<double>(received) : 0.0;
  VLOG(5) << LOG_MODIFY_WITH_CARE
          << "[" << this << "]" << " Destructor: "
          << " base: " << base_seq << " cycles: " << cycles
          << " max: " << max_seq << " received: " << received
          << " lost: " << getCumulativeNumberOfPacketsLost()
          << " PLR: " << dLossPercentage;
}

void MemberEntry::init(const RtpPacket& rtpPacket)
{
  initSequence(rtpPacket.getSequenceNumber());
}

void MemberEntry::initSequence( uint16_t uiSequenceNumber )
{    
  // TODO: for now forward: torefactor
  init_seq(uiSequenceNumber);
  max_seq = uiSequenceNumber - 1;
  m_uiProbation = MIN_SEQUENTIAL;// already inited in constructor
  VLOG(2) << "[" << this << "]" << " Start SN: " << uiSequenceNumber << " Max: " << max_seq << " Probation: " << m_uiProbation;
  m_bInit = true;
}

void MemberEntry::updateSdesInfo( const SdesInformation& sdesInfo )
{
  /*
  New entries MAY be considered
  not valid until multiple packets carrying the new SSRC have been
  received (see Appendix A.1), or until an SDES RTCP packet containing
  a CNAME for that SSRC has been received.
  */
  if (!sdesInfo.getCName().empty())
  {
    m_sdesInfo.setCName(sdesInfo.getCName());
    // participant is now valid
    m_uiProbation = 0;
  }
  if (!sdesInfo.getName().empty())
    m_sdesInfo.setName(sdesInfo.getName());
  if (!sdesInfo.getEmail().empty())
    m_sdesInfo.setEmail(sdesInfo.getEmail());
  if (!sdesInfo.getPhone().empty())
    m_sdesInfo.setPhone(sdesInfo.getPhone());
  if (!sdesInfo.getLoc().empty())
    m_sdesInfo.setLoc(sdesInfo.getLoc());
  if (!sdesInfo.getTool().empty())
    m_sdesInfo.setTool(sdesInfo.getTool());
  if (!sdesInfo.getNote().empty())
    m_sdesInfo.setNote(sdesInfo.getNote());
  if (!sdesInfo.getPriv().empty())
    m_sdesInfo.setPriv(sdesInfo.getPriv());
}

int32_t MemberEntry::getCumulativeNumberOfPacketsLost() const
{
  /*
  The number of packets
  received is simply the count of packets as they arrive, including any
  late or duplicate packets.  The number of packets expected can be
  computed by the receiver as the difference between the highest
  sequence number received (s->max_seq) and the first sequence number
  received (s->base_seq).  Since the sequence number is only 16 bits
  and will wrap around, it is necessary to extend the highest sequence
  number with the (shifted) count of sequence number wraparounds
  (s->cycles).  Both the received packet count and the count of cycles
  are maintained the RTP header validity check routine in Appendix A.1.
  */
  //uint32_t uiExpected = max_seq - base_seq;
  uint32_t extended_max = cycles + max_seq;
  uint32_t expected = extended_max - base_seq + 1;

  int32_t iLost = expected - received;

#ifdef DEBUG_RTCP
  VLOG(2) << "ex max: " << extended_max << " cyc: " << cycles
             << " max_seq: " << max_seq << " expec: " << expected
             << " base_seq: " << base_seq << " recv: " << received
             << " lost: "<< iLost;
#endif

  // Cap value
  /*
  Since this signed number is carried in 24 bits, it should be clamped
  at 0x7fffff for positive loss or 0x800000 for negative loss rather
  than wrapping around.
  */
  if (iLost > 0x7FFFFF) iLost = 0x7FFFFF;
  if (iLost < -0x800000) iLost = 0x800000;

  return iLost;
}

uint8_t MemberEntry::getLostFraction()
{
  /*
  The fraction of packets lost during the last reporting interval
  (since the previous SR or RR packet was sent) is calculated from
  differences in the expected and received packet counts across the
  interval, where expected_prior and received_prior are the values
  saved when the previous reception report was generated
  */
  uint8_t fraction;
  uint32_t extended_max = cycles + max_seq;
  uint32_t expected = extended_max - base_seq + 1;
  uint32_t expected_interval = expected - expected_prior;
  expected_prior = expected;
  uint32_t received_interval = received - received_prior;
  received_prior = received;
  int32_t lost_interval = expected_interval - received_interval;

#ifdef DEBUG_RTCP
  VLOG(2) << "Calculating lost fraction: exp: " << expected << "=" << extended_max << "-" << base_seq << "+1"
          << " recv: " << received
          << " exp-int): " << expected_interval
          << " recv-int: " << received_interval;
#endif

  if (expected_interval == 0 || lost_interval <= 0)
    fraction = 0;
  else
    fraction = (lost_interval << 8) / expected_interval;
  return fraction;
}

uint32_t MemberEntry::getExtendedHighestSequenceNumber() const
{
  return cycles + max_seq;
}

void MemberEntry::newReportingInterval()
{
  // reset state for next interval
  m_uiRtcpPacketsReceivedDuringLastInterval = 0;
  m_uiRtpPacketsReceivedDuringLastInterval = 0;
}

void MemberEntry::onReceiveRtpPacket( const RtpPacket& packet, uint32_t uiRtpTimestampFrequency, bool &bIsRtcpSyncronised, boost::posix_time::ptime& tPresentation )
{
  // update extended sequence number
  RtpPacket& rtpPacket = const_cast<RtpPacket&>(packet);

  // NB: doReceiveRtpPacket must be called BEFORE the extended sequence number is set
  // since the cycles are calculated there
  doReceiveRtpPacket(packet);
  // NB: this is VERY important for insertion into the playout buffer!!!
  rtpPacket.setExtendedSequenceNumber(cycles | static_cast<uint32_t>(packet.getSequenceNumber()));

  // calculate presentation time of RTP packet
  // Before RTCP sync, we use wall clock as reference time
  if (m_tSyncRef.is_not_a_date_time())
  {
    m_uiSyncRtpRef = packet.getRtpTimestamp();
    m_tSyncRef = m_tLastRtpPacketSent;
  }

  tPresentation = RtpTime::getPresentationTime(m_uiSyncRtpRef, m_tSyncRef, packet.getRtpTimestamp(), uiRtpTimestampFrequency);

  bIsRtcpSyncronised = m_bIsRtcpSynchronised;

  // Save these as the new synchronization timestamp & time
  m_uiSyncRtpRef = rtpPacket.getRtpTimestamp();
  m_tSyncRef = tPresentation;
}

void MemberEntry::onSendRtpPacket( const RtpPacket& packet )
{
  m_tLastRtpPacketSent = boost::posix_time::microsec_clock::universal_time();
  m_tLastPacketSent = m_tLastRtpPacketSent;
  m_bIsSender = true;
}

void MemberEntry::processRtpPacket(uint16_t uiSequenceNumber, uint32_t uiRtpTs, uint32_t uiRtpArrivalTs)
{
  m_tLastRtpPacketSent = boost::posix_time::microsec_clock::universal_time();
  m_tLastPacketSent = m_tLastRtpPacketSent;
  m_bIsSender = true;

  ++m_uiRtpPacketsReceivedDuringLastInterval;
  update_seq(uiSequenceNumber);

  // TODO: revise loss estimator, profiling has shown it to be expensive
  if (m_bEnableDetailedLossDetection)
    m_lossEstimator.addSequenceNumber(uiSequenceNumber);

  // jitter calculation
  if (received > 1 && uiRtpTs != m_uiPreviousRtpTs)
  {
#ifdef DEBUG_JITTER_CALC
    DLOG(INFO) << "RTP packet: Arrival: " << uiRtpArrivalTs << " RTP TS: " << uiRtpTs << "Diff: " << m_iPreviousDiff;
    int iDiffArrival = uiRtpArrivalTs - m_uiPreviousArrival;
    int iDiffRtp = uiRtpTs - m_uiPreviousRtpTs;
    DLOG(INFO) << "RTP packet: Prev Arrival: " << m_uiPreviousArrival << " Diff: " << iDiffArrival <<" Prev RTP TS: " << m_uiPreviousRtpTs << " Diff: " << iDiffRtp;
#endif
    // store info for next receive
    int iCurrentDiff = uiRtpArrivalTs - uiRtpTs ;
    int iDiff = iCurrentDiff - m_iPreviousDiff;
#ifdef DEBUG_JITTER_CALC
    DLOG(INFO) << "Prev Diff: " << m_iPreviousDiff << " Curr diff: " << iCurrentDiff << " D(i,j): " << iDiff;
#endif
    int adjustment = ((std::abs(iDiff) - (int)m_uiJitter)/16);
    m_uiJitter = m_uiJitter + adjustment;
#ifdef DEBUG_JITTER_CALC
    DLOG(INFO) << "Jitter: " <<  jitter << " Adjustment: " << adjustment;
#endif

    // update previous for next calculation
    m_iPreviousDiff = iCurrentDiff;
    m_uiPreviousRtpTs = uiRtpTs;
    m_uiPreviousArrival = uiRtpArrivalTs;
  }
  else
  {
    // store info for next receive
    m_iPreviousDiff = uiRtpArrivalTs - uiRtpTs;
    m_uiPreviousRtpTs = uiRtpTs;
    m_uiPreviousArrival = uiRtpArrivalTs;
#ifdef DEBUG_JITTER_CALC
    DLOG(INFO) << "First RTP packet: RTP Arrival: " << uiRtpArrivalTs << " RTP TS: " << uiRtpTs << " Diff: " << m_iPreviousDiff;
#endif
  }
}

void MemberEntry::onReceiveRtcpPacket()
{
  m_tLastPacketSent = boost::posix_time::microsec_clock::universal_time();
  ++m_uiRtcpPacketsReceivedDuringLastInterval;
}

bool MemberEntry::isInactiveAndCanBeRemoved( boost::posix_time::ptime tNow ) const
{
  // A participant can be removed of
  // - he has been inactive for more than BYE_TIMEOUT_SECONDS
  // - the last RTP or RTCP packet was sent more than
  return m_bInactive && ((tNow - m_tMarkedInactive).total_seconds() >= (int64_t)BYE_TIMEOUT_SECONDS);
}

void MemberEntry::onSrReceived( const rfc3550::RtcpSr& rSr )
{
  onReceiveRtcpPacket();

#ifdef DEBUG_RTCP
  VLOG(2) << "SR received: " << hex(m_uiSSRC);
#endif

  // Save these as the new synchronization timestamp & time
  m_bIsRtcpSynchronised = true;
  m_uiSyncRtpRef = rSr.getRtpTimestamp();
  m_tSyncRef = RtpTime::convertNtpTimestampToPosixTime(rSr.getNtpTimestampMsw(), rSr.getNtpTimestampLsw());

  m_bIsSender = true;

  m_tLsr = boost::posix_time::microsec_clock::universal_time();
  // update time last SR received
  uint32_t uiNtpMsw = rSr.getNtpTimestampMsw();
  uint32_t uiNtpLsw = rSr.getNtpTimestampLsw();

#ifdef DEBUG_RTT
  uint32_t uiNtpArrivalMsw = 0, uiNtpArrivalLsw = 0;
  split(rSr.getArrivalTimeNtp(), uiNtpArrivalMsw, uiNtpArrivalLsw);
  LOG(INFO) << "DEBUG_RTT: SR received " << " from SSRC " << hex(rSr.getSSRC())
            << " at NTP " << hex(uiNtpArrivalMsw) << "." << hex(uiNtpArrivalLsw)
            << " " << convertNtpTimestampToPosixTime(rSr.getArrivalTimeNtp())
            << " with SR NTP " << hex(uiNtpMsw) << "." << hex(uiNtpLsw)
            << " " << convertNtpTimestampToPosixTime(uiNtpMsw, uiNtpLsw)
            << " m_tLsr: " << m_tLsr;

#endif

  m_uiLsr = ((uiNtpMsw & 0xFFFF) << 16);
  m_uiLsr |= ((uiNtpLsw >> 16) &  0xFFFF);

  // trying to move handling of SR reports to inside this class from SessionDatabase
  const std::vector<rfc3550::RtcpRrReportBlock>& vReports = rSr.getReceiverReportBlocks();
  VLOG(10) << "Received SR with " << vReports.size() << " receiver report blocks";
  processReceiverReportBlocks(rSr.getSSRC(), rSr.getArrivalTimeNtp(), vReports);
}

void MemberEntry::onXrReceived(const rfc3611::RtcpXr& rXr)
{
  onReceiveRtcpPacket();
  // allow subclasses to implement this
}

uint32_t MemberEntry::getDlsr() const
{
  return m_uiDlsr;
}

void MemberEntry::finaliseRRData()
{
  // this value is only calculated if an SR has been received
  if (!m_tLsr.is_not_a_date_time())
  {
    const boost::posix_time::ptime tNow =  boost::posix_time::microsec_clock::universal_time();
    m_tDlsr = tNow - m_tLsr;
  }
  else
  {
    m_tDlsr = boost::posix_time::seconds(0);
  }
#ifdef DEBUG_RTT
  DLOG(INFO) << "[" << this << "]" << "finalising RR data: LSR: " << hex(m_uiLsr) << " " << m_tLsr;
#endif

  // calculate time elapsed in seconds
  uint32_t uiSeconds = static_cast<uint32_t>(m_tDlsr.total_seconds());
  uint32_t uiMicroseconds = static_cast<uint32_t>(m_tDlsr.fractional_seconds());

  double dDelay = (double)uiSeconds + uiMicroseconds/1000000.0;
  m_uiDlsr = RtpTime::convertDelaySinceLastSenderReportToDlsr(dDelay);

#ifdef DEBUG_RTT
  DLOG(INFO) << "[" << this << "] SSRC: " << hex(m_uiSSRC)
             << " DLSR: " << hex(m_uiDlsr) << " = "
             << m_tDlsr.total_milliseconds() << "ms ("
             << dDelay <<  "s) Last SR received: "
             << m_tLsr;
#endif

  if (m_bEnableDetailedLossDetection)
  {
    auto losses = m_lossEstimator.getEstimatedLostSequenceNumbers();
    if (!losses.empty())
    {
      std::ostringstream ostr;
      std::for_each(losses.begin(), losses.end(), [&ostr](uint16_t uiSN)
      {
        ostr << uiSN << " ";
      });
      VLOG(2) << "RTP sequence number losses: " << ostr.str();
    }
  }
}

void MemberEntry::finaliseData()
{
  // Subclasses can implement this as required
}

void MemberEntry::processReceiverReportBlocks(const uint32_t uiReporterSSRC, uint64_t uiNtpArrivalTs, const std::vector<rfc3550::RtcpRrReportBlock>& vReports)
{
  std::for_each(vReports.begin(), vReports.end(), [this, uiReporterSSRC, uiNtpArrivalTs](const rfc3550::RtcpRrReportBlock& rrBlock)
  {
    // only update local info if the block applies to the local SSRC!
    if (isRelevantToParticipant(rrBlock.getReporteeSSRC()))
    {
      // calculate RTT
      // DLOG(INFO) << "Updating LSR 1";
      processReceiverReportBlock(uiReporterSSRC, uiNtpArrivalTs, rrBlock);
    }
  });
}

void MemberEntry::processReceiverReportBlock(const uint32_t uiReporterSSRC, uint64_t uiNtpArrivalTs, const rfc3550::RtcpRrReportBlock &rrBlock)
{
  m_uiLsrRR = rrBlock.getLastSr();
  if (m_uiLsrRR == 0) return;

  m_uiNtpArrival = static_cast<uint32_t>((uiNtpArrivalTs >> 16) & 0xFFFFFFFF);
  m_uiDlsrRR = rrBlock.getDelaySinceLastSr();
  uint32_t uiRtt = 0;
  int32_t iRtt = m_uiNtpArrival - m_uiDlsrRR - m_uiLsrRR;

  if (iRtt >= 0)
  {
    uiRtt = static_cast<uint32_t>(iRtt);
  }
  else
    uiRtt = 0;

  m_uiJitter = rrBlock.getInterarrivalJitter();
  m_uiLost = rrBlock.getCumulativeNumberOfPacketsLost();
  m_uiLossFraction = rrBlock.getFractionLost();
  m_dRtt = uiRtt/65536.0;

  VLOG(1) << "[" << this << "]"
          << " RR Reporter SSRC: " << hex(uiReporterSSRC)
          << " Reportee: "  << hex(rrBlock.getReporteeSSRC())
          << " Arrival: " << hex(m_uiNtpArrival)
          << " LSR: " << hex(m_uiLsrRR)
          << " DLSR: " << hex(m_uiDlsrRR)
          << " (" << RtpTime::convertDlsrToSeconds(rrBlock.getDelaySinceLastSr()) << "s)"
          << " RTT: " << hex(iRtt) << " unsigned: " << hex(uiRtt) << " ( " << m_dRtt << "s )"
          << " Jitter: " << hex(rrBlock.getInterarrivalJitter())
          << " Lost: " << hex(rrBlock.getCumulativeNumberOfPacketsLost())
          << " Frac: " << hex(rrBlock.getFractionLost());

  MemberUpdate update(m_uiSSRC, m_dRtt, m_uiJitter, m_uiLost, m_uiLossFraction, rrBlock.getExtendedHighestSNReceived());

  if (m_onUpdate)
  {
    m_onUpdate(update);
  }

}

void MemberEntry::onRrReceived( const rfc3550::RtcpRr& rRr )
{
  onReceiveRtcpPacket();

#ifdef DEBUG_RTT
    uint32_t uiNtpMsw = 0, uiNtpLsw = 0;
    split(rRr.getArrivalTimeNtp(), uiNtpMsw, uiNtpLsw);
//    getNTPTimeStamp(uiNtpMsw, uiNtpLsw);
    LOG(INFO) << "DEBUG_RTT: RR received at NTP " << hex(uiNtpMsw) << "." << hex(uiNtpLsw)
              <<  " " << convertNtpTimestampToPosixTime(rRr.getArrivalTimeNtp()) << " from SSRC " << hex(rRr.getReporterSSRC());
#endif

  // update jitter and RTT calculations
  processReceiverReportBlocks(rRr.getReporterSSRC(), rRr.getArrivalTimeNtp(), rRr.getReceiverReportBlocks());
}

void MemberEntry::onByeReceived()
{
  onReceiveRtcpPacket();

  // mark participant as inactive for later removal if participant has not already been marked as such
  if (!m_bInactive)
    setInactive();
}

void MemberEntry::setInactive()
{
  m_bInactive = true;
  DLOG(INFO) << "SSRC " << hex(m_uiSSRC) << " marked inactive";
  m_tMarkedInactive = boost::posix_time::microsec_clock::universal_time();
}

void MemberEntry::init_seq( uint16_t seq )
{
#if 0
  VLOG(2) << "MemberEntry::init_seq: " << seq;
#endif
  base_seq = seq;
  max_seq = seq;
  bad_seq = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
  cycles = 0;
  received = 0;
  received_prior = 0;
  expected_prior = 0;
  /* other initialization */
#ifdef DEBUG_MEMBER_VALIDATION_ALGORITHM
  DLOG(INFO) << "base_seq: " << seq << " max_seq: " << max_seq << " bad_seq: " << m_uiProbation;
#endif
}

int MemberEntry::update_seq( uint16_t seq )
{
#if 0
  VLOG(2) << "MemberEntry::update_seq: " << seq;
#endif
  uint16_t udelta = seq - max_seq;
#ifdef DEBUG_MEMBER_VALIDATION_ALGORITHM
  DLOG(INFO) << "seq: " << seq << " max_seq: " << max_seq << " udelta: " << udelta;
#endif
  /*
  * Source is not valid until MIN_SEQUENTIAL packets with
  * sequential sequence numbers have been received.
  */

  if (m_uiProbation)
  {
    /* packet is in sequence */
    if (seq == max_seq + 1) 
    {
#ifdef DEBUG_MEMBER_VALIDATION_ALGORITHM
      DLOG(INFO) << "In sequence: prob: " << m_uiProbation);
#endif
      m_uiProbation--;
      max_seq = seq;
      if (m_uiProbation == 0) 
      {       
#ifdef DEBUG_MEMBER_VALIDATION_ALGORITHM
        DLOG(INFO) << "Init!";
#endif
        init_seq(seq);
        received++;
        return 1;
      }
    } 
    else
    {
#ifdef DEBUG_MEMBER_VALIDATION_ALGORITHM
      DLOG(INFO) << "Not in sequence: prob: " << m_uiProbation;
#endif
      m_uiProbation = MIN_SEQUENTIAL - 1;
      max_seq = seq;
    }
    return 0;
  } 
  else if (udelta < MAX_DROPOUT)
  {
#ifdef DEBUG_MEMBER_VALIDATION_ALGORITHM
    DLOG(INFO) << "udelta < MAX_DROPOUT: " << udelta << " < " << MAX_DROPOUT;
#endif
    /* in order, with permissible gap */
    if (seq < max_seq) {
      /*
      * Sequence number wrapped - count another 64K cycle.
      */
      cycles += RTP_SEQ_MOD;
      VLOG(2) << "SN wrap: Seq: " << seq << " max seq: " << max_seq ;
    }
    max_seq = seq;
  } 
  else if (udelta <= RTP_SEQ_MOD - m_uiMaxMisorder)
  {
    VLOG(5) << "udelta <= RTP_SEQ_MOD - m_uiMaxMisorder: "
            << udelta << "(" << seq << "-" << max_seq
            << ") <= " << RTP_SEQ_MOD<< " - " << m_uiMaxMisorder;

    /* the sequence number made a very large jump */
    if (seq == bad_seq) 
    {
      // part of normal operation
      LOG(INFO) << "Reinitialising sequence number - bad SN: " << seq;
      /*
      * Two sequential packets -- assume that the other side
      * restarted without telling us so just re-sync
      * (i.e., pretend this was the first packet).
      */
      init_seq(seq);
    }
    else 
    {
      bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
      // part of normal operation
      LOG(INFO) << "Bad SN: " << bad_seq;
      return 0;
    }
  } 
  else 
  {
    /* duplicate or reordered packet */
    VLOG(3) << "Dup or reordered packet SN: " << seq;
  }
  received++;
  return 1;
}

} // rfc3550
} // rtp_plus_plus
