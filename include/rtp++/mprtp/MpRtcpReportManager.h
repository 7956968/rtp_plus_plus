#pragma once
#include <memory>
#include <rtp++/mprtp/MpRtpFeedbackManager.h>
#include <rtp++/rfc4585/Rfc4585RtcpReportManager.h>

namespace rtp_plus_plus
{
namespace mprtp
{

/// @def DEBUG_MP_RTCP Adds debugging information
// #define DEBUG_MP_RTCP

class MpRtcpReportManager : public rfc4585::RtcpReportManager
{
public:

    MpRtcpReportManager(boost::asio::io_service& ioService,
                        RtpSessionState& rtpSessionState,
                        const RtpSessionParameters& rtpParameters,
                        RtpReferenceClock& referenceClock,
                        std::unique_ptr<rfc3550::SessionDatabase>& pSessionDatabase,
                        const RtpSessionStatistics& rtpSessionStatistics,
                        uint16_t uiFlowId);

protected:
    virtual CompoundRtcpPacket createAdditionalReports(const rfc3550::RtcpReportData_t& reportData, const CompoundRtcpPacket& compoundPacket);

private:
    uint16_t m_uiFlowId;
};

}
}
