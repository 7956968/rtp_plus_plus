#pragma once
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <cpputil/GenericParameters.h>
#include <rtp++/MemberUpdate.h>
#include <rtp++/PayloadPacketiserBase.h>
#include <rtp++/RtpPacket.h>
#include <rtp++/RtpPacketGroup.h>
#include <rtp++/RtpReferenceClock.h>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/RtpSessionState.h>
#include <rtp++/RtpTime.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/mprtp/MpRtpFlow.h>
#include <rtp++/mprtp/MpRtpHeader.h>
#include <rtp++/mprtp/MpRtpFeedbackManager.h>
#include <rtp++/network/ExistingConnectionAdapter.h>
#include <rtp++/network/RtpNetworkInterface.h>
#include <rtp++/rfc3550/RtcpReportManager.h>
#include <rtp++/rfc3550/SessionDatabase.h>
#include <rtp++/rfc4585/Rfc4585RtcpReportManager.h>

namespace rtp_plus_plus
{

// fwd
class IFeedbackManager;

/// @def DEBUG_INCOMING_RTP Debug information about incoming RTP/RTCP
// #define DEBUG_INCOMING_RTP

/**
 * @brief The RtpSessionV2 can be controlled using the start and stop methods.
 * An RTP session is over once the RTCP BYE is sent. This is triggered via
 * shutting down the RtcpReportManager. Once it is detected that the BYE has
 * been sent, the RTCP network interfaces are shutdown, completing all outstanding
 * asynchronous actions
 */
class RtpSession :  private boost::noncopyable
{
public:
  typedef boost::shared_ptr<RtpSession> ptr;
  typedef boost::function<void ()> RtpSessionCompleteHandler_t;
  typedef boost::function<void (const RtpPacket&, const EndPoint& ep,
                                bool bSSRCValidated, bool bRtcpSynchronised,
                                const boost::posix_time::ptime&)> IncomingRtpCb_t;
  typedef boost::function<void (const RtpPacket&, rfc5285::HeaderExtensionElement&)> ExtensionHeaderCb_t;
  /**
   * @brief create
   * @param rIoService
   * @param rtpParameters
   * @param rtpReferenceClock
   * @param applicationParameters
   * @param bIsSender
   * @return
   */
  static ptr create(boost::asio::io_service& rIoService, const RtpSessionParameters& rtpParameters,
                    RtpReferenceClock& rtpReferenceClock,
                    const GenericParameters &applicationParameters, bool bIsSender);
  /**
   * @brief create
   * @param rIoService
   * @param adapter
   * @param rtpParameters
   * @param rtpReferenceClock
   * @param applicationParameters
   * @param bIsSender
   * @return
   */
  static ptr create(boost::asio::io_service& rIoService, ExistingConnectionAdapter& adapter, const RtpSessionParameters& rtpParameters,
                    RtpReferenceClock& rtpReferenceClock,
                    const GenericParameters &applicationParameters, bool bIsSender);
  /**
   * @brief RtpSession Standard contructor
   * @param ioService
   * @param parameters
   * @param rtpReferenceClock
   * @param applicationParameters
   * @param bIsSender
   */
  RtpSession(boost::asio::io_service& ioService, const RtpSessionParameters& parameters,
             RtpReferenceClock& rtpReferenceClock,
             const GenericParameters& applicationParameters, bool bIsSender);
  /**
   * @brief RtpSession Constructor that takes existing connections
   * @param ioService
   * @param adapter
   * @param parameters
   * @param rtpReferenceClock
   * @param applicationParameters
   * @param bIsSender
   */
  RtpSession(boost::asio::io_service& ioService, ExistingConnectionAdapter& adapter, const RtpSessionParameters& parameters,
             RtpReferenceClock& rtpReferenceClock,
             const GenericParameters& applicationParameters, bool bIsSender);
  /**
   * @brief Destructor
   */
  ~RtpSession();
  /**
   * @brief Getter for RTP session parameters
   *
   * @return a reference to the RTP session parameters.
   */
  const RtpSessionParameters& getRtpSessionParameters() const { return m_parameters; }
  /**
   * @brief Getter for RTP session state
   *
   * @return a reference to the RTP session state.
   */
  const RtpSessionState& getRtpSessionState() const { return  m_rtpSessionState; }
  /**
   * @brief Getter for RTP session state
   *
   * @return a reference to the RTP session state.
   */
  RtpSessionState& getRtpSessionState() { return  m_rtpSessionState; }
  /**
   * @brief Getter for RTP session statistics
   *
   * @return a reference to the RTP session statistics.
   */
  const RtpSessionStatistics& getRtpSessionStatistics() const { return m_rtpSessionStatistics; }
  /**
   * @brief Getter for application parameters
   *
   * @return a reference to the application parameters.
   */
  const GenericParameters& getApplicationParameters() const { return m_applicationParameters; }
  /**
   * @brief Getter for asio IO service
   *
   * @return a reference to the asio IO service.
   */
  boost::asio::io_service& getIoService() { return m_rIoService; }
  /**
   * @brief Getter for RTP reference clock
   *
   * @return a reference to the RTP reference clock.
   */
  RtpReferenceClock& getRtpReferenceClock() { return m_referenceClock; }
  /**
   * @brief Getter for RTP network interface 
   * @return A reference to the RTP network interface
   *
   * Care should be taken when calling this method as this class controls the
   * life time!
   */
  RtpNetworkInterface::ptr& getRtpNetworkInterface(uint32_t uiIndex) { return m_vRtpInterfaces.at(uiIndex); }
  /**
   * @brief For AUs only: returns the packetisation info if the subclass has implemented
   * the functionality
   */
  std::vector<std::vector<uint16_t> > getRtpPacketisationInfo() const;
#if 0
  /**
   * @brief setFeedbackManager
   * @param pFeedbackManager
   */
  void setFeedbackManager(IFeedbackManager* pFeedbackManager) { m_pFeedbackManager = pFeedbackManager; }
#endif
  /**
   * @brief Getter for if session is active
   */
  bool isActive() const;
  /**
   * @brief Getter for is session is shutting down
   */
  bool isShuttingDown() const;
  /**
   * @brief isMpRtpSession
   * @return if the session is a multipath RTP session
   */
  bool isMpRtpSession() const { return m_bIsMpRtpSession; }
  /**
   * @brief getNumberOfFlows
   * @return
   */
  uint16_t getNumberOfFlows() const { return m_MpRtp.m_mFlows.empty() ? 1 : m_MpRtp.m_mFlows.size(); }
  /**
   * @brief RTP session control method: starts RTP session
   */
  boost::system::error_code start();
  /**
   * @brief RTP session control method: stops RTP session
   */
  boost::system::error_code stop();
  /**
   * @brief RTP session control method: resets RTP session
   */
  boost::system::error_code reset();
  /**
   * @brief Setter for callback for incoming RTP packets. The calling class must configure these callbacks
   */
  void setIncomingRtpHandler(IncomingRtpCb_t incomingRtp) { m_incomingRtp = incomingRtp; }
  /**
   * @brief Setter for callback for incoming RTCP packets. The calling class must configure these callbacks
   */
  void setIncomingRtcpHandler(CompoundRtcpCb_t incomingRtcp) { m_incomingRtcp = incomingRtcp; }
  /**
   * @brief Setter for callback before outgoing RTP packets. The calling class must configure these callbacks
   */
  void setBeforeOutgoingRtpHandler(RtpCb_t outgoingRtp) { m_beforeOutgoingRtp = outgoingRtp; }
  /**
   * @brief Setter for callback for outgoing RTP packets. The calling class must configure these callbacks
   */
  void setOutgoingRtpHandler(RtpCb_t outgoingRtp) { m_afterOutgoingRtp = outgoingRtp; }
  /**
   * @brief Setter for callback for outgoing RTCP packets. The calling class must configure these callbacks
   */
  void setOutgoingRtcpHandler(CompoundRtcpCb_t outgoingRtcp) { m_outgoingRtcp = outgoingRtcp; }
  /**
   * @brief setFeedbackCallback Setter for feedback callback
   * @param onFeedback
   */
  void setFeedbackCallback(rfc4585::FeedbackCallback_t onFeedback) { m_onFeedback = onFeedback; }
  /**
   * @brief setMpFeedbackCallback Setter for multipath feedback callback
   * @param onMpFeedback
   */
  void setMpFeedbackCallback(mprtp::MpFeedbackCallback_t onMpFeedback) { m_onMpFeedback = onMpFeedback; }
  /**
   * @brief This method packetises a media sample into RTP packets. These packets must be passed
   * into the sendRtpPackets method in order to be sent over the network.
   * The caller can access the payload, the packetised RTP data, etc.
   */
  std::vector<RtpPacket> packetise(const media::MediaSample& mediaSample);
  /**
   * @brief This method packetises an access unit (vector of media samples) into RTP packets. These packets must be passed
   * into the sendRtpPackets method in order to be sent over the network.
   * The caller can access the payload, the packetised RTP data, etc.
   */
  std::vector<RtpPacket> packetise(const std::vector<media::MediaSample>& vMediaSamples);
  /**
   * @brief packetise packetises a media sample into RTP packets. These packets must be passed
   * into the sendRtpPackets method in order to be sent over the network.
   * The caller can access the payload, the packetised RTP data, etc.
   * NOTE: This overload of the method exists so that the Rtp session manager can decide how to package
   * the media samples in groups but still assign the RTP packets the same RTP TS.
   * @param mediaSample The media sample to be packetised into RTP packets
   * @param uiRtpTimestamp The RTP TS of the packet
   * @return A vector of RTP packets
   */
  std::vector<RtpPacket> packetise(const media::MediaSample& mediaSample, const uint32_t uiRtpTimestamp);
  /**
   * @brief packetise packetises an access unit (vector of media samples) into RTP packets. These packets must be passed
   * into the sendRtpPackets method in order to be sent over the network.
   * The caller can access the payload, the packetised RTP data, etc.
   * @param vMediaSamples The Access Unit consisting of multiple media samples
   * @param uiRtpTimestamp The RTP TS to be used for all packets
   * @return A vector of RTP packets
   */
  std::vector<RtpPacket> packetise(const std::vector<media::MediaSample>& vMediaSamples, const uint32_t uiRtpTimestamp);
  /**
   * @brief sendRtpPacket Sends an RTP packets over the network
   * @param rtpPacket The RTP packet to be sent over the network
   * @param uiSubflowId The flow ID to be used. This is only applicable when using MPRTP or SCTP.
   */
  void sendRtpPacket(const RtpPacket& rtpPacket, uint32_t uiSubflowId = 0);
  /**
   * @brief Sends previously generated RTP packets over the network
   *
   * DEPRECATED: using new API for packet pacing and scheduling
   */
#if 1
  void sendRtpPackets(const std::vector<RtpPacket>& vRtpPackets);
#endif
  /**
   * @brief Depacketises the RTP packet group and returns a vector of media samples
   */
  std::vector<media::MediaSample> depacketise(const RtpPacketGroup& rtpPacketGroup);
  /**
   * @brief tryScheduleEarlyFeedback
   * @param uiFlow
   * @param tScheduled
   * @param uiMs
   * @return
   */
  bool tryScheduleEarlyFeedback(uint16_t uiFlow, boost::posix_time::ptime& tScheduled, uint32_t& uiMs);
  /**
   * @brief setMemberUpdateHandler
   * @param onUpdate
   */
  void setMemberUpdateHandler(MemberUpdateCB_t onUpdate) { m_onUpdate = onUpdate; }
  /**
   * @brief onMemberUpdate The callback to be invoked every time a member update occurs.
   * @param memberUpdate The callback to be invoked every time an RTP session is updated
   */
  void onMemberUpdate(const MemberUpdate& memberUpdate);
  /**
   * @brief setRtpSessionCompleteHandler configures the handler to be called once the RTP session is complete
   * @param handler The callback to be invoked on RTP session completion
   */
  void setRtpSessionCompleteHandler (RtpSessionCompleteHandler_t handler) { m_onRtpSessionComplete = handler; }
  /**
   * @brief findSubflowWithSmallestRtt
   * @param dRtt
   * @return
   */
  uint16_t findSubflowWithSmallestRtt(double& dRtt) const;
  /**
   * @brief getCumulativeLostAsReceiver returns the cumulative packets lost in the role of RTP receiver.
   * @return The number of packets lost in the RTP session.
   */
  uint32_t getCumulativeLostAsReceiver() const;

protected:
  /**
   * @brief Creates the RTP network interfaces
   *
   */
  virtual std::vector<RtpNetworkInterface::ptr> createRtpNetworkInterfaces(const RtpSessionParameters& rtpParameters, const GenericParameters& applicationParameters);
  /**
   * @brief Configures the network interfaces
   */
  void configureNetworkInterface(RtpNetworkInterface::ptr& pRtpInterface, const RtpSessionParameters &rtpParameters, const GenericParameters& applicationParameters);
  /**
   * @brief Creates the payload packetiser object
   */
  PayloadPacketiserBase::ptr createPayloadPacketiser(const RtpSessionParameters& rtpParameters, const GenericParameters &applicationParameters);
  /**
   * @brief Creates the Session Database object
   */
  rfc3550::SessionDatabase::ptr createSessionDatabase(const RtpSessionParameters& rtpParameters, const GenericParameters& applicationParameters);
  /**
   * @brief Creates the RTCP report managers object
   */
  std::vector<rfc3550::RtcpReportManager::ptr> createRtcpReportManagers(const RtpSessionParameters &rtpParameters, const GenericParameters& applicationParameters);
  /**
   * @brief startReadingFromSockets starts read operations on RTP and RTCP sockets
   */
  void startReadingFromSockets();

protected:

