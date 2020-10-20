#include "CorePch.h"
#include <rtp++/SvcRtpPacketisationSessionManager.h>
#include <cpputil/Conversion.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>
#include <rtp++/media/h264/SvcExtensionHeader.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/application/ApplicationParameters.h>

namespace rtp_plus_plus
{

using media::MediaSample;

std::unique_ptr<SvcRtpPacketisationSessionManager> SvcRtpPacketisationSessionManager::create(boost::asio::io_service& ioService,
                                                                                             const GenericParameters& applicationParameters)
{
  return std::unique_ptr<SvcRtpPacketisationSessionManager>( new SvcRtpPacketisationSessionManager(ioService, applicationParameters) );
}

SvcRtpPacketisationSessionManager::SvcRtpPacketisationSessionManager(boost::asio::io_service& ioService,
                                                                     const GenericParameters& applicationParameters)
  :GroupedRtpSessionManager(applicationParameters),
    m_bVerbose(false),
    m_uiAccessUnitIndex(0),
    m_uiLastRtpSN(0)
{
  boost::optional<bool> verbose = applicationParameters.getBoolParameter(app::ApplicationParameters::vvv);
  if (verbose)
    m_bVerbose = *verbose;
}

SvcRtpPacketisationSessionManager::~SvcRtpPacketisationSessionManager()
{
  VLOG(1) << LOG_MODIFY_WITH_CARE
          << " Total AUs: " << m_uiAccessUnitIndex - 1
          << " Last RTP Packet SN: " << m_uiLastRtpSN;
}

void SvcRtpPacketisationSessionManager::doSend(const MediaSample& mediaSample, const std::string &/*sMid*/)
{
  // NOOP
}

void SvcRtpPacketisationSessionManager::doSend(const MediaSample& mediaSample)
{
  //  std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(mediaSample);

  //  // instead of sending video, we just write the information to log file
  //  LOG(INFO) << "Outgoing media sample: TS: " << mediaSample.getStartTime()
  //            << " Size: " << mediaSample.getPayloadSize()
  //            << " " << rtpPackets.size() << " RTP packets: ";

  //  if (m_bVerbose)
  //  {
  //    for (auto& rtpPacket: rtpPackets)
  //    {
  //      VLOG(2) << rtpPacket.getHeader();
  //    }
  //  }
  // HACK: example code to illustrate what to do here: this code just select the RTP session via
  // round robin
  Buffer buffer = mediaSample.getDataBuffer();
  IBitStream in(buffer);
  // forbidden
  in.skipBits(1);
  // nri
  uint32_t uiNri = 0, uiNut = 0;
  in.read(uiNri, 2);
  //  NUT
  in.read(uiNut, 5);
  int gSessionIndex = (uiNut==media::h264::NUT_CODED_SLICE_EXT||uiNut==media::h264::NUT_RESERVED_21);// NUT_RESERVED_21 is in this old version of the encoder

  RtpSession::ptr pRtpSession = getRtpSession(getMid(gSessionIndex));


  std::vector<RtpPacket> rtpPackets = pRtpSession->packetise(mediaSample);
#if 0
  DLOG(INFO) << "Packetizing video received TS: " << mediaSample.getStartTime()
             << " Size: " << mediaSample.getDataBuffer().getSize()
             << " into " << rtpPackets.size() << " RTP packets";

#endif
  // pRtpSession->sendRtpPackets(rtpPackets);
}

void SvcRtpPacketisationSessionManager::doSend(const std::vector<MediaSample>& vMediaSamples, const std::string &/*sMid*/)
{
  // NOOP
}

void SvcRtpPacketisationSessionManager::doSend(const std::vector<MediaSample>& vMediaSamples)
{
  //  boost::mutex::scoped_lock l(m_lock);
  //  boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
  //  uint32_t uiUsecs = 0;
  //  if (!m_tLastAu.is_not_a_date_time())
  //  {
  //    uiUsecs = static_cast<uint32_t>((tNow - m_tLastAu).total_microseconds());
  //  }

  //  std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(vMediaSamples);
  //  std::vector<std::vector<uint16_t> > vRtpInfo = m_pRtpSession->getRtpPacketisationInfo();
  //  assert(vRtpInfo.size() == vMediaSamples.size());

  //  assert(!vMediaSamples.empty());
  //  assert(!rtpPackets.empty());
  //  uint32_t uiDiffRtp = 0;
  //  if (!m_tLastAu.is_not_a_date_time())
  //  {
  //    uiDiffRtp = rtpPackets[0].getRtpTimestamp() - m_uiLastRtp;
  //  }
  //  // Diff in normal time
  //  double dTime = 0.0;
  //  RtpTime::convertRtpToSystemTimestamp(dTime, uiDiffRtp, 90000);

  //  bool bH264 = false;
  //  if (m_sCodec == rfc6184::H264 || m_sCodec == rfc6190::H264_SVC)
  //    bH264 = true;

  //  uint32_t iRtpPacketIndex = 0;

  //  for (size_t i = 0; i < vMediaSamples.size(); ++i)
  //  {
  //    std::ostringstream ostr;
  //    for (uint16_t uiSN : vRtpInfo[i])
  //    {
  //      ostr << " <<" << uiSN << ">>";
  //    }

  //    if (bH264)
  //    {
  //      using boost::optional;
  //      using namespace media::h264;

  //      // OK since we only using 2 layers for now
  //      NalUnitType eType = getNalUnitType(vMediaSamples[i]);
  //      std::ostringstream layer;
  //      switch (eType)
  //      {
  //        case media::h264::NUT_PREFIX_NAL_UNIT:
  //        {
  //          optional<svc::SvcExtensionHeader> pSvcHeader = svc::extractSvcExtensionHeader(vMediaSamples[i]);
  //          layer << "BL ";
  //          assert (pSvcHeader);
  //          layer << pSvcHeader->dependency_id << ":"
  //                << pSvcHeader->quality_id << ":"
  //                << pSvcHeader->temporal_id << " ";
  //          break;
  //        }
  //        case media::h264::NUT_CODED_SLICE_EXT:
  //        case media::h264::NUT_RESERVED_21:
  //        {
  //          optional<svc::SvcExtensionHeader> pSvcHeader = svc::extractSvcExtensionHeader(vMediaSamples[i]);
  //          layer << "EL ";
  //          assert (pSvcHeader);
  //          layer << pSvcHeader->dependency_id << ":"
  //                << pSvcHeader->quality_id << ":"
  //                << pSvcHeader->temporal_id << " ";
  //          break;
  //        }
  //        default:
  //        {
  //          layer << "BL 0:0:0 ";
  //          break;
  //        }
  //      }

  //      VLOG(2) << LOG_MODIFY_WITH_CARE
  //              << " AU #: " << m_uiAccessUnitIndex
  //              << " NUT: " << eType << " " << layer.str()
  //              << " " << vMediaSamples[i].getPayloadSize() << " RTP SN: ["
  //              << ostr.str() << " ]";
  //      ;
  //    }
  //    else
  //    {
  //      VLOG(2) << LOG_MODIFY_WITH_CARE
  //              << " AU #: " << m_uiAccessUnitIndex
  //              << " " << vMediaSamples[i].getPayloadSize() << " RTP SN: ["
  //              << ostr.str() << " ]";
  //      ;
  //    }

  //    if (m_bVerbose)
  //    {
  //      for (size_t j = iRtpPacketIndex; j < iRtpPacketIndex + vRtpInfo[j].size(); ++j)
  //      {
  //        VLOG(2) << rtpPackets[j].getHeader();
  //      }
  //    }
  //    iRtpPacketIndex += vRtpInfo[i].size();
  //  }

  //  m_tLastAu = tNow;
  //  m_uiLastRtp = rtpPackets[0].getRtpTimestamp();
  //  ++m_uiAccessUnitIndex;

  //  uint32_t uiSize = vRtpInfo[vMediaSamples.size() - 1].size();
  //  m_uiLastRtpSN = vRtpInfo[vMediaSamples.size() - 1][uiSize - 1];

  // HACK: example code to illustrate what to do here: this code just select the RTP session via
  // round robin
  //boost::posix_time::ptime currentTime =  boost::posix_time::microsec_clock::universal_time();
  for (int i=0; i<2;i++)
  {
    std::vector<MediaSample> MediaSamples;
    for (const MediaSample& mediaSample: vMediaSamples)
    {
      Buffer buffer = mediaSample.getDataBuffer();
      IBitStream in(buffer);

      // forbidden
      in.skipBits(1);
      // nri
      uint32_t uiNri = 0, uiNut = 0;
      in.read(uiNri, 2);
      //  NUT
      in.read(uiNut, 5);
      if(i==(uiNut==media::h264::NUT_CODED_SLICE_EXT||uiNut==media::h264::NUT_RESERVED_21))// NUT_RESERVED_21 is in this old version of the encoder
        MediaSamples.push_back(mediaSample);

    }
    RtpSession::ptr pRtpSession = getRtpSession(getMid(i));
    //gSessionIndex = (gSessionIndex + 1)%getSessionCount();

    std::vector<RtpPacket> rtpPackets = pRtpSession->packetise(MediaSamples/*, currentTime*/);
    std::vector<std::vector<uint16_t> > vRtpInfo = pRtpSession->getRtpPacketisationInfo();
    assert(vRtpInfo.size() == MediaSamples.size());

    for (size_t j = 0; j < MediaSamples.size(); ++j)
    {
      std::ostringstream ostr;
      for (uint16_t uiSN : vRtpInfo[j])
      {
        ostr << " <<" << uiSN << ">>";
      }
      VLOG(2) << LOG_MODIFY_WITH_CARE
              << " AU #: " << m_uiAccessUnitIndex
              << " MID: " << getMid(i)
              << " " << MediaSamples[j].getPayloadSize() << " RTP SN: ["
              << ostr.str() << " ]";
    }

#if 0
    DLOG(INFO) << "Packetizing video received TS: " << mediaSample.getStartTime()
               << " Size: " << mediaSample.getDataBuffer().getSize()
               << " into " << rtpPackets.size() << " RTP packets";

#endif

    uint32_t uiSize = vRtpInfo[MediaSamples.size() - 1].size();
    m_uiLastRtpSN = vRtpInfo[MediaSamples.size() - 1][uiSize - 1];
    // pRtpSession->sendRtpPackets(rtpPackets);
  }

  ++m_uiAccessUnitIndex;

}

}
