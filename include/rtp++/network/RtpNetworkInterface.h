#pragma once
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <rtp++/RtpPacket.h>
#include <rtp++/RtpPacketiser.h>
#include <rtp++/RtcpPacketBase.h>
#include <rtp++/RtcpParserInterface.h>
#include <rtp++/network/EndPoint.h>
#include <rtp++/rfc3550/Rfc3550RtcpValidator.h>
#ifdef ENABLE_LIB_SRTP
#include <srtp.h>
#endif

namespace rtp_plus_plus
{

/// Callback for RTP packets
typedef boost::function<void (const RtpPacket&, const EndPoint& ep)> RtpCb_t;
/// Callback for RTCP packets
typedef boost::function<void (const CompoundRtcpPacket&, const EndPoint& ep)> CompoundRtcpCb_t;
//typedef boost::function<void (const CompoundRtcpPacket&, const EndPoint& ep)> RtcpCb_t;

/// @def MEASURE_RTP_INTERARRIVAL_TIME Debug information about interarrival time in ms
// #define MEASURE_RTP_INTERARRIVAL_TIME

/// @def RTCP_STRICT Validate outgoing RTCP packets
// #define RTCP_STRICT

/// @def DEBUG_RTP Debug information about incoming and outgoing RTP packets
// #define DEBUG_INCOMING_RTP

/// @def DEBUG_RTCP Debug information about incoming and outgoing RTCP packets
// #define DEBUG_INCOMING_RTCP

/**
 * @brief This class abstracts an RTP network interface from the actual sending and receiving of the packets
 * Subclasses are responsible for sending and receiving the RTP/RTCP data by implementing the
 * doSendRtp doSendRtcp methods and calling onRtpSend onRtcpSent onReceiveRtp onReceiveRtcp
 */
class RtpNetworkInterface : private boost::noncopyable
{
public:
  typedef std::unique_ptr<RtpNetworkInterface> ptr;

  virtual ~RtpNetworkInterface();
  /**
   * @brief Configure RTCP validator
   * @param[in] pValidator The validator to be used for RTCP packets
   */
  void setRtcpValidator(rfc3550::RtcpValidator::ptr pValidator);
  /**
   * @brief This method allows the addition of RTCP parsers for the purpose
   * of handling other RTCP types
   * @param[in] pParser The RTCP parser to be added
   */
  void registerRtcpParser(RtcpParserInterface::ptr pParser);
  /**
   * @brief Packetisers implementing other RFCs can be configured here
   * Note: any registered RTP extension header and RTCP parsers
   * will be lost if the RTP packetiser is replaced
   */
  void setRtpPacketiser(std::unique_ptr<RtpPacketiser> pPacketiser);
  /**
   * @brief getRtpPacketiser Gets reference to RTP/RTCP packetiser
   * @return
   */
  RtpPacketiser& getRtpPacketiser() { return *m_pRtpPacketiser.get(); }
  /**
   * @brief Callback for incoming RTP packets. The calling class must configure these callbacks
   */
  void setIncomingRtpHandler(RtpCb_t incomingRtp) { m_incomingRtp = incomingRtp; }
  /**
   * @brief Callback for incoming RTCP packets. The calling class must configure these callbacks
   */
  void setIncomingRtcpHandler(CompoundRtcpCb_t incomingRtcp) { m_incomingRtcp = incomingRtcp; }
  /**
   * Callback for outgoing RTP packets. The calling class must configure these callbacks
   */
  void setOutgoingRtpHandler(RtpCb_t outgoingRtp) { m_outgoingRtp = outgoingRtp; }
  /**
   * Callback for outgoing RTCP packets. The calling class must configure these callbacks
   */
  void setOutgoingRtcpHandler(CompoundRtcpCb_t outgoingRtcp) {m_outgoingRtcp = outgoingRtcp; }
  /**
   * @brief secureRtp
   * @param bSecure If the communication should be secure or not
   */
  bool secureRtp(bool bSecure);
  /**
   * @brief This method should initalise the component and setup all required
   */
  virtual void reset() = 0;
  /**
   * @brief If this method is called the component will begin the shutdown sequence
   * - This will generate an RTCP BYE
   * - No further sends or receives will occur once shutdown has been called
   */
  virtual void shutdown() = 0;
  /**
   * @brief This method should make the sockets ready for receiving data
   */
  virtual bool recv() = 0;
  /**
   * This method should be called to deliver a media sample
   * First method of asynchronous sending interface
   * The sublass is responsible for implementing doSendRtp
   */
  bool send(const RtpPacket& rtpPacket, const EndPoint& destination);
  /**
   * This method should be called to deliver an RTCP report
   * First method of asynchronous sending interface
   * The subclass is responsible for implementing doSendRtcp
   */
  bool send(const CompoundRtcpPacket& compoundPacket, const EndPoint& destination);

protected:

  /**
   * @brief RtpNetworkInterface
   */
  RtpNetworkInterface();
  /**
   * @brief RtpNetworkInterface
   * @param pPacketiser
   */
  RtpNetworkInterface(std::unique_ptr<RtpPacketiser> pPacketiser);
  /**
   * The subclass must implement these methods
   */
  virtual bool doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp) = 0;
  /**
   *
   */
  virtual bool doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp) = 0;
  /**
   * Second method of the asynchronous sending interface
   * This must be called by the subclass once the sending
   * of the RTP packet has completed, regardless of whether
   * the call was successful
   */
  void onRtpSent(Buffer buffer, const EndPoint& ep);
  /**
   * Second method of the asynchronous sending interface
   * This must be called by the subclass once the sending
   * of the RTCP packet has completed, regardless of whether
   * the call was successful
   */
  void onRtcpSent(Buffer buffer, const EndPoint& ep);
  /**
   * This method must be called by the subclass when an RTP packet has been received
   */
  void processIncomingRtpPacket(NetworkPacket networkPacket, const EndPoint& ep);
  /**
   * This method must be called by the subclass when an RTCP packet has been received
   */
  void processIncomingRtcpPacket(NetworkPacket networkPacket, const EndPoint& ep);

private:

  /// RTP packetiser for incoming and outgoing packets
  std::unique_ptr<RtpPacketiser> m_pRtpPacketiser;

  /// SRTP
  bool m_bSecureRtp;

#ifdef ENABLE_LIB_SRTP
  srtp_t m_srtpSession;
  srtp_policy_t m_srtpPolicy;
  uint8_t m_srtpKey[30];
#endif

  RtpCb_t m_incomingRtp;
  RtpCb_t m_outgoingRtp;
  CompoundRtcpCb_t m_incomingRtcp;
  CompoundRtcpCb_t m_outgoingRtcp;

  /// State management for callbacks
  std::deque<RtpPacket> m_qRtp;
  std::deque<CompoundRtcpPacket> m_qRtcp;
  boost::mutex m_lock;
  boost::mutex m_rtcplock;

#ifdef MEASURE_RTP_INTERARRIVAL_TIME
  // for measuring interarrival jitter
  boost::posix_time::ptime m_tPreviousArrival;
#endif
};

} // rtp_plus_plus
