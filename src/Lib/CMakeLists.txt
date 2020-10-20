# CMakeLists.txt in Lib dir

MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  IF(MSVC)
    GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledBasename}.pch")
    SET(Sources ${${SourcesVar}})

    SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_OUTPUTS "${PrecompiledBinary}")
    SET_SOURCE_FILES_PROPERTIES(${Sources}
                                PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledBinary}\" /FI\"${PrecompiledBinary}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_DEPENDS "${PrecompiledBinary}")  
    # Add precompiled header to SourcesVar
    LIST(APPEND ${SourcesVar} ${PrecompiledSource})
  ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
  message("Clang detected: setting warning level")
# set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror")
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror -Wno-unused-function")
#  ADD_DEFINITIONS (-Werror )
#  ADD_DEFINITIONS (-Wall )
#  ADD_DEFINITIONS (/W4 )
#  set_property( GLOBAL APPEND_STRING PROPERTY COMPILE_FLAGS -Werror )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
endif()


SET(CORE_SRCS 
CorePch.cpp
GroupedRtpSessionManager.cpp
PayloadPacketiserBase.cpp
PtsBasedJitterBuffer.cpp
RtcpPacketBase.cpp
RtpJitterBuffer.cpp
RtpJitterBufferV2.cpp
RtpPacketGroup.cpp
RtpPacketisationSessionManager.cpp
RtpPacketiser.cpp
RtpReferenceClock.cpp
RtpSessionManager.cpp
RtpSession.cpp
RtpSessionParameters.cpp
RtpSessionStatistics.cpp
RtpTime.cpp
RtcpTransmissionTimerBase.cpp
SvcGroupedRtpSessionManager.cpp
SvcRtpPacketisationSessionManager.cpp
TransmissionManager.cpp
)
SET(APPLICATION_SRCS
application/ApplicationConfiguration.cpp
application/ApplicationContext.cpp
application/ApplicationParameters.cpp
application/ApplicationUtil.cpp
)
SET(EXPERIMENTAL_SRCS
experimental/GoogleRemb.cpp
experimental/Nada.cpp
experimental/RtcpHeaderExtension.cpp
experimental/Scream.cpp
)
SET(MEDIA_H264_SRCS
media/h264/H264AnnexBStreamParser.cpp
)
SET(MEDIA_H265_SRCS
media/h265/H265AnnexBStreamParser.cpp
)
SET(MEDIA_SRCS
media/AsyncStreamMediaSource.cpp
media/BufferedMediaReader.cpp
media/GeneratedMediaSource.cpp
media/MediaGeneratorService.cpp
media/MediaSink.cpp
media/MediaTypes.cpp
media/NalUnitMediaSource.cpp
media/SimpleMultimediaService.cpp
media/SimpleMultimediaServiceV2.cpp
media/StreamMediaSource.cpp
media/VirtualVideoDevice.cpp
media/VirtualVideoDeviceV2.cpp
media/YuvMediaSource.cpp
)
SET(MEDIA_SESSION_SRCS
mediasession/MediaSessionDescription.cpp
mediasession/RtpAnalysisMediaSession.cpp
mediasession/RtpAnalysisMediaSessionFactory.cpp
mediasession/RtpPacketisationMediaSessionFactory.cpp
mediasession/SimpleMediaSession.cpp
mediasession/SimpleMediaSessionV2.cpp
mediasession/SimpleMediaSessionFactory.cpp
mediasession/SvcMediaSessionFactory.cpp
mediasession/SvcRtpPacketisationMediaSessionFactory.cpp
mediasession/VirtualMediaSessionFactory.cpp
)
SET(MPRTP_SRCS
mprtp/FlowSpecificMemberEntry.cpp
mprtp/MpRtcp.cpp
mprtp/MpRtcpReportManager.cpp
mprtp/MpRtp.cpp
mprtp/MpRtpFeedbackManager.cpp
mprtp/MpRtpMemberEntry.cpp
mprtp/MpRtpSessionDatabase.cpp
mprtp/MpRtpTranslator.cpp
mprtp/MpRtpTranslatorRtpSessionManager.cpp
mprtp/MpRtpTranslatorSessionFactory.cpp
)
SET(NETWORK_SRCS
network/DccpRtpConnection.cpp
network/MediaSessionNetworkManager.cpp
network/MuxedUdpRtpNetworkInterface.cpp
network/NetworkInterfaceUtil.cpp
network/PortAllocationManager.cpp
network/RtpForwarder.cpp
network/RtpNetworkInterface.cpp
network/RtspAdapterRtpNetworkInterface.cpp
network/SctpAssociation.cpp
network/SctpAssociationV2.cpp
network/SctpRtpNetworkInterface.cpp
network/TcpRtpConnection.cpp
network/UdpRtpNetworkInterface.cpp
network/UdpForwarder.cpp
network/UdpSocketWrapper.cpp
network/VirtualUdpRtpNetworkInterface.cpp
)
SET(RFC2326_SRCS
rfc2326/HeaderFields.cpp
rfc2326/IRtspAgent.cpp
rfc2326/LiveRtspServer.cpp
rfc2326/LiveServerMediaSession.cpp
rfc2326/Rfc2326.cpp
rfc2326/Register.cpp
rfc2326/RtspClient.cpp
rfc2326/RtspClientConnection.cpp
rfc2326/RtspClientSession.cpp
rfc2326/RtspMessage.cpp
rfc2326/RtspServer.cpp
rfc2326/RtspServerConnection.cpp
rfc2326/RtspUtil.cpp
rfc2326/SelfRegisteringRtspServer.cpp
rfc2326/SelfRegisteringRtspServerConnection.cpp
rfc2326/ServerMediaSession.cpp
rfc2326/Transport.cpp
)
SET(RFC3261_SRCS
rfc3261/ClientTransaction.cpp
rfc3261/DialogManager.cpp
rfc3261/Registrar.cpp
rfc3261/Rfc3261.cpp
rfc3261/ServerTransaction.cpp
rfc3261/SipAgent.cpp
rfc3261/SipClientTcpConnection.cpp
rfc3261/SipDialog.cpp
rfc3261/SipMessage.cpp
rfc3261/SipProxy.cpp
rfc3261/SipServerTcpConnection.cpp
rfc3261/SipUri.cpp
rfc3261/StatelessProxy.cpp
rfc3261/TransactionUser.cpp
rfc3261/TransportLayer.cpp
rfc3261/UserAgent.cpp
rfc3261/UserAgentBase.cpp
rfc3261/UserAgentClient.cpp
rfc3261/UserAgentServer.cpp
)
SET(RFC3264_SRCS
rfc3264/OfferAnswerModel.cpp
rfc3264/Rfc3264.cpp
)
SET(RFC3550_SRCS
rfc3550/ComputeRtcpInterval.cpp
rfc3550/MemberEntry.cpp
rfc3550/Rfc3550.cpp
rfc3550/Rfc3550RtcpValidator.cpp
rfc3550/Rtcp.cpp
rfc3550/RtcpReportManager.cpp
rfc3550/RtcpTransmissionTimer.cpp
rfc3550/RtpConstants.cpp
rfc3550/RtpHeader.cpp
rfc3550/SessionDatabase.cpp
)
SET(RFC3581_SRCS
rfc3581/Rfc3581.cpp
)
SET(RFC3611_SRCS
rfc3611/Rfc3611.cpp
rfc3611/XrMemberEntry.cpp
)
SET(RFC3711_SRCS
rfc3711/Rfc3711.cpp
)
SET(RFC4145_SRCS
rfc4145/Rfc4145.cpp
)
SET(RFC4566_SRCS
rfc4566/MediaDescription.cpp
rfc4566/Rfc4566.cpp
rfc4566/SdpParser.cpp
rfc4566/SessionDescription.cpp
)
SET(RFC4571_SRCS
rfc4571/MuxedTcpRtpNetworkInterface.cpp
rfc4571/PassiveTcpRtpNetworkInterface.cpp
rfc4571/TcpRtpNetworkInterface.cpp
)
SET(RFC4585_SRCS
rfc4585/FeedbackManager.cpp
rfc4585/RtcpFb.cpp
rfc4585/Rfc4585.cpp
rfc4585/Rfc4585RtcpReportManager.cpp
rfc4585/Rfc4585RtcpTransmissionTimer.cpp
)
SET(RFC4588_SRCS
rfc4588/Rfc4588.cpp
)
SET(RFC4867_SRCS
rfc4867/Rfc4867.cpp
rfc4867/Rfc4867Packetiser.cpp
)
SET(RFC5285_SRCS
rfc5285/Rfc5285.cpp
rfc5285/RtpHeaderExtension.cpp
)
SET(RFC5506_SRCS
rfc5506/Rfc5506.cpp
rfc5506/Rfc5506RtcpValidator.cpp
)
SET(RFC5761_SRCS
rfc5761/Rfc5761.cpp
)
SET(RFC5762_SRCS
rfc5762/MuxedDccpRtpNetworkInterface.cpp
rfc5762/PassiveDccpRtpNetworkInterface.cpp
rfc5762/DccpRtpNetworkInterface.cpp
)
SET(RFC5888_SRCS
rfc5888/Rfc5888.cpp
)
SET(RFC6051_SRCS
rfc6051/Rfc6051.cpp
)
SET(RFC6184_SRCS
rfc6184/Rfc6184.cpp
rfc6184/Rfc6184Packetiser.cpp
)
SET(RFC6190_SRCS
rfc6190/Rfc6190.cpp
rfc6190/Rfc6190Packetiser.cpp
)
SET(RFCHEVC_SRCS
rfchevc/Rfchevc.cpp
rfchevc/RfchevcPacketiser.cpp
)
SET(RTO_SRCS
rto/BasicRtoEstimator.cpp
rto/CrossPathRtoEstimator.cpp
rto/MultipathRtoEstimator.cpp
rto/RetransmissionInfo.cpp
rto/RtoEstimationSession.cpp
rto/SimpleRtoEstimator.cpp
rto/StatisticsManager.cpp
rto/VirtualRtpSession.cpp
)
SET(SCHEDULING_SRCS
scheduling/AckBasedRtpScheduler.cpp
scheduling/NadaScheduler.cpp
scheduling/ScreamScheduler.cpp
)
SET(SCTP_SRCS
sctp/SctpH264RtpPolicyManager.cpp
sctp/SctpH264RtpSessionManager.cpp
sctp/SctpH265RtpPolicyManager.cpp
sctp/SctpH265RtpSessionManager.cpp
sctp/SctpRtpPolicy.cpp
sctp/SctpShvcRtpPacketisationSessionManager.cpp
sctp/SctpShvcRtpSessionManager.cpp
sctp/SctpSvcRtpPacketisationSessionManager.cpp
sctp/SctpSvcRtpSessionManager.cpp
)
SET(UTIL_SRCS
util/Base64.cpp
)

