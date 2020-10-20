#include "CorePch.h"
#include <rtp++/RtpPacketisationSessionManager.h>
#include <cpputil/Conversion.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>
#include <rtp++/media/h264/SvcExtensionHeader.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/application/ApplicationParameters.h>

namespace rtp_plus_plus
{

using media::MediaSample;

std::unique_ptr<RtpPacketisationSessionManager> RtpPacketisationSessionManager::create(boost::asio::io_service& ioService,
                                                                                       const GenericParameters& applicationParameters)
{
  return std::unique_ptr<RtpPacketisationSessionManager>( new RtpPacketisationSessionManager(ioService, applicationParameters) );
}

RtpPacketisationSessionManager::RtpPacketisationSessionManager(boost::asio::io_service& ioService,
                                                               const GenericParameters& applicationParameters)
  :RtpSessionManager(ioService, applicationParameters),
    m_bVerbose(false),
    m_uiLastRtp(0),
    m_uiAccessUnitIndex(0),
    m_uiLastRtpSN(0)
{
  boost::optional<bool> verbose = applicationParameters.getBoolParameter(app::ApplicationParameters::vvv);
  if (verbose)
    m_bVerbose = *verbose;
}

RtpPacketisationSessionManager::~RtpPacketisationSessionManager()
{
  VLOG(1) << LOG_MODIFY_WITH_CARE
          << " Total AUs: " << m_uiAccessUnitIndex - 1
          << " Last RTP Packet SN: " << m_uiLastRtpSN;
}

void RtpPacketisationSessionManager::send(const MediaSample& mediaSample)
{
  std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(mediaSample);

  // instead of sending video, we just write the information to log file
  LOG(INFO) << "Outgoing media sample: TS: " << mediaSample.getStartTime()
            << " Size: " << mediaSample.getPayloadSize()
            << " " << rtpPackets.size() << " RTP packets: ";

  if (m_bVerbose)
  {
    for (auto& rtpPacket: rtpPackets)
    {
      VLOG(2) << rtpPacket.getHeader();
    }
  }
}

void RtpPacketisationSessionManager::send(const std::vector<MediaSample>& vMediaSamples)
{
  boost::mutex::scoped_lock l(m_lock);
  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();

  std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(vMediaSamples);
  std::vector<std::vector<uint16_t> > vRtpInfo = m_pRtpSession->getRtpPacketisationInfo();
  assert(vRtpInfo.size() == vMediaSamples.size());

  assert(!vMediaSamples.empty());
  assert(!rtpPackets.empty());
  uint32_t uiDiffRtp = 0;
  if (!m_tLastAu.is_not_a_date_time())
  {
    uiDiffRtp = rtpPackets[0].getRtpTimestamp() - m_uiLastRtp;
  }
  // Diff in normal time
  double dTime = 0.0;
  RtpTime::convertRtpToSystemTimestamp(dTime, uiDiffRtp, 90000);

  bool bH264 = false;
  if (m_sCodec == rfc6184::H264 || m_sCodec == rfc6190::H264_SVC)
    bH264 = true;

  uint32_t iRtpPacketIndex = 0;

  for (size_t i = 0; i < vMediaSamples.size(); ++i)
  {
    std::ostringstream ostr;
    for (uint16_t uiSN : vRtpInfo[i])
    {
      ostr << " <<" << uiSN << ">>";
    }

    if (bH264)
    {
      using boost::optional;
      using namespace media::h264;

      // OK since we only using 2 layers for now
      NalUnitType eType = getNalUnitType(vMediaSamples[i]);
      std::ostringstream layer;
      switch (eType)
      {
        case media::h264::NUT_PREFIX_NAL_UNIT:
        {
          optional<svc::SvcExtensionHeader> pSvcHeader = svc::extractSvcExtensionHeader(vMediaSamples[i]);
          layer << "BL ";
          assert (pSvcHeader);
          layer << pSvcHeader->dependency_id << ":"
                << pSvcHeader->quality_id << ":"
                << pSvcHeader->temporal_id << " ";
          break;
        }
        case media::h264::NUT_CODED_SLICE_EXT:
        case media::h264::NUT_RESERVED_21:
        {
          optional<svc::SvcExtensionHeader> pSvcHeader = svc::extractSvcExtensionHeader(vMediaSamples[i]);
          layer << "EL ";
          assert (pSvcHeader);
          layer << pSvcHeader->dependency_id << ":"
                << pSvcHeader->quality_id << ":"
                << pSvcHeader->temporal_id << " ";
          break;
        }
        default:
        {
          layer << "BL 0:0:0 ";
          break;
        }
      }

      VLOG(2) << LOG_MODIFY_WITH_CARE
              << " AU #: " << m_uiAccessUnitIndex
              << " NUT: " << eType << " " << layer.str()
              << " " << vMediaSamples[i].getPayloadSize() << " RTP SN: ["
              << ostr.str() << " ]";
      ;
    }
    else
    {
      VLOG(2) << LOG_MODIFY_WITH_CARE
              << " AU #: " << m_uiAccessUnitIndex
              << " " << vMediaSamples[i].getPayloadSize() << " RTP SN: ["
              << ostr.str() << " ]";
      ;
    }

    if (m_bVerbose)
    {
      for (size_t j = iRtpPacketIndex; j < iRtpPacketIndex + vRtpInfo[j].size(); ++j)
      {
        VLOG(2) << rtpPackets[j].getHeader();
      }
    }
    iRtpPacketIndex += vRtpInfo[i].size();
  }

  m_tLastAu = tNow;
  m_uiLastRtp = rtpPackets[0].getRtpTimestamp();
  ++m_uiAccessUnitIndex;

  uint32_t uiSize = vRtpInfo[vMediaSamples.size() - 1].size();
  m_uiLastRtpSN = vRtpInfo[vMediaSamples.size() - 1][uiSize - 1];
}

}
