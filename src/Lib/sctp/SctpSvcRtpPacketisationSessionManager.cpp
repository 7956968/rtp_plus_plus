#include "CorePch.h"
#include <rtp++/sctp/SctpSvcRtpPacketisationSessionManager.h>
#include <cpputil/Conversion.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>
#include <rtp++/media/h264/SvcExtensionHeader.h>
#include <rtp++/application/ApplicationParameters.h>

namespace rtp_plus_plus
{

using media::MediaSample;

namespace sctp
{

std::unique_ptr<SctpSvcRtpPacketisationSessionManager> SctpSvcRtpPacketisationSessionManager::create(boost::asio::io_service& ioService,
                                                                                       const GenericParameters& applicationParameters)
{
  return std::unique_ptr<SctpSvcRtpPacketisationSessionManager>( new SctpSvcRtpPacketisationSessionManager(ioService, applicationParameters) );
}

SctpSvcRtpPacketisationSessionManager::SctpSvcRtpPacketisationSessionManager(boost::asio::io_service& ioService,
                                                               const GenericParameters& applicationParameters)
  :RtpSessionManager(ioService, applicationParameters),
    m_bVerbose(false),
    m_uiAccessUnitIndex(0),
    m_uiLastRtpSN(0)
{
  // get SCTP AVC application parameters
  boost::optional<std::vector<std::string> > vPolicies = applicationParameters.getStringsParameter(app::ApplicationParameters::sctp_policies);
  if (vPolicies)
  {
    setSctpPolicies(*vPolicies);
  }
  else
  {
    VLOG(2) << "No SCTP policies configured, using default policies";
  }

  boost::optional<bool> verbose = applicationParameters.getBoolParameter(app::ApplicationParameters::vvv);
  if (verbose)
    m_bVerbose = *verbose;
}

SctpSvcRtpPacketisationSessionManager::~SctpSvcRtpPacketisationSessionManager()
{
  VLOG(1) << LOG_MODIFY_WITH_CARE
          << " Total AUs: " << m_uiAccessUnitIndex
          << " Last RTP Packet SN: " << m_uiLastRtpSN;
}

void SctpSvcRtpPacketisationSessionManager::setSctpPolicies(const std::vector<std::string>& vPolicies )
{
  boost::optional<SctpRtpPolicy> policies = SctpRtpPolicy::create(vPolicies);
  if (policies)
  {
    m_sctpPolicy = *policies;
    // TODO: need some way to update SCTP network interface
  }
  else
  {
    m_sctpPolicy = SctpRtpPolicy::createDefaultPolicy();
  }

  m_sctpPolicyManager.setPolicy(m_sctpPolicy);
}

void SctpSvcRtpPacketisationSessionManager::send(const MediaSample& mediaSample)
{
  uint32_t uiChannelId = m_sctpPolicyManager.getRtpChannelForSvcMedia(mediaSample, m_uiAccessUnitIndex);
  std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(mediaSample);

  // instead of sending video, we just write the information to log file
  LOG(INFO) << "SCTP Channel: " << uiChannelId
            << " Outgoing media sample: TS: " << mediaSample.getStartTime()
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

using namespace media::h264;

void SctpSvcRtpPacketisationSessionManager::send(const std::vector<MediaSample>& vMediaSamples)
{
  boost::mutex::scoped_lock l(m_lock);

  std::vector<std::vector<MediaSample> > vGroups;
  std::vector<uint32_t> vChannels;

  // create initial group
  assert(!vMediaSamples.empty());

  uint32_t uiLastChannel = UINT_MAX;

  // group into vectors of consecutive similar channels
  for (size_t i = 0; i < vMediaSamples.size(); ++i)
  {
    uint32_t uiChannel = m_sctpPolicyManager.getRtpChannelForSvcMedia(vMediaSamples[i], m_uiAccessUnitIndex);
    if (uiChannel != uiLastChannel)
    {
      // create new group
      vGroups.push_back(std::vector<MediaSample>());
      vChannels.push_back(uiChannel);
      uiLastChannel = uiChannel;
    }

    vGroups[vGroups.size()-1].push_back(vMediaSamples[i]);

    NalUnitType eType = getNalUnitType(vMediaSamples[i]);
    switch (eType)
    {
      case NUT_PREFIX_NAL_UNIT:
      {
        // skip the following NAL unit
        ++i;
        // applies to following NAL units as well
        vGroups[vGroups.size()-1].push_back(vMediaSamples[i]);
        break;
      }
      default:
        break;
    }
  }

  // packetise and send each group
  // NOTE: prefix NAL units must be assigned the same channel as the following NAL unit
  for (size_t i = 0; i < vGroups.size(); ++i)
  {
    auto& mediaSampleGroup = vGroups[i];
    std::vector<RtpPacket> rtpPackets = m_pRtpSession->packetise(mediaSampleGroup);
    std::vector<std::vector<uint16_t> > vRtpInfo = m_pRtpSession->getRtpPacketisationInfo();
    assert(vRtpInfo.size() == mediaSampleGroup.size());

    using boost::optional;
    using namespace media::h264;
    for (size_t j = 0; j < mediaSampleGroup.size(); ++j)
    {
      const MediaSample& mediaSample = mediaSampleGroup[j];
      // OK since we only using 2 layers for now
      NalUnitType eType = getNalUnitType(mediaSample);
      std::ostringstream layer;
      switch (eType)
      {
        case media::h264::NUT_PREFIX_NAL_UNIT:
        {
          optional<svc::SvcExtensionHeader> pSvcHeader = svc::extractSvcExtensionHeader(mediaSample);
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
          optional<svc::SvcExtensionHeader> pSvcHeader = svc::extractSvcExtensionHeader(mediaSample);
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

      // get sequence numbers
      std::ostringstream ostr;
      for (uint16_t uiSN : vRtpInfo[j])
      {
        ostr << " <<" << uiSN << ">>";
      }


      VLOG(2) << LOG_MODIFY_WITH_CARE
              << " AU #: " << m_uiAccessUnitIndex
              << " SCTP channel: " << vChannels[i]
              << " NUT: " << eType << " " << layer.str()
              << " (" << mediaSampleGroup[j].getPayloadSize() << ") RTP SN: [ "
              << ostr.str() << "] ";
                 ;
    }

    if (m_bVerbose)
    {
      for (auto& rtpPacket: rtpPackets)
      {
        VLOG(2) << rtpPacket.getHeader();
      }
    }

    m_uiLastRtpSN = rtpPackets[rtpPackets.size() - 1].getSequenceNumber();
  }

  ++m_uiAccessUnitIndex;
}

}
}
