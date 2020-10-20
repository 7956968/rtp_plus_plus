#pragma once

/// rtp++ version info
/// 0.2.1 - Added RTSP server implementation
/// 0.3.0 - Started SRTP support, SVC Packetisation application
/// 0.3.1 - RTSP server now has profile-level-id and sprop-parameter-sets in SDP
/// 0.3.2 - Compilation under VS 2012 Update 3 to target Windows XP
/// 0.3.3 - Compilation under VS 2012 Update 3 to target Windows XP, replaced 
///         AsyncStreamMediaSource with NalUnitMediaSource
/// 0.3.4 - Compilation under VS 2012 Update 3 to target Windows XP with boost and glog recompilation
/// 0.3.5 - RTSP server configurability
/// 0.3.6 - Marker bit BF in CldRtspServer
/// 0.3.7 - DCCP transport and RTP over RTSP support added to RtspClient.
/// 0.4.0 - SIP functionality
/// 0.5.0 - Introduced RPC for Cloud Control
/// 0.5.1 - Bugfix in SimplePacketLossDetection: now handling reordered packets.

#define RTP_PLUS_PLUS_VERSION_STRING "0.5.1"