  /**
   * @brief two phase inialisation method: this must be called after the subclass has been constructed
   * If initialise fails the previously constructed object MUST NOT be used.
   * An alternative design would be to call this in start and add a virtual doStart method?
   */
  bool initialise();
  /**
   * @brief Create network interfaces for single path case
   */
  std::vector<RtpNetworkInterface::ptr> createRtpNetworkInterfacesSinglePath(const RtpSessionParameters& rtpParameters, const GenericParameters& applicationParameters);
  /**
   * @brief Create network interfaces for MPRTP case
   */
  std::vector<RtpNetworkInterface::ptr> createRtpNetworkInterfacesMultiPath(const RtpSessionParameters& rtpParameters, const GenericParameters& applicationParameters);
  /**
   * @brief Creates RTP network interfaces from existing RTSP connection
   */
  std::vector<RtpNetworkInterface::ptr> createRtpNetworkInterfacesFromExistingRtspConnection(ExistingConnectionAdapter adapter, const RtpSessionParameters& rtpParameters, const GenericParameters& applicationParameters);
  /**
   * @brief initialiseRtpNetworkInterfaces
   * @param rtpParameters
   * @param applicationParameters
   */
  void initialiseRtpNetworkInterfaces(const RtpSessionParameters &rtpParameters, const GenericParameters& applicationParameters);
  /**
   * @brief shutdownNetworkInterfaces
   */
  void shutdownNetworkInterfaces();
  /**
   * @brief initialiseRtcp
   */
  void initialiseRtcp();
  /**
   * @brief shutdownRtcp
   */
  void shutdownRtcp();
  /**
   * @brief onIncomingRtp
   * @param rtpPacket
   * @param ep
   */
  void onIncomingRtp( const RtpPacket& rtpPacket, const EndPoint& ep );
  /**
   * @brief onOutgoingRtp
   * @param rtpPacket
   * @param ep
   */
  void onOutgoingRtp( const RtpPacket& rtpPacket, const EndPoint& ep );
  /**
   * @brief onIncomingRtcp
   * @param compoundRtcp
   * @param ep
   */
  void onIncomingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep );
  /**
   * @brief onOutgoingRtcp
   * @param compoundRtcp
   * @param ep
   */
  void onOutgoingRtcp( const CompoundRtcpPacket& compoundRtcp, const EndPoint& ep );

private:
  /**
   * @brief configureApplicationParameters
   * @param applicationParameters
   */
  void configureApplicationParameters(const GenericParameters& applicationParameters);
  /**
   * @brief handleRapidSyncRtpExtensionHeader
   * @param rtpPacket
   * @param extensionHeader
   */
  void handleRapidSyncRtpExtensionHeader(const RtpPacket& rtpPacket, rfc5285::HeaderExtensionElement& extensionHeader);
  /**
   * @brief handleRtcpRtpExtensionHeader
   * @param rtpPacket
   * @param extensionHeader
   */
  void handleRtcpRtpExtensionHeader(const RtpPacket& rtpPacket, rfc5285::HeaderExtensionElement& extensionHeader);
  /**
   * @brief generateMpRtpExtensionHeader generates an MPRTP header extension
   * @param uiSubflowId Subflow Id of the header to be generated
   * @param bRtx If the SN should be generated for the flow, or the RTX flow
   * @return
   */
  mprtp::MpRtpSubflowRtpHeader generateMpRtpExtensionHeader(uint16_t uiSubflowId, bool bRtx = false);
  /**
   * @brief lookupEndPoint
   * @param uiSubflowId
   * @return
   */
  const EndPoint& lookupEndPoint(uint16_t uiSubflowId);
  /**
   * @brief setRtpHeaderValues
   * @param rtpPackets
   * @param uiRtpTimestamp
   * @param iFlowHint
   * @param uiNtpMws
   * @param uiNtpLws
   */
  void setRtpHeaderValues(std::vector<RtpPacket>& rtpPackets, uint32_t uiRtpTimestamp, int32_t iFlowHint, const uint32_t uiNtpMws, const uint32_t uiNtpLws);
  /**
   * @brief handleMpRtpExtensionHeader
   * @param rtpPacket
   * @param extensionHeader
   */
  void handleMpRtpExtensionHeader(const RtpPacket& rtpPacket, rfc5285::HeaderExtensionElement& extensionHeader);
  /**
   * This method is periodically called when RTCP reports are generated
   * @param uiSN the sequence number of the RTP packet assumed to be lost that was a false positive
   */
  void onRtcpReportGeneration( const CompoundRtcpPacket& rtcp );
  /**
   * @brief onRtcpReportGeneration
   * @param rtcp
   * @param uiInterfaceIndex
   * @param rtcpEp
   */
  void onRtcpReportGeneration( const CompoundRtcpPacket& rtcp, uint32_t uiInterfaceIndex, const EndPoint& rtcpEp );
  /**
   * @brief onGenerateFeedback
   * @param feedback
   */
  void onGenerateFeedback(CompoundRtcpPacket& feedback);
  /**
   * @brief onGenerateFeedbackMpRtp
   * @param uiFlowId
   * @param feedback
   * @return
   */
  void onGenerateFeedbackMpRtp(uint16_t uiFlowId, CompoundRtcpPacket& feedback);
  /**
   * @brief doPacketise
   * @param mediaSample
   * @param uiRtpTimestamp
   * @param uiNtpMws
   * @param uiNtpLws
   * @return
   */
  std::vector<RtpPacket> doPacketise(const media::MediaSample& mediaSample, const uint32_t uiRtpTimestamp, const uint32_t uiNtpMws, const uint32_t uiNtpLws);
  /**
   * @brief doPacketise
   * @param vMediaSamples
   * @param uiRtpTimestamp
   * @param uiNtpMws
   * @param uiNtpLws
   * @return
   */
  std::vector<RtpPacket> doPacketise(const std::vector<media::MediaSample>& vMediaSamples, const uint32_t uiRtpTimestamp, const uint32_t uiNtpMws, const uint32_t uiNtpLws);
  /**
   * @brief registerExtensionHeaderHandler
   * @param uiExtensionId
   * @param extensionHeaderCb
   */
  void registerExtensionHeaderHandler(uint32_t uiExtensionId, const ExtensionHeaderCb_t& extensionHeaderCb)
  {
    m_mExtensionHeaderHandlers[uiExtensionId] = extensionHeaderCb;
  }
protected:

