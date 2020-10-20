#include "CorePch.h"
#include <rtp++/experimental/Nada.h>
#include <rtp++/TransmissionManager.h>
#include <rtp++/experimental/ExperimentalRtcp.h>

namespace rtp_plus_plus
{
namespace experimental
{

const std::string NADA = "nada";

bool supportsNadaFb(const RtpSessionParameters& rtpParameters)
{
  return rtpParameters.supportsFeedbackMessage(NADA);
}

#define PRIO  1.0 	    //       | Weight of priority of the flow   |    1.0
                        //       | supported by media encoder       |                |
#define X_REF_ms 20.0   //       | Reference congestion level       |    20ms        |
#define KAPPA 0.5       //       | Scaling parameter for gradual    |    0.5         |
                        //       | rate update calculation          |                |
#define ETA   2.0       //       | Scaling parameter for gradual    |    2.0         |
                        //       | rate update calculation          |                |
#define TAU_ms   500.0  //       | Upper bound of RTT in gradual    |    500ms       |
                        //       | rate update calculation          |                |
#define DELTA_ms 100    //       | Target feedback interval         |    100ms       |
#define LOGWIN_ms 500   //       | Observation window in time for   |    500ms       |
                        //       | calculating packet summary       |                |
                        //       | statistics at receiver           |                |
#define QEPS_ms 10      //       | Threshold for determining queuing|     10ms       |
                        //       | delay build up at receiver       |                |
#define QTH_ms 100      //       | Delay threshold for non-linear   |    100ms       |
                        //       | warping                          |                |
#define QMAX_ms 400     //       | Delay upper bound for non-linear |    400ms       |
                        //       | warping                          |                |
#define DLOSS_s 1       //       | Delay penalty for loss           |    1.0s        |
#define DMARK_ms 200    //       | Delay penalty for ECN marking    |    200ms       |
#define GAMMA_MAX 0.2   //       | Upper bound on rate increase     |     20%        |
                        //       | ratio for accelerated ramp-up    |                |
#define QBOUND_ms 50.0  //       | Upper bound on self-inflicted    |    50ms        |
                        //       | queuing delay during ramp up     |                |
#define FPS 30          //       | Frame rate of incoming video     |     30         |
#define BETA_S 0.1      //       | Scaling parameter for modulating |    0.1         |
                        //       | outgoing sending rate            |                |
#define BETA_V 0.1      //       | Scaling parameter for modulating |    0.1         |
                        //       | video encoder target rate        |                |
#define ALPHA 0.1       //       | Smoothing factor in exponential  |    0.1         |
                        //       | smoothing of packet loss and     |                |
                        //       | marking ratios                   |

NadaSender::NadaSender(TransmissionManager& transmissionManager, uint32_t uiFps, uint32_t uiMinRateKbps, uint32_t uiMaxRateKbps)
  :m_transmissionManager(transmissionManager),
    m_uiFps(uiFps),
    m_uiMinRateKbps(uiMinRateKbps),
    m_uiMaxRateKbps(uiMaxRateKbps),
    m_uiMinNetworkRateKbps(0.7 * m_uiMinRateKbps), // RG: try linking min and max network rates to rate settings
    m_uiMaxNetworkRateKbps(10 * m_uiMaxRateKbps),
    t_last_(boost::posix_time::microsec_clock::universal_time()),
    t_curr_(t_last_),
    delta_(0),
    r_n_(uiMinRateKbps),
    r_send_(uiMinRateKbps),
    r_recv_(0),
    r_vin_(uiMinRateKbps),
    r_vout_(uiMinRateKbps),
    rtt_(0),
    buffer_len_(0),
    x_n_(0),
    x_prev_(0),
    x_diff_(0)
{
  VLOG(2) << "NadaSender FPS: " << m_uiFps << " Min rate: " << m_uiMinRateKbps << "kbps Max rate: " << m_uiMaxRateKbps << "kbps";
  /*
   The operation of the live video encoder is out of the scope of the
   design for the congestion control scheme in NADA.  Instead, its
   behavior is treated as a black box.

   A rate shaping buffer is employed to absorb any instantaneous
   mismatch between encoder rate output r_vout and regulated sending
   rate r_send.  Its current level of occupancy is measured in bytes and
   is denoted as buffer_len.

   The operation of the live video encoder is out of the scope of the
   design for the congestion control scheme in NADA.  Instead, its
   behavior is treated as a black box.

   A rate shaping buffer is employed to absorb any instantaneous
   mismatch between encoder rate output r_vout and regulated sending
   rate r_send.  Its current level of occupancy is measured in bytes and
   is denoted as buffer_len.
   */

  /*
   The target rate for the live video encoder deviates from the network
   congestion control rate r_n based on the level of occupancy in the
   rate shaping buffer:

       r_vin = r_n - BETA_V*8*buffer_len*FPS.     (11)

   The actual sending rate r_send is regulated in a similar fashion:

       r_send = r_n + BETA_S*8*buffer_len*FPS.    (12)

   In (11) and (12), the first term indicates the rate calculated from
   network congestion feedback alone.  The second term indicates the
   influence of the rate shaping buffer.  A large rate shaping buffer
   nudges the encoder target rate slightly below -- and the sending rate
   slightly above -- the reference rate r_n.

   Intuitively, the amount of extra rate offset needed to completely
   drain the rate shaping buffer within the duration of a single video
   frame is given by 8*buffer_len*FPS, where FPS stands for the frame
   rate of the video.  The scaling parameters BETA_V and BETA_S can be
   tuned to balance between the competing goals of maintaining a small
   rate shaping buffer and deviating the system from the reference rate
   point.
   */

}

uint32_t NadaSender::calculateRateKbps(const experimental::RtcpNadaFb& nadaFb, uint32_t buffer_len, uint32_t& r_vin, uint32_t& r_send)
{
  buffer_len_ = buffer_len;
  /*
  obtain current timestamp: t_curr
  obtain values of rmode, x_n, and r_recv from feedback report
  update estimation of rtt
  measure feedback interval: delta = t_curr - t_last
  if rmode == 0:
   update r_n following accelerated ramp-up rules
  else:
   update r_n following gradual update rules
  clip rate r_n within the range of [RMIN, RMAX]
  x_prev = x_n
  t_last = t_curr
  */
  // update estimation of RTT: does this need to be filtered?
  double dRtt = m_transmissionManager.getLatestPathInfo().getRoundTripTime();
  rtt_ = static_cast<uint32_t>(dRtt * 1000);

  rmode_ = nadaFb.get_rmode();
  x_n_ = nadaFb.get_x_n();
  r_recv_ = nadaFb.get_r_recv();
  t_curr_ = boost::posix_time::microsec_clock::universal_time();
  delta_ = (t_curr_ - t_last_).total_milliseconds();

  switch (rmode_)
  {
    case ACCELERATED_RAMP_UP:
    {
      gamma_ = std::min(GAMMA_MAX, QBOUND_ms/(rtt_+DELTA_ms));
      r_n_  =  (1 + gamma_) * r_recv_;
      VLOG(2) << "NadaSender::calculateRateKbps RTT: FB (mode: ACCELERATED_RAMP_UP "
                 << " x_n: " << x_n_
                 << " r_recv: " << r_recv_
                 << ") delta: " << delta_
                 << " RTT: " << rtt_
                 << " gamma: " << gamma_
                 << " r_n: " << r_n_;
      /*
       The rate increase multiplier gamma is calculated as a function of
       upper bound of self-inflicted queuing delay (QBOUND), round-trip-time
       (rtt), and target feedback interval DELTA.  It has a maximum value of
       GAMMA_MAX.  The rationale behind (3)-(4) is that the longer it takes
       for the sender to observe self-inflicted queuing delay build-up, the
       more conservative the sender should be in increasing its rate, hence
       the smaller the rate increase multiplier.
       */
      break;
    }
    case GRADUAL_RATE_UPDATE:
    {
      int64_t x_offset_2 = PRIO*X_REF_ms*RMAX_kbps/r_n_;
      int64_t x_offset = x_n_ - x_offset_2;
      x_diff_   = x_n_ - x_prev_;

      int64_t contr_x_offset = (KAPPA* (delta_/TAU_ms)*(x_offset/TAU_ms) * r_n_);
      int64_t contr_x_diff =  ( KAPPA*ETA*(x_diff_/TAU_ms) * r_n_);
      // underflow
      if (contr_x_offset + contr_x_diff <= r_n_)
      {
        r_n_ = r_n_ - contr_x_offset - contr_x_diff;
      }
      else
      {
        r_n_ = 0;
      }

      VLOG(2) << "NadaSender::calculateRateKbps RTT: FB (mode: GRADUAL_RATE_UPDATE "
              << " x_n: " << x_n_
              << " r_recv: " << r_recv_
              << ") delta: " << delta_
              << " RTT: " << rtt_
              << " x_offset: " << x_offset << " PRIO*X_REF_ms*RMAX_kbps/r_n: "<< x_offset_2
              << " x_diff: " << x_diff_ << " x_prev: " << x_prev_
              << " contr(x_offset): " << contr_x_offset
              << " contr(x_diff): " << contr_x_diff
              << " r_n: " << r_n_  ;

      /*
         The rate changes in proportion to the previous rate decision.  It is
         affected by two terms: offset of the aggregate congestion signal from
         its value at equilibrium (x_offset) and its change (x_diff).
         Calculation of x_offset depends on maximum rate of the flow (RMAX),
         its weight of priority (PRIO), as well as a reference congestion
         signal (X_REF).  The value of X_REF is chosen that the maximum rate
         of RMAX can be achieved when the observed congestion signal level is
         below PRIO*X_REF.

         At equilibrium, the aggregated congestion signal stablizes at x_n =
         PRIO*X_REF*RMAX/r_n.  This ensures that when multiple flows share the
         same bottleneck and observe a common value of x_n, their rates at
         equilibrium will be proportional to their respective priority levels
         (PRIO) and maximum rate (RMAX).
         */

      break;
    }
  }

  // clipping
  r_n_ = std::min(r_n_, m_uiMaxRateKbps);
  r_n_ = std::max(r_n_, m_uiMinRateKbps);

  // encoder rate
  uint32_t vin = BETA_V*8*buffer_len_* m_uiFps;
  // clipping
  if (r_n_ > vin)
  {
    r_vin_ = r_n_ - vin;
    r_vin_ = std::max(r_vin_, static_cast<uint32_t>((1 - BETA_V)*r_n_)); // linking to current rate
  }
  else
  {
    r_vin_ = static_cast<uint32_t>((1 - BETA_V)*r_n_); // linking to current rate
  }

  // network rate
  uint32_t send = BETA_S * 8 * buffer_len_ * m_uiFps;
  r_send_ = r_n_ + send;
  // clipping
#if 0
  r_send_ = std::min(r_send_, static_cast<uint32_t>((1 + BETA_S) * r_n_)); // linking to current rate
#else
  r_send_ = std::min(r_send_, m_uiMaxNetworkRateKbps); // linking to max network rate
#endif
  r_vin = r_vin_;
  r_send = r_send_;
  VLOG(2) << "NADA buffer_len: " << buffer_len_ << " RTT: " << rtt_ << "ms rmode: " << rmode_
          << " delta: " << delta_ << " r_n: " << r_n_ << " vin: " << vin << " send: " << send
          << " r_vin: " << r_vin_ << " rsend: " << r_send_;

  x_prev_ = x_n_;
  t_last_ = t_curr_;

  return r_n_;
}


NadaReceiver::NadaReceiver()
  : rmode_(ACCELERATED_RAMP_UP),
    t_last_(boost::posix_time::microsec_clock::universal_time()),
    t_curr_(t_last_),
    r_recv_(0),
    d_base_(UINT32_MAX),
    d_fwd_(0),
    d_n_(0),
    d_tilde_(0),
    p_mark_(0),
    p_loss_(0),
    x_n_(0),
    x_prev_(0),
    x_diff_(0)
{
  QMAX_MINUS_QTH_POW4 = pow(QMAX_ms - QTH_ms, 4);
}

void NadaReceiver::updateLogWindow(const boost::posix_time::ptime& tNow)
{
  boost::posix_time::ptime tStartOfLogWin = tNow - boost::posix_time::milliseconds(LOGWIN_ms);
  for (auto it = m_qArrivingPackets.begin(); it != m_qArrivingPackets.end(); /**/)
  {
   if (it->getArrivalTime() < tStartOfLogWin)
   {
     it = m_qArrivingPackets.erase(it); // c++11 erase
   }
   else
   {
     // ++it;
     break; // we can break early here since we can assume that the packets are sorted by arrival time
   }
  }
}

void NadaReceiver::receivePacket(const RtpPacket& rtpPacket)
{
  t_curr_ = boost::posix_time::microsec_clock::universal_time();
  // obtain current timestamp t_curr
  // obtain from packet header sending time stamp t_sent
  // obtain one-way delay measurement: d_fwd = t_curr - t_sent
  d_fwd_ = static_cast<uint32_t>(rtpPacket.getOwdSeconds() * 1000);
  // update baseline delay: d_base = min(d_base, d_fwd)
  d_base_ = std::min(d_base_, d_fwd_);
  // update queuing delay:  d_n = d_fwd - d_base
  d_n_ = d_fwd_ - d_base_;
  // update packet loss ratio estimate p_loss
  VLOG(2) << "NadaReceiver::receivePacket delays: SN: " << rtpPacket.getExtendedSequenceNumber() << " d_fwd: " << d_fwd_ << " d_base: " << d_base_ << " d_n: " << d_n_;

  m_qArrivingPackets.push_back(rtpPacket);
  updateLogWindow(t_curr_);

  double p_inst = calculateLostOrReorderedPacketsRatio();
  p_loss_ = ALPHA*p_inst + (1-ALPHA)*p_loss_;
  // update packet marking ratio estimate p_mark
  // N/A

  // update measurement of receiving rate r_recv
  r_recv_ = calculateReceivingRateKbps(t_curr_);

  /*
   The delay estimation process in NADA follows a similar approach as in
   earlier delay-based congestion control schemes, such as LEDBAT
   [RFC6817].  NADA estimates the forward delay as having a constant
   base delay component plus a time varying queuing delay component.
   The base delay is estimated as the minimum value of one-way delay
   observed over a relatively long period (e.g., tens of minutes),
   whereas the individual queuing delay value is taken to be the
   difference between one-way delay and base delay.

   The individual sample values of queuing delay should be further
   filtered against various non-congestion-induced noise, such as spikes
   due to processing "hiccup" at the network nodes.  Current
   implementation employs a 15-tab minimum filter over per-packet
   queuing delay estimates.
   */

  /*
   The receiver detects packet losses via gaps in the RTP sequence
   numbers of received packets.  Packets arriving out-of-order are
   discarded, and count towards losses.  The instantaneous packet loss
   ratio p_inst is estimated as the ratio between the number of missing
   packets over the number of total transmitted packets within the
   recent observation window LOGWIN.  The packet loss ratio p_loss is
   obtained after exponential smoothing:

       p_loss = ALPHA*p_inst + (1-ALPHA)*p_loss.   (10)

   The filtered result is reported back to the sender as the observed
   packet loss ratio p_loss.

   Estimation of packet marking ratio p_mark follows the same procedure
   as above.  It is assumed that ECN marking information at the IP
   header can be passed to the transport layer by the receiving
   endpoint.
   */

  /*
   It is fairly straighforward to estimate the receiving rate r_recv.
   NADA maintains a recent observation window with time span of LOGWIN,
   and simply divides the total size of packets arriving during that
   window over the time span.  The receiving rate (r_recv) is included
   as part of the feedback report.
   */
}

bool NadaReceiver::generateFeedback( uint32_t& rmode, int64_t& x_n, uint32_t& r_recv)
{
  t_curr_ = boost::posix_time::microsec_clock::universal_time();
  // On time to send a new feedback report (t_curr - t_last > DELTA):
  if ((t_curr_ - t_last_).total_milliseconds() < DELTA_ms)
  {
    return false;
  }

  // calculate non-linear warping of delay d_tilde if packet loss exists during window
  double dLossRatio = calculateLostOrReorderedPacketsRatio();
  if (dLossRatio> 0.0)
  {
    // nonLinearWarpingOfQueingDelay
    d_tilde_ = (d_n_ < QTH_ms) ?
          d_n_ :
          ( (d_n_ < QMAX_ms) ?
              (QTH_ms * ((QMAX_ms - d_n_)/static_cast<double>(QMAX_MINUS_QTH_POW4))):
              0);
    VLOG(2) << "NadaReceiver::generateFeedback non-linear warping delay: loss ratio: " << dLossRatio << " d_tilde: " << d_tilde_ << " d_n: " << d_n_;
  }
  // calculate aggregate congestion signal x_n
  x_n_ = aggregateCongestionSignal();
  x_n = x_n_;

  // determine mode of rate adaptation for sender: rmode
  if (p_loss_ > 0.0 || buildUpOfQueuingDelayOverWindow())
  {
    rmode = rmode_ = GRADUAL_RATE_UPDATE;
  }
  else
  {
    rmode = rmode_ = ACCELERATED_RAMP_UP;
  }

  // updated on packet receive
  r_recv = r_recv_;
  // send RTCP feedback report containing values of: rmode, x_n, and r_recv
  // update t_last = t_curr
  t_last_ = t_curr_;
  return true;
}


bool isSNGreaterThan(uint32_t uiSN1, uint32_t uiSN2)
{
  return ((uiSN1 - uiSN2) < (UINT32_MAX >> 1));
}

inline double NadaReceiver::calculateLostOrReorderedPacketsRatio()
{
  if (m_qArrivingPackets.empty()) return 0.0;

  auto it = m_qArrivingPackets.begin();
  uint32_t uiPrevSN = it->getSequenceNumber();
  uint32_t uiLoss = 0;
  for (++it ; it != m_qArrivingPackets.end(); ++it)
  {
    if (it->getSequenceNumber() != uiPrevSN + 1)
    {
      if (isSNGreaterThan(it->getSequenceNumber(), uiPrevSN))
      {
        uiLoss += (it->getSequenceNumber() - uiPrevSN - 1);
        uiPrevSN = it->getSequenceNumber();
      }
      else
      {
        // older/reordered packet -> ignore as we'll have marked reordered packets
        // as lost as specified in the NADA specification
      }
    }
    else
    {
      uiPrevSN = it->getSequenceNumber();
    }
  }
  // not 100% correct in the case of re-ordering
  uint32_t uiTotalPackets = m_qArrivingPackets.rbegin()->getSequenceNumber() - m_qArrivingPackets.begin()->getSequenceNumber() + 1;
  VLOG(2) << "NadaReceiver::calculateLostOrReorderedPacketsRatio packets lost: " << uiLoss << "/" << uiTotalPackets;
  return uiLoss/static_cast<double>(uiTotalPackets);
}

inline uint32_t NadaReceiver::calculateReceivingRateKbps(const boost::posix_time::ptime& tNow)
{
  uint32_t uiTotalBytesReceivedInWindow = std::accumulate(m_qArrivingPackets.begin(),
                                                          m_qArrivingPackets.end(),
                                                          0,
                                                          [](int iSum, const RtpPacket& rtpPacket)
  {
    return iSum + rtpPacket.getSize();
  });
  // when using the actual duration of the packets in the window, the value is inaccurate for small packet windows
  // e.g. when starting to stream. Just using window size as duration even though this might not be 100% accurate when the window
  // is full
  // automatic conversion to kbit by dividing by ms
#if 0
  uint64_t uiDiff = (tNow - m_qArrivingPackets.begin()->getArrivalTime()).total_milliseconds();
#else
  uint64_t uiDiff = LOGWIN_ms;
#endif
  // fix for div by 0 error on start-up
  if (uiDiff == 0) uiDiff = 1;
  uint32_t uiKbps = (uiTotalBytesReceivedInWindow * 8)/uiDiff;
  return uiKbps;
}

inline bool NadaReceiver::buildUpOfQueuingDelayOverWindow()
{
  for (auto& rtpPacket : m_qArrivingPackets)
  {
    if ((rtpPacket.getOwdSeconds() * 1000) - d_base_ > QEPS_ms)
    {
      return true;
    }
  }
}

inline int64_t NadaReceiver::aggregateCongestionSignal()
{
  VLOG(2) << "NadaReceiver::aggregateCongestionSignal: d_tilde: " << d_tilde_ << " p_mark: " << p_mark_ << " p_loss: " << p_loss_;
  return d_tilde_ + (p_mark_* DMARK_ms) + (p_loss_* DLOSS_s * 1000 /* conversion to ms*/);
  /*
     Here, DMARK is prescribed delay penalty associated with ECN markings
     and DLOSS is prescribed delay penalty associated with packet losses.
     The value of DLOSS and DMARK does not depend on configurations at the
     network node, but does assume that ECN markings, when available,
     occur before losses.  Furthermore, the values of DLOSS and DMARK need
     to be set consistently across all NADA flows for them to compete
     fairly.

     In the absence of packet marking and losses, the value of x_n reduces
     to the observed queuing delay d_n.  In that case the NADA algorithm
     operates in the regime of delay-based adaptation.

     Given observed per-packet delay and loss information, the receiver is
     also in a good position to determine whether the network is
     underutilized and recommend the corresponding rate adaptation mode
     for the sender.  The criteria for operating in accelerated ramp-up
     mode are:

     o  No recent packet losses within the observation window LOGWIN; and

     o  No build-up of queuing delay: d_fwd-d_base < QEPS for all previous
        delay samples within the observation window LOGWIN.

     Otherwise the algorithm operates in graduate update mode.
  */
}


} // experimental
} // rtp_plus_plus