SET(CORE_HEADERS
CorePch.h
../../include/rtp++/CoreDecl.h
../../include/rtp++/ForwardIncomingRtpSessionManager.h
../../include/rtp++/GroupedRtpSessionManager.h
../../include/rtp++/IFeedbackManager.h
../../include/rtp++/IRtpJitterBuffer.h
# ../../include/rtp++/IRtpSessionInfoManager.h
../../include/rtp++/IRtpSessionManager.h
../../include/rtp++/LossEstimator.h
../../include/rtp++/PayloadPacketiserBase.h
../../include/rtp++/PlayoutBufferNode.h
../../include/rtp++/PtsBasedJitterBuffer.h
../../include/rtp++/RtcpPacketBase.h
../../include/rtp++/RtcpParserInterface.h
../../include/rtp++/RtcpTransmissionTimerBase.h
../../include/rtp++/RtpJitterBuffer.h
../../include/rtp++/RtpJitterBufferV2.h
../../include/rtp++/RtpDump.h
../../include/rtp++/RtpPacket.h
../../include/rtp++/RtpPacketGroup.h
../../include/rtp++/RtpPacketisationSessionManager.h
../../include/rtp++/RtpPacketiser.h
../../include/rtp++/RtpReferenceClock.h
../../include/rtp++/RtpSessionManager.h
../../include/rtp++/RtpSessionParameters.h
../../include/rtp++/RtpSessionState.h
../../include/rtp++/RtpSessionStatistics.h
../../include/rtp++/RtpSession.h
../../include/rtp++/RtpTime.h
../../include/rtp++/RtpUtil.h
../../include/rtp++/SvcGroupedRtpSessionManager.h
../../include/rtp++/SvcRtpPacketisationSessionManager.h
../../include/rtp++/TransmissionManager.h
../../include/rtp++/Version.h
)
SET(APPLICATION_HEADERS
../../include/rtp++/application/Application.h
../../include/rtp++/application/ApplicationConfiguration.h
../../include/rtp++/application/ApplicationContext.h
../../include/rtp++/application/ApplicationUtil.h
../../include/rtp++/application/ApplicationParameters.h
)
SET(EXPERIMENTAL_HEADERS
../../include/rtp++/experimental/Experimental.h
../../include/rtp++/experimental/ExperimentalRtcp.h
../../include/rtp++/experimental/GoogleRemb.h
../../include/rtp++/experimental/Nada.h
../../include/rtp++/experimental/ICooperativeCodec.h
../../include/rtp++/experimental/RtcpGenericAck.h
../../include/rtp++/experimental/RtcpHeaderExtension.h
../../include/rtp++/experimental/Scream.h
)
SET(MEDIA_HEADERS
../../include/rtp++/media/AmrFileSource.h
../../include/rtp++/media/AmrFileSink.h
../../include/rtp++/media/AsyncStreamMediaSource.h
../../include/rtp++/media/BufferedMediaReader.h
../../include/rtp++/media/DirectShowDevice.h
../../include/rtp++/media/GeneratedMediaSource.h
../../include/rtp++/media/IMediaSampleSource.h
../../include/rtp++/media/ITransform.h
../../include/rtp++/media/IVideoCodecTransform.h
../../include/rtp++/media/IVideoDevice.h
../../include/rtp++/media/MediaConfigurationParser.h
../../include/rtp++/media/MediaConsumerService.h
../../include/rtp++/media/MediaGeneratorService.h
../../include/rtp++/media/MediaModels.h
../../include/rtp++/media/MediaSample.h
../../include/rtp++/media/MediaSink.h
../../include/rtp++/media/MediaSource.h
../../include/rtp++/media/MediaTypes.h
../../include/rtp++/media/MediaStreamParser.h
../../include/rtp++/media/NalUnitMediaSource.h
../../include/rtp++/media/SimpleMultimediaService.h
../../include/rtp++/media/SimpleMultimediaServiceV2.h
../../include/rtp++/media/StreamMediaSource.h
../../include/rtp++/media/VideoInputSource.h
../../include/rtp++/media/VirtualVideoDevice.h
../../include/rtp++/media/VirtualVideoDeviceV2.h
../../include/rtp++/media/YuvMediaSource.h
)
SET(MEDIA_H264_HEADERS
../../include/rtp++/media/h264/H264AnnexBStreamParser.h
../../include/rtp++/media/h264/H264AnnexBStreamWriter.h
../../include/rtp++/media/h264/H264FormatDescription.h
../../include/rtp++/media/h264/H264NalUnitTypes.h
../../include/rtp++/media/h264/SvcExtensionHeader.h
)
SET(MEDIA_H265_HEADERS
../../include/rtp++/media/h265/H265AnnexBStreamParser.h
../../include/rtp++/media/h265/H265AnnexBStreamWriter.h
../../include/rtp++/media/h265/H265FormatDescription.h
../../include/rtp++/media/h265/H265NalUnitTypes.h
)
SET(MEDIA_SESSION_HEADERS
../../include/rtp++/mediasession/MediaSessionDescription.h
../../include/rtp++/mediasession/RtpAnalysisMediaSession.h
../../include/rtp++/mediasession/RtpAnalysisMediaSessionFactory.h
../../include/rtp++/mediasession/RtpPacketisationMediaSessionFactory.h
../../include/rtp++/mediasession/SimpleMediaSession.h
../../include/rtp++/mediasession/SimpleMediaSessionV2.h
../../include/rtp++/mediasession/SimpleMediaSessionFactory.h
../../include/rtp++/mediasession/SvcMediaSessionFactory.h
../../include/rtp++/mediasession/SvcRtpPacketisationMediaSessionFactory.h
../../include/rtp++/mediasession/VirtualMediaSessionFactory.h
)
SET(MPRTP_HEADERS
../../include/rtp++/mprtp/MpRtpFeedbackManager.h
../../include/rtp++/mprtp/FlowSpecificMemberEntry.h
../../include/rtp++/mprtp/MpRtcp.h
../../include/rtp++/mprtp/MpRtcpReportManager.h
../../include/rtp++/mprtp/MpRtp.h
../../include/rtp++/mprtp/MpRtpFlow.h
../../include/rtp++/mprtp/MpRtpHeaderExtension.h
../../include/rtp++/mprtp/MpRtpHeader.h
../../include/rtp++/mprtp/MpRtpMemberEntry.h
../../include/rtp++/mprtp/MpRtpSessionDatabase.h
../../include/rtp++/mprtp/MpRtpTranslator.h
../../include/rtp++/mprtp/MpRtpTranslatorRtpSessionManager.h
../../include/rtp++/mprtp/MpRtpTranslatorSessionFactory.h
)
SET(NETWORK_HEADERS
../../include/rtp++/network/dccp/dccp.hpp
../../include/rtp++/network/dccp/packetstream_socket_service.hpp
../../include/rtp++/network/AddressDescriptor.h
../../include/rtp++/network/AddressDescriptorParser.h
../../include/rtp++/network/DccpRtpConnection.h
../../include/rtp++/network/EndPoint.h
../../include/rtp++/network/ExistingConnectionAdapter.h
../../include/rtp++/network/MediaSessionNetworkManager.h
../../include/rtp++/network/MuxedUdpRtpNetworkInterface.h
../../include/rtp++/network/NetworkInterfaceUtil.h
../../include/rtp++/network/NetworkPacket.h
../../include/rtp++/network/PortAllocationManager.h
../../include/rtp++/network/RtpForwarder.h
../../include/rtp++/network/RtpNetworkInterface.h
../../include/rtp++/network/RtspAdapterRtpNetworkInterface.h
../../include/rtp++/network/Sctp.h
../../include/rtp++/network/SctpAssociation.h
../../include/rtp++/network/SctpAssociationV2.h
../../include/rtp++/network/SctpRtpNetworkInterface.h
../../include/rtp++/network/TcpRtpConnection.h
../../include/rtp++/network/UdpForwarder.h
../../include/rtp++/network/UdpRtpNetworkInterface.h
../../include/rtp++/network/UdpSocketWrapper.h
../../include/rtp++/network/VirtualUdpRtpNetworkInterface.h
)
SET(RFC2326_HEADERS
../../include/rtp++/rfc2326/HeaderFields.h
../../include/rtp++/rfc2326/IRtspAgent.h
../../include/rtp++/rfc2326/MessageBody.h
../../include/rtp++/rfc2326/LiveRtspServer.h
../../include/rtp++/rfc2326/LiveServerMediaSession.h
../../include/rtp++/rfc2326/Register.h
../../include/rtp++/rfc2326/ResponseCodes.h
../../include/rtp++/rfc2326/Rfc2326.h
../../include/rtp++/rfc2326/RtpInfo.h
../../include/rtp++/rfc2326/RtspClient.h
../../include/rtp++/rfc2326/RtspClientConnection.h
../../include/rtp++/rfc2326/RtspClientSession.h
../../include/rtp++/rfc2326/RtspErrorCategory.h
../../include/rtp++/rfc2326/RtspMessage.h
../../include/rtp++/rfc2326/RtspMethod.h
../../include/rtp++/rfc2326/RtspServer.h
../../include/rtp++/rfc2326/RtspServerConnection.h
../../include/rtp++/rfc2326/RtspUri.h
../../include/rtp++/rfc2326/RtspUtil.h
../../include/rtp++/rfc2326/SelfRegisteringRtspServer.h
../../include/rtp++/rfc2326/SelfRegisteringRtspServerConnection.h
../../include/rtp++/rfc2326/ServerMediaSession.h
../../include/rtp++/rfc2326/SessionGenerator.h
../../include/rtp++/rfc2326/Transport.h
)
SET(RFC3261_HEADERS
../../include/rtp++/rfc3261/ClientTransaction.h
../../include/rtp++/rfc3261/DialogManager.h
../../include/rtp++/rfc3261/Methods.h
../../include/rtp++/rfc3261/MessageBody.h
../../include/rtp++/rfc3261/ResponseCodes.h
../../include/rtp++/rfc3261/Registrar.h
../../include/rtp++/rfc3261/Rfc3261.h
../../include/rtp++/rfc3261/Protocols.h
../../include/rtp++/rfc3261/ServerTransaction.h
../../include/rtp++/rfc3261/SipAgent.h
../../include/rtp++/rfc3261/SipClientTcpConnection.h
../../include/rtp++/rfc3261/SipContext.h
../../include/rtp++/rfc3261/SipDialog.h
../../include/rtp++/rfc3261/SipErrorCategory.h
../../include/rtp++/rfc3261/SipMessage.h
../../include/rtp++/rfc3261/SipProxy.h
../../include/rtp++/rfc3261/SipServerTcpConnection.h
../../include/rtp++/rfc3261/SipUri.h
../../include/rtp++/rfc3261/StatelessProxy.h
../../include/rtp++/rfc3261/TransactionUser.h
../../include/rtp++/rfc3261/TransportLayer.h
../../include/rtp++/rfc3261/UserAgent.h
../../include/rtp++/rfc3261/UserAgentBase.h
../../include/rtp++/rfc3261/UserAgentClient.h
../../include/rtp++/rfc3261/UserAgentServer.h
)
SET(RFC3264_HEADERS
../../include/rtp++/rfc3264/OfferAnswerModel.h
../../include/rtp++/rfc3264/Rfc3264.h
)
SET(RFC3550_HEADERS
../../include/rtp++/rfc3550/ComputeRtcpInterval.h
../../include/rtp++/rfc3550/MemberEntry.h
../../include/rtp++/rfc3550/Rfc3550.h
../../include/rtp++/rfc3550/Rtcp.h
../../include/rtp++/rfc3550/RtcpParser.h
../../include/rtp++/rfc3550/RtcpReportManager.h
../../include/rtp++/rfc3550/Rfc3550RtcpTransmissionTimer.h
../../include/rtp++/rfc3550/Rfc3550RtcpValidator.h
../../include/rtp++/rfc3550/RtpConstants.h
../../include/rtp++/rfc3550/RtpHeader.h
../../include/rtp++/rfc3550/SdesInformation.h
../../include/rtp++/rfc3550/SessionDatabase.h
)
SET(RFC3581_HEADERS
../../include/rtp++/rfc3581/Rfc3581.h
)
SET(RFC3611_HEADERS
../../include/rtp++/rfc3611/Rfc3611.h
../../include/rtp++/rfc3611/RtcpXr.h
../../include/rtp++/rfc3611/RtcpXrFactory.h
../../include/rtp++/rfc3611/XrMemberEntry.h
)
SET(RFC3711_HEADERS
../../include/rtp++/rfc3711/Rfc3711.h
)
SET(RFC4145_HEADERS
../../include/rtp++/rfc4145/Rfc4145.h
)
SET(RFC4566_HEADERS
../../include/rtp++/rfc4566/MediaDescription.h
../../include/rtp++/rfc4566/Rfc4566.h
../../include/rtp++/rfc4566/SdpParser.h
../../include/rtp++/rfc4566/SessionDescription.h
)
SET(RFC4571_HEADERS
../../include/rtp++/rfc4571/MuxedTcpRtpNetworkInterface.h
../../include/rtp++/rfc4571/PassiveTcpRtpNetworkInterface.h
../../include/rtp++/rfc4571/Rfc4571.h
../../include/rtp++/rfc4571/TcpRtpNetworkInterface.h
)
SET(RFC4585_HEADERS
../../include/rtp++/rfc4585/FeedbackManager.h
../../include/rtp++/rfc4585/RtcpFb.h
../../include/rtp++/rfc4585/Rfc4585.h
../../include/rtp++/rfc4585/Rfc4585RtcpParser.h
../../include/rtp++/rfc4585/Rfc4585RtcpReportManager.h
../../include/rtp++/rfc4585/Rfc4585RtcpTransmissionTimer.h
)
SET(RFC4588_HEADERS
../../include/rtp++/rfc4588/Rfc4588.h
)
SET(RFC4867_HEADERS
../../include/rtp++/rfc4867/Rfc4867.h
../../include/rtp++/rfc4867/Rfc4867Packetiser.h
)
SET(RFC5285_HEADERS
../../include/rtp++/rfc5285/Extmap.h
../../include/rtp++/rfc5285/HeaderExtensionElement.h
../../include/rtp++/rfc5285/Rfc5285.h
../../include/rtp++/rfc5285/RtpHeaderExtension.h
)
SET(RFC5506_HEADERS
../../include/rtp++/rfc5506/Rfc5506.h
../../include/rtp++/rfc5506/Rfc5506RtcpValidator.h
)
SET(RFC5761_HEADERS
../../include/rtp++/rfc5761/Rfc5761.h
)
SET(RFC5762_HEADERS
../../include/rtp++/rfc5762/MuxedDccpRtpNetworkInterface.h
../../include/rtp++/rfc5762/PassiveDccpRtpNetworkInterface.h
../../include/rtp++/rfc5762/Rfc5762.h
../../include/rtp++/rfc5762/DccpRtpNetworkInterface.h
)
SET(RFC5888_HEADERS
../../include/rtp++/rfc5888/Rfc5888.h
)
SET(RFC6051_HEADERS
../../include/rtp++/rfc6051/Rfc6051.h
)
SET(RFC6184_HEADERS
../../include/rtp++/rfc6184/Rfc6184.h
../../include/rtp++/rfc6184/Rfc6184Packetiser.h
)
SET(RFC6190_HEADERS
../../include/rtp++/rfc6190/Rfc6190.h
../../include/rtp++/rfc6190/Rfc6190Packetiser.h
)
SET(RFCHEVC_HEADERS
../../include/rtp++/rfchevc/Rfchevc.h
../../include/rtp++/rfchevc/RfchevcPacketiser.h
)
SET(RTO_HEADERS
../../include/rtp++/rto/AR2Predictor.h
../../include/rtp++/rto/BasicRtoEstimator.h
../../include/rtp++/rto/Channel.h
../../include/rtp++/rto/ChannelDataSource.h
../../include/rtp++/rto/CrossPathRtoEstimator.h
../../include/rtp++/rto/NormalDistribution.h
../../include/rtp++/rto/MovingAveragePredictor.h
../../include/rtp++/rto/MultipathRtoEstimator.h
../../include/rtp++/rto/MultipathRtoManagerInterface.h
../../include/rtp++/rto/PacketInterarrivalTimePredictor.h
../../include/rtp++/rto/PacketTransmissionInfo.h
../../include/rtp++/rto/PredictorBase.h
../../include/rtp++/rto/PredictorTypes.h
../../include/rtp++/rto/RetransmissionInfo.h
../../include/rtp++/rto/RtoEstimationSession.h
../../include/rtp++/rto/RtoManagerInterface.h
../../include/rtp++/rto/SimpleRtoEstimator.h
../../include/rtp++/rto/StatisticsManager.h
../../include/rtp++/rto/VirtualRtpSessionBase.h
../../include/rtp++/rto/VirtualRtpSession.h
../../include/rtp++/rto/z.h
)
SET(SCHEDULING_HEADERS
../../include/rtp++/scheduling/AckBasedRtpScheduler.h
../../include/rtp++/scheduling/DistributedScheduler.h
../../include/rtp++/scheduling/NadaScheduler.h
../../include/rtp++/scheduling/PacedRtpScheduler.h
../../include/rtp++/scheduling/RandomScheduler.h
../../include/rtp++/scheduling/RoundRobinScheduler.h
../../include/rtp++/scheduling/RtpScheduler.h
../../include/rtp++/scheduling/SchedulerFactory.h
../../include/rtp++/scheduling/ScreamScheduler.h
../../include/rtp++/scheduling/XyzScheduler.h
)
SET(SCTP_HEADERS
../../include/rtp++/sctp/SctpH264RtpPolicyManager.h
../../include/rtp++/sctp/SctpH264RtpSessionManager.h
../../include/rtp++/sctp/SctpH265RtpPolicyManager.h
../../include/rtp++/sctp/SctpH265RtpSessionManager.h
../../include/rtp++/sctp/SctpRtpPolicy.h
../../include/rtp++/sctp/SctpSvcRtpPacketisationSessionManager.h
../../include/rtp++/sctp/SctpShvcRtpSessionManager.h
../../include/rtp++/sctp/SctpShvcRtpPacketisationSessionManager.h
../../include/rtp++/sctp/SctpSvcRtpSessionManager.h
)
SET(UTIL_HEADERS
../../include/rtp++/util/Base64.h
../../include/rtp++/util/RandomUtil.h
../../include/rtp++/util/TracesUtil.h
)
SET(CPP_UTIL_HEADERS
$ENV{CPP_UTIL_DIR}/include/cpputil/Buffer.h
$ENV{CPP_UTIL_DIR}/include/cpputil/Clock.h
$ENV{CPP_UTIL_DIR}/include/cpputil/ConsoleApplicationUtil.h
$ENV{CPP_UTIL_DIR}/include/cpputil/Conversion.h
$ENV{CPP_UTIL_DIR}/include/cpputil/ExceptionBase.h
$ENV{CPP_UTIL_DIR}/include/cpputil/FileUtil.h
$ENV{CPP_UTIL_DIR}/include/cpputil/GenericParameters.h
$ENV{CPP_UTIL_DIR}/include/cpputil/IBitStream.h
$ENV{CPP_UTIL_DIR}/include/cpputil/MakeService.h
$ENV{CPP_UTIL_DIR}/include/cpputil/OBitStream.h
$ENV{CPP_UTIL_DIR}/include/cpputil/RandomUtil.h
$ENV{CPP_UTIL_DIR}/include/cpputil/RunningAverageQueue.h
$ENV{CPP_UTIL_DIR}/include/cpputil/ServiceController.h
$ENV{CPP_UTIL_DIR}/include/cpputil/ServiceManager.h
$ENV{CPP_UTIL_DIR}/include/cpputil/ServiceThread.h
$ENV{CPP_UTIL_DIR}/include/cpputil/StringTokenizer.h
$ENV{CPP_UTIL_DIR}/include/cpputil/Utility.h
)