  /// asio service
  boost::asio::io_service& m_rIoService;

  RtpSessionParameters m_parameters;
  RtpReferenceClock& m_referenceClock;
  GenericParameters m_applicationParameters;
  ExistingConnectionAdapter m_existingConnectionAdapter;

  bool m_bIsSender;
  // for one way delay measurement
  bool m_bTryExtractNtpTimestamp;
  // rapid sync mode
  enum RapidSyncMode
  {
    RS_NONE = 0,             // no rapid sync header
    RS_EVERY_SAMPLE = 1,     // sync first RTP packet of every media sample
    RS_EVERY_RTP_PACKET = 2  // every RTP packet
  };
  RapidSyncMode m_eRapidSyncMode;
  uint32_t m_uiRapidSyncExtmapId;

  /// Payload specific packetiser
  PayloadPacketiserBase::ptr m_pPayloadPacketiser;
  rfc3550::SessionDatabase::ptr m_pSessionDb;
  std::vector<RtpNetworkInterface::ptr> m_vRtpInterfaces;
  std::vector<rfc3550::RtcpReportManager::ptr> m_vRtcpReportManagers;
#if 0
  IFeedbackManager* m_pFeedbackManager;
#endif
  enum SessionState
  {
    SS_STOPPED,
    SS_STARTED,       ///< set if start is called successfully
    SS_SHUTTING_DOWN  ///< set when shutting down to prevent new asynchronous operations from being launched
  };
  SessionState m_state;
  // RTP dynamic state
  RtpSessionState m_rtpSessionState;

