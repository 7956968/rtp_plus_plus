#pragma once
#include <cpputil/GenericParameters.h>
#include <rtp++/RtpPacket.h>
#include <rtp++/network/RtpNetworkInterface.h>
#include <rtp++/network/SctpAssociation.h>
//#include <rtp++/network/SctpChannelDescriptor.h>
#include <rtp++/sctp/SctpRtpPolicy.h>

namespace rtp_plus_plus
{
namespace sctp
{

class SctpRtpNetworkInterface : public RtpNetworkInterface
{
public:
  /**
    * @brief Constructor
    * @param localRtpEp The local RTP endpoint
    * @param remoteRtpEp The local RTP endpoint
    * @param bIsActive If the local RTP endpoint initiates the connection
    * @param descriptor SCTP channel descriptor for active endpoint
    * @param bConnect The local RTP endpoint
    */
  SctpRtpNetworkInterface(const EndPoint& localRtpEp, const EndPoint &remoteRtpEp,
                          bool bIsActive, const sctp::SctpRtpPolicy& sctpPolicy,
                          bool bConnect, const GenericParameters &applicationParameters);

  /**
    * This method should initalise the component and setup all required
    */
  virtual void reset();

  /**
    * If this method is called the component will begin the shutdown sequence
    * - This will generate an RTCP BYE
    * - No further sends or receives will occur once shutdown has ben called
    */
  virtual void shutdown();

  /**
    * This method should make the sockets ready for receiving data
    */
  virtual bool recv();

  void updateRoundTripTime(double dRtt);
protected:
  /**
    * Implementation of sending RTP over SCTP. This uses a hack to send media over
    * a desired channel. The channel is identified by the id in the endpoint.
    */
  virtual bool doSendRtp(Buffer rtpBuffer, const EndPoint& rtpEp);
  /**
    * Implementation of sending RTCP over SCTP. This uses a hack to send media over
    * a desired channel. The channel is identified by the id in the endpoint:
    * channel 0 will be used for RTCP
    */
  virtual bool doSendRtcp(Buffer rtcpBuffer, const EndPoint& rtcpEp);

  void onSctpRecv(uint32_t uiChannelId, const NetworkPacket& networkPacket);
  void onSctpSendFail(uint8_t* pData, uint32_t uiLength);
private:
  bool createChannel(const PolicyDescriptor& policy);
  bool createChannel(const PolicyDescriptor& policy, double dRtt);

private:
  // End Points
  EndPoint m_localRtpEp;
  EndPoint m_remoteRtpEp;
  /// connection management: who initiates the connection
  bool m_bIsConnected;

  sctp::SctpRtpPolicy m_sctpPolicy;
  /// shutdown flag
  bool m_bShuttingDown;
  static const uint32_t RTCP_CHANNEL_ID;
#ifdef ENABLE_SCTP_USERLAND
  // HACK
  bool m_bConnect;
  peer_connection m_sctpAssociation;
#endif

  // map to store state of channels when using RTT-based channel configuration
  std::map<uint32_t, bool> m_mChannelStatus;
  double m_dRtt;

  enum PrioritisationMode
  {
    PRIORITISE_NONE,
    PRIORITISE_BL,
    PRIORITISE_SCALABLITY
  };

  PrioritisationMode m_ePriotizationMode;

};

} // sctp
} // rtp_plus_plus