SET(RTP_SRCS
${CORE_SRCS} 
${APPLICATION_SRCS}
${EXPERIMENTAL_SRCS}
${MEDIA_SRCS}
${MEDIA_H264_SRCS} 
${MEDIA_H265_SRCS}
${MEDIA_SESSION_SRCS}
${MPRTP_SRCS}
${NETWORK_SRCS} 
${RFC2326_SRCS}
${RFC3261_SRCS}
${RFC3264_SRCS}
${RFC3550_SRCS}
${RFC3581_SRCS}
${RFC3611_SRCS}
${RFC3711_SRCS}
${RFC4145_SRCS} 
${RFC4566_SRCS}
${RFC4571_SRCS}
${RFC4585_SRCS}
${RFC4588_SRCS}
${RFC4867_SRCS}
${RFC5285_SRCS}
${RFC5506_SRCS}
${RFC5761_SRCS}
${RFC5762_SRCS}
${RFC5888_SRCS}
${RFC6051_SRCS}
${RFC6184_SRCS}
${RFC6190_SRCS}
${RFCHEVC_SRCS}
${RTO_SRCS}
${SCHEDULING_SRCS}
${SCTP_SRCS}
${UTIL_SRCS}
)

SOURCE_GROUP("Source Files\\core" FILES ${CORE_SRCS})
SOURCE_GROUP("Source Files\\application" FILES ${APPLICATION_SRCS})
SOURCE_GROUP("Source Files\\experimental" FILES ${EXPERIMENTAL_SRCS})
SOURCE_GROUP("Source Files\\media" FILES ${MEDIA_SRCS})
SOURCE_GROUP("Source Files\\media\\h264" FILES ${MEDIA_H264_SRCS})
SOURCE_GROUP("Source Files\\media\\h265" FILES ${MEDIA_H265_SRCS})
SOURCE_GROUP("Source Files\\mediasession" FILES ${MEDIA_SESSION_SRCS})
SOURCE_GROUP("Source Files\\mprtp" FILES ${MPRTP_SRCS})
SOURCE_GROUP("Source Files\\network" FILES ${NETWORK_SRCS})
SOURCE_GROUP("Source Files\\rfc2326" FILES ${RFC2326_SRCS})
SOURCE_GROUP("Source Files\\rfc3261" FILES ${RFC3261_SRCS})
SOURCE_GROUP("Source Files\\rfc3264" FILES ${RFC3264_SRCS})
SOURCE_GROUP("Source Files\\rfc3550" FILES ${RFC3550_SRCS})
SOURCE_GROUP("Source Files\\rfc3581" FILES ${RFC3581_SRCS})
SOURCE_GROUP("Source Files\\rfc3611" FILES ${RFC3611_SRCS})
SOURCE_GROUP("Source Files\\rfc3711" FILES ${RFC3711_SRCS})
SOURCE_GROUP("Source Files\\rfc4145" FILES ${RFC4145_SRCS})
SOURCE_GROUP("Source Files\\rfc4566" FILES ${RFC4566_SRCS})
SOURCE_GROUP("Source Files\\rfc4571" FILES ${RFC4571_SRCS})
SOURCE_GROUP("Source Files\\rfc4585" FILES ${RFC4585_SRCS})
SOURCE_GROUP("Source Files\\rfc4588" FILES ${RFC4588_SRCS})
SOURCE_GROUP("Source Files\\rfc4867" FILES ${RFC4867_SRCS})
SOURCE_GROUP("Source Files\\rfc5285" FILES ${RFC5285_SRCS})
SOURCE_GROUP("Source Files\\rfc5506" FILES ${RFC5506_SRCS})
SOURCE_GROUP("Source Files\\rfc5761" FILES ${RFC5761_SRCS})
SOURCE_GROUP("Source Files\\rfc5762" FILES ${RFC5762_SRCS})
SOURCE_GROUP("Source Files\\rfc5888" FILES ${RFC5888_SRCS})
SOURCE_GROUP("Source Files\\rfc6051" FILES ${RFC6051_SRCS})
SOURCE_GROUP("Source Files\\rfc6184" FILES ${RFC6184_SRCS})
SOURCE_GROUP("Source Files\\rfc6190" FILES ${RFC6190_SRCS})
SOURCE_GROUP("Source Files\\rfchevc" FILES ${RFCHEVC_SRCS})
SOURCE_GROUP("Source Files\\rto" FILES ${RTO_SRCS})
SOURCE_GROUP("Source Files\\scheduling" FILES ${SCHEDULING_SRCS})
SOURCE_GROUP("Source Files\\sctp" FILES ${SCTP_SRCS})
SOURCE_GROUP("Source Files\\util" FILES ${UTIL_SRCS})