  /// flag that gets cleared when packets arrive from an unknown source
  /// and is set on validation
  bool m_bValidated;

  /// RTP and RTCP statistics
  RtpSessionStatistics m_rtpSessionStatistics;

  mutable boost::mutex m_componentLock;

  // mapping of media time to system time for RTCP SR
  // when an RTP packet is sent we want to store the corresponding system time
  // so that we can work out the RTP timestamp when we send the SR
  boost::posix_time::ptime m_tLastRtpPacketSent;
  uint32_t m_uiRtpTsLastRtpPacketSent;

  // receiver variables
  // first sample received
  bool m_bFirstSample;
  // timestamp of first sampe that is used
  // as offset for subsequent samples
  double m_dStartOffset;

  /// This variable is used to keep track
  /// of how many bye have been sent. This
  /// is required for MPRTP functionality.
  uint32_t m_uiByeSentCount;

  IncomingRtpCb_t m_incomingRtp;
  CompoundRtcpCb_t m_incomingRtcp;
  RtpCb_t m_beforeOutgoingRtp;
  RtpCb_t m_afterOutgoingRtp;
  CompoundRtcpCb_t m_outgoingRtcp;
  rfc4585::FeedbackCallback_t m_onFeedback;
  mprtp::MpFeedbackCallback_t m_onMpFeedback;

  // Callback for member updates
  MemberUpdateCB_t m_onUpdate;
  RtpSessionCompleteHandler_t m_onRtpSessionComplete;
  // Helper variables to store packetisation info
  // sequence number of first RTP packet in last packetised AU
  uint16_t m_uiLastAUStartSN;
  // count of RTP packets in last packetised AU
  uint32_t m_uiLastAUPacketCount;

  uint32_t m_uiRtcpRtpHeaderExtmapId;
  std::unordered_map<uint32_t, ExtensionHeaderCb_t> m_mExtensionHeaderHandlers;
protected:

  // MPRTP variables
  bool m_bIsMpRtpSession;
  uint32_t m_uiMpRtpExtmapId;
  typedef std::map<uint16_t, mprtp::MpRtpFlow> FlowMap_t;
  struct MpRtpParameters
  {
    uint16_t m_uiLastFlowId;
    FlowMap_t m_mFlows;
    std::map<uint16_t, uint32_t> m_mFlowInterfaceMap;
  };

  MpRtpParameters m_MpRtp;

  std::unordered_map<uint16_t, MemberUpdate> m_mPathInfo;
};

} // rtp_plus_plus
