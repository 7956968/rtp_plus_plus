#include "CorePch.h"
#include <rtp++/media/h264/H264NalUnitTypes.h>
#include <rtp++/media/h264/SvcExtensionHeader.h>
#include <rtp++/sctp/SctpH264RtpPolicyManager.h>


namespace rtp_plus_plus
{

using namespace media::h264;
using media::MediaSample;

namespace sctp
{

SctpH264RtpPolicyManager::SctpH264RtpPolicyManager()
  :m_uiLastAuIndex(0),
    m_uiLevelIndex(0)
{

}

void SctpH264RtpPolicyManager::setPolicy(const SctpRtpPolicy &sctpPolicy)
{
  m_sctpPolicy = sctpPolicy;
}

uint32_t SctpH264RtpPolicyManager::getRtpChannelForAvcMedia(const MediaSample& mediaSample)
{
  const std::vector<PolicyDescriptor>& policies = m_sctpPolicy.getAdditionalRtpPolicies();
  // first check if there is a policy for the NAL type
  for (const PolicyDescriptor& desc: policies)
  {
    if (desc.FrameType == FRAME_TYPE_NALU)
    {
      NalUnitType eType = getNalUnitType(mediaSample);
      if (eType == desc.NaluType)
        return desc.Channel_Id;
    }
  }
  // otherwise check if there is a rule for AVC frame type
  int32_t iFrameType = determineAvcFrameType(mediaSample);
  for (const PolicyDescriptor& desc: policies)
  {
    if (desc.FrameType == iFrameType)
    {
      return desc.Channel_Id;
    }
  }
  // else use default RTP policy
  return m_sctpPolicy.getRtpPolicy().Channel_Id;
}

uint32_t SctpH264RtpPolicyManager::getRtpChannelForAvcMedia(const std::vector<MediaSample>& vMediaSamples)
{
  const std::vector<PolicyDescriptor>& policies = m_sctpPolicy.getAdditionalRtpPolicies();
  int32_t iMostImportantFrameType = determineAvcAccessUnitImportance(vMediaSamples);
  for (const PolicyDescriptor& desc: policies)
  {
    if (desc.FrameType == iMostImportantFrameType)
    {
      return desc.Channel_Id;
    }
  }
  // else use default RTP policy
  return m_sctpPolicy.getRtpPolicy().Channel_Id;
}

bool areDQTSet(const PolicyDescriptor& desc)
{
  return (desc.D_L_ID != TYPE_NOT_SET && desc.Q_T_ID != TYPE_NOT_SET && desc.T_ID != TYPE_NOT_SET);
}

bool areDQTSet(int32_t iD, int32_t iQ,int32_t iT)
{
  return (iD != TYPE_NOT_SET && iQ != TYPE_NOT_SET && iT != TYPE_NOT_SET);
}

const uint32_t LEVEL[] = {0, 2, 1, 2};
uint32_t SctpH264RtpPolicyManager::getRtpChannelForSvcMedia(const MediaSample& mediaSample, uint32_t uiAccessUnitNumber)
{
  if (uiAccessUnitNumber != m_uiLastAuIndex)
  {
    m_uiLevelIndex = (m_uiLevelIndex + 1)%4;
    m_uiLastAuIndex = uiAccessUnitNumber;
  }
  uint32_t uiTemporalLevel = LEVEL[m_uiLevelIndex];

  const std::vector<PolicyDescriptor>& policies = m_sctpPolicy.getAdditionalRtpPolicies();
  NalUnitType eType = getNalUnitType(mediaSample);
  // first check if there is a policy for the NAL type

  for (int i = policies.size() - 1; i >= 0 ; --i)
  {
    const PolicyDescriptor& desc = policies[i];
    if (desc.Layer == LAYER_TYPE_NALU)
    {
      if (eType == desc.NaluType)
      {
        // check if the descriptor is for all types, or only specific ones
        if (areDQTSet(desc))
        {
          // Must match on DQT
          // get DQT of media sample if possible
          int32_t iD = 0, iQ = 0, iT = 0;
          determineSvcLayer(mediaSample, iD, iQ, iT);
          if (areDQTSet(iD, iQ, iT))
          {
            if (desc.D_L_ID == iD &&
                desc.Q_T_ID == iQ &&
                desc.T_ID == iT)
            {
              // MATCH on DQT
              return desc.Channel_Id;
            }
          }
        }
        else
        {
          // match suffices
          return desc.Channel_Id;
        }
      }
    }
  }

  // Look for DQT match
  int32_t iD = 0, iQ = 0, iT = 0;
  int32_t iLayer = determineSvcLayer(mediaSample, iD, iQ, iT);
  // HACK: override and use predefined T
  iT = uiTemporalLevel;
  for (int i = policies.size() - 1; i >= 0 ; --i)
  {
    const PolicyDescriptor& desc = policies[i];
    if (iD != -1 && iQ != -1 && iT != -1)
    {
      if (desc.D_L_ID == iD &&
          desc.Q_T_ID == iQ &&
          desc.T_ID == iT)
      {
        return desc.Channel_Id;
      }
    }
  }

  // just try match on layer
  for (int i = policies.size() - 1; i >= 0 ; --i)
  {
    const PolicyDescriptor& desc = policies[i];
    if (desc.Layer == iLayer)
    {
      return desc.Channel_Id;
    }
  }

  // else use default RTP policy
  VLOG(2) << "No rule found for NALU: "
          << eType << " Layer:" << iLayer
          << " DQT: " << iD << ":" << iQ << ":" << iT;

  return m_sctpPolicy.getRtpPolicy().Channel_Id;
}

int32_t SctpH264RtpPolicyManager::determineAvcFrameType(const MediaSample& mediaSample)
{
  using namespace media::h264;
  NalUnitType eType = getNalUnitType(mediaSample);
  VLOG(15) << "NAL Unit Type: " << eType;
  switch (eType)
  {
    case NUT_CODED_SLICE_OF_A_NON_IDR_PICTURE:
    case NUT_SUPPLEMENTAL_ENHANCEMENT_INFORMATION_SEI:
    {
      return sctp::FRAME_TYPE_P;
      break;
    }
    case NUT_CODED_SLICE_OF_AN_IDR_PICTURE:
    case NUT_SEQUENCE_PARAMETER_SET:
    case NUT_PICTURE_PARAMETER_SET:
    case NUT_SUBSET_SEQUENCE_PARAMETER_SET:
    {
      return sctp::FRAME_TYPE_I;
      break;
    }
    case NUT_ACCESS_UNIT_DELIMITER:
    case NUT_PREFIX_NAL_UNIT:
    default:
    {
      return sctp::FRAME_TYPE_B;
      break;
    }
  }
}

int32_t SctpH264RtpPolicyManager::determineAvcAccessUnitImportance(const std::vector<MediaSample>& vMediaSamples)
{
  // start with least importance
  int iMaxFrameImportance = sctp::FRAME_TYPE_B;
  for (size_t i = 0; i < vMediaSamples.size(); ++i)
  {
    int32_t iFrameImportance = determineAvcFrameType(vMediaSamples[i]);
    VLOG(2) << "NAL Unit Type: " << media::h264::getNalUnitType(vMediaSamples[i]) << " Frame type: " << iFrameImportance;
    if (iFrameImportance < iMaxFrameImportance)
    {
      iMaxFrameImportance = iFrameImportance;
    }
  }
  return iMaxFrameImportance;
}

int32_t SctpH264RtpPolicyManager::determineSvcLayer(const MediaSample& mediaSample, int32_t& iD, int32_t& iQ, int32_t& iT)
{
  using namespace media::h264;
  NalUnitType eType = getNalUnitType(mediaSample);
  switch (eType)
  {
    case NUT_PREFIX_NAL_UNIT:
    {
      boost::optional<svc::SvcExtensionHeader> pSvcHeader = svc::extractSvcExtensionHeader(mediaSample);
      if (pSvcHeader)
      {
        iD = pSvcHeader->dependency_id;
        iQ = pSvcHeader->quality_id;
        iT = pSvcHeader->temporal_id;
      }
      return LAYER_TYPE_BL;
      break;
    }
    case NUT_CODED_SLICE_EXT:
    case NUT_RESERVED_21: /* SVC encoder not updated to latest version of standard */
    {
      boost::optional<svc::SvcExtensionHeader> pSvcHeader = svc::extractSvcExtensionHeader(mediaSample);
      if (pSvcHeader)
      {
        iD = pSvcHeader->dependency_id;
        iQ = pSvcHeader->quality_id;
        iT = pSvcHeader->temporal_id;
      }
      return LAYER_TYPE_EL;
      break;
    }
    default:
    {
      return LAYER_TYPE_BL;
    }
  }
}

}
}