SOURCE_GROUP("Header Files\\cpp-util" FILES ${CPP_UTIL_HEADERS})
SOURCE_GROUP("Header Files\\core" FILES ${CORE_HEADERS})
SOURCE_GROUP("Header Files\\application" FILES ${APPLICATION_HEADERS})
SOURCE_GROUP("Header Files\\experimental" FILES ${EXPERIMENTAL_HEADERS})
SOURCE_GROUP("Header Files\\media" FILES ${MEDIA_HEADERS})
SOURCE_GROUP("Header Files\\media\\h264" FILES ${MEDIA_H264_HEADERS})
SOURCE_GROUP("Header Files\\media\\h265" FILES ${MEDIA_H265_HEADERS})
SOURCE_GROUP("Header Files\\mediasession" FILES ${MEDIA_SESSION_HEADERS})
SOURCE_GROUP("Header Files\\mprtp" FILES ${MPRTP_HEADERS})
SOURCE_GROUP("Header Files\\network" FILES ${NETWORK_HEADERS})
SOURCE_GROUP("Header Files\\rfc2326" FILES ${RFC2326_HEADERS})
SOURCE_GROUP("Header Files\\rfc3261" FILES ${RFC3261_HEADERS})
SOURCE_GROUP("Header Files\\rfc3264" FILES ${RFC3264_HEADERS})
SOURCE_GROUP("Header Files\\rfc3550" FILES ${RFC3550_HEADERS})
SOURCE_GROUP("Header Files\\rfc3581" FILES ${RFC3581_HEADERS})
SOURCE_GROUP("Header Files\\rfc3611" FILES ${RFC3611_HEADERS})
SOURCE_GROUP("Header Files\\rfc3711" FILES ${RFC3711_HEADERS})
SOURCE_GROUP("Header Files\\rfc4145" FILES ${RFC4145_HEADERS})
SOURCE_GROUP("Header Files\\rfc4566" FILES ${RFC4566_HEADERS})
SOURCE_GROUP("Header Files\\rfc4571" FILES ${RFC4571_HEADERS})
SOURCE_GROUP("Header Files\\rfc4585" FILES ${RFC4585_HEADERS})
SOURCE_GROUP("Header Files\\rfc4588" FILES ${RFC4588_HEADERS})
SOURCE_GROUP("Header Files\\rfc4867" FILES ${RFC4867_HEADERS})
SOURCE_GROUP("Header Files\\rfc5285" FILES ${RFC5285_HEADERS})
SOURCE_GROUP("Header Files\\rfc5506" FILES ${RFC5506_HEADERS})
SOURCE_GROUP("Header Files\\rfc5761" FILES ${RFC5761_HEADERS})
SOURCE_GROUP("Header Files\\rfc5762" FILES ${RFC5762_HEADERS})
SOURCE_GROUP("Header Files\\rfc5888" FILES ${RFC5888_HEADERS})
SOURCE_GROUP("Header Files\\rfc6051" FILES ${RFC6051_HEADERS})
SOURCE_GROUP("Header Files\\rfc6184" FILES ${RFC6184_HEADERS})
SOURCE_GROUP("Header Files\\rfc6190" FILES ${RFC6190_HEADERS})
SOURCE_GROUP("Header Files\\rfchevc" FILES ${RFCHEVC_HEADERS})
SOURCE_GROUP("Header Files\\rto" FILES ${RTO_HEADERS})
SOURCE_GROUP("Header Files\\scheduling" FILES ${SCHEDULING_HEADERS})
SOURCE_GROUP("Header Files\\sctp" FILES ${SCTP_HEADERS})
SOURCE_GROUP("Header Files\\util" FILES ${UTIL_HEADERS})


