#pragma once
#include <rtp++/network/AddressDescriptor.h>
#include <rtp++/RtpSessionParameters.h>
#include <rtp++/rfc4566/SessionDescription.h>

namespace rtp_plus_plus
{
namespace mprtp
{

// URN for SDP
extern const std::string EXTENSION_NAME_MPRTP;

// feature for MPRTP RTSP SETUP
extern const std::string SETUP_MPRTP;

// for RTSP Setup
extern const std::string DEST_MPRTP_ADDR;
extern const std::string SRC_MPRTP_ADDR;

extern const unsigned MAX_MISORDER;

extern const uint8_t SUBFLOW_SPECIFIC_REPORT;
extern const uint8_t INTERFACE_ADVERTISEMENT_IPV4;
extern const uint8_t INTERFACE_ADVERTISEMENT_IPV6;
extern const uint8_t INTERFACE_ADVERTISEMENT_DNS;

const uint8_t PT_MPRTCP = 211;

/// SDP attributes
extern const std::string MPRTP;
/**
 * Checks the parameters to determine if it is an MPRTP session
 * @param[in] rtpParameters The RTP session parameters to be checked
 */
extern bool isMpRtpSession(const RtpSessionParameters& rtpParameters);
/**
 * @brief Checks the session description to determine if it is an MPRTP session
 * @param[in] sessionDescription The session description to be checked
 */
extern bool isMpRtpSession(const rfc4566::SessionDescription& sessionDescription);
/**
 * returns the transport string needed for RTSP transport of MPRTP
 */
extern std::string getMpRtpRtspTransportString(const MediaInterfaceDescriptor_t& mediaInterfaceDescriptor);
/**
 * @brief returns media interface descriptor parsed from transport string
 * @param sMpRtpAddr The src_mprtp_addr or dest_mprtp_addr field to be parsed
 * @param bRtcpMux If RTP and RTCP are muxed.
 * @return an empty descriptor if it failed to parse
 *
 * The transport string should have the format "1 146.64.28.171 49170[; 2 146.64.28.172 49170]"
 */
extern MediaInterfaceDescriptor_t getMpRtpInterfaceDescriptorFromRtspTransportString(const std::string& sMpRtpAddr, bool bRtcpMux);

} // mprtp
} // rtp_plus_plus
