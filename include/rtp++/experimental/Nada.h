#pragma once
#include <cstdint>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <rtp++/RtpPacket.h>
#include <rtp++/RtpSessionParameters.h>

namespace rtp_plus_plus {


// fwd
class TransmissionManager;

namespace experimental {

// fwd
class RtcpNadaFb;

// continued from Rfc4585.h
const uint8_t TL_FB_NADA = 13; // experimental value

// a=rtcp-fb:<payload type> nada
/// SDP attributes
extern const std::string NADA;

extern bool supportsNadaFb(const RtpSessionParameters& rtpParameters);

const uint32_t ACCELERATED_RAMP_UP = 0;
const uint32_t GRADUAL_RATE_UPDATE = 1;

#define RMIN_kbps  150u //       | Minimum rate of application      |    150 Kbps    |
                        //       | supported by media encoder       |                |
#define RMAX_kbps  1500u//       | Maximum rate of application      |    1.5 Mbps    |

/**
 * @brief The NadaSender class
 */
class NadaSender
{
public:
  /**
   * @brief NadaSender
   */
  NadaSender(TransmissionManager& transmissionManager, uint32_t uiFps, uint32_t uiMinRateKbps = RMIN_kbps, uint32_t uiMaxRateKbps = RMAX_kbps);
  /**
   * @brief calculateRateKbps calculate rates based on NADA
   * @param nadaFb NADA feedback received from receiver
   * @param buffer_len A rate shaping buffer is employed to absorb any instantaneous
   mismatch between encoder rate output r_vout and regulated sending
   rate r_send.  Its current level of occupancy is measured in bytes.
   * @param r_vin Rate of video encoder into rate shaping buffer
   * @param r_send Rate of sending into network
   * @return
   */
  uint32_t calculateRateKbps(const experimental::RtcpNadaFb& nadaFb, uint32_t buffer_len, uint32_t& r_vin, uint32_t& r_send);
private:

private:

  TransmissionManager& m_transmissionManager;
  uint32_t m_uiFps;
  uint32_t m_uiMinRateKbps;
  uint32_t m_uiMaxRateKbps;
  uint32_t m_uiMinNetworkRateKbps;
  uint32_t m_uiMaxNetworkRateKbps;
  uint32_t rmode_; //  Rate update mode: (0 = accelerated ramp-up, 1 = gradual rate update
  boost::posix_time::ptime t_last_;       //  Last time sending/receiving a feedback          |
  boost::posix_time::ptime t_curr_;       //  Current timestamp                               |
  uint32_t delta_;        //  Observed interval between current and previous  |
                          //  feedback reports: delta = t_curr-t_last         |
  uint32_t r_n_;          //  Reference rate based on network congestion      |
  uint32_t r_send_;       //  Sending rate                                    |
  uint32_t r_recv_;       //  Receiving rate                                  |
  uint32_t r_vin_;        //  Target rate for video encoder                   |
  uint32_t r_vout_;       //  Output rate from video encoder                  |
  uint32_t gamma_;        //  Rate increase multiplier in accelerated ramp-up |
  uint32_t rtt_;          //  Estimated round-trip-time at sender             |
  uint32_t buffer_len_;   //  Rate shaping buffer occupancy measured in bytes |
  int64_t x_n_;           //  Aggregate congestion signal                     |
  int64_t x_prev_;        //  Previous value of aggregate congestion signal   |
  int64_t x_diff_;        //  Change in aggregate congestion signal w.r.t.    |
                          //  its previous value: x_diff = x_n - x_prev       |
};

class NadaReceiver
{
public:
  NadaReceiver();
  /**
   * @brief receivePacket
   * @param rtpPacket
   */
  void receivePacket(const RtpPacket& rtpPacket);
  /**
   * @brief generateFeedback
   * @return
   */
  bool generateFeedback(uint32_t& rmode, int64_t& x_n, uint32_t& r_recv);

private:

  void updateLogWindow(const boost::posix_time::ptime& tNow);
  double calculateLostOrReorderedPacketsRatio();
  uint32_t calculateReceivingRateKbps(const boost::posix_time::ptime& tNow);
  int64_t aggregateCongestionSignal();

  bool buildUpOfQueuingDelayOverWindow();
private:

  uint32_t rmode_; //  Rate update mode: (0 = accelerated ramp-up, 1 = gradual rate update
  boost::posix_time::ptime t_last_;       //  Last time sending/receiving a feedback          |
  boost::posix_time::ptime t_curr_;       //  Current timestamp                               |
  uint32_t r_recv_;       //  Receiving rate                                  |
  int64_t d_base_;       //  Estimated baseline delay                        |
  int64_t d_fwd_;        //  Measured and filtered one-way delay             |
  int64_t d_n_;          //  Estimated queueing delay                        |
  int64_t d_tilde_;      //  Equivalent delay after non-linear warping       |
  double p_mark_;         //  Estimated packet ECN marking ratio              |
  double p_loss_;         //  Estimated packet loss ratio                     |
  int64_t x_n_;          //  Aggregate congestion signal                     |
  int64_t x_prev_;       //  Previous value of aggregate congestion signal   |
  int64_t x_diff_;       //  Change in aggregate congestion signal w.r.t.    |
                         //  its previous value: x_diff = x_n - x_prev       |

  uint64_t QMAX_MINUS_QTH_POW4;

  // deque for storing arrival info of packets to maintain 500ms LOGWIN
  std::deque<RtpPacket> m_qArrivingPackets;
};

} // experimental
} // rtp_plus_plus