SET(RTP_HEADERS 
${CPP_UTIL_HEADERS}
${CORE_HEADERS}
${APPLICATION_HEADERS}
${EXPERIMENTAL_HEADERS}
${MEDIA_HEADERS}
${MEDIA_H264_HEADERS}
${MEDIA_H265_HEADERS}
${MEDIA_SESSION_HEADERS}
${MPRTP_HEADERS}
${NETWORK_HEADERS}
${RFC2326_HEADERS}
${RFC3261_HEADERS}
${RFC3264_HEADERS}
${RFC3550_HEADERS}
${RFC3581_HEADERS}
${RFC3611_HEADERS}
${RFC3711_HEADERS}
${RFC4145_HEADERS}
${RFC4566_HEADERS}
${RFC4571_HEADERS}
${RFC4585_HEADERS}
${RFC4588_HEADERS}
${RFC4867_HEADERS}
${RFC5285_HEADERS}
${RFC5506_HEADERS}
${RFC5761_HEADERS}
${RFC5762_HEADERS}
${RFC5888_HEADERS}
${RFC6051_HEADERS}
${RFC6184_HEADERS}
${RFC6190_HEADERS}
${RFCHEVC_HEADERS}
${RTO_HEADERS}
${SCHEDULING_HEADERS}
${SCTP_HEADERS}
${UTIL_HEADERS}
)

INCLUDE_DIRECTORIES(
${rtp++Includes}
${rtp++_SOURCE_DIR}/Libs/MultiPartyRewriter_MMSys/src
${rtp++Includes}
${rtp++_SOURCE_DIR}/Libs/OpenH264Codec
${rtp++_SOURCE_DIR}/Libs/VppH264Codec
${rtp++_SOURCE_DIR}/Libs/X264Codec
${rtp++_SOURCE_DIR}/Libs/X265Codec
)

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
# 64bit
  SET(CMAKE_CXX_FLAGS "-fPIC")
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
# 32 bit
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

# ADD_MSVC_PRECOMPILED_HEADER("CorePch.h" "CorePch.cpp" CORE_SRCS)

ADD_LIBRARY( Core STATIC ${RTP_SRCS} ${RTP_HEADERS})


# adding precompiled header support on windows
if (MSVC)
   set_target_properties(Core PROPERTIES COMPILE_FLAGS "/YuCorePch.h")
   set_source_files_properties(CorePch.cpp PROPERTIES COMPILE_FLAGS "/YcCorePch.h")
endif(MSVC)
