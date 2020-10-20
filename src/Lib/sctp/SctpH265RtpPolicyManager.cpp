#include "CorePch.h"
#include <rtp++/media/h265/H265NalUnitTypes.h>
#include <rtp++/sctp/SctpH265RtpPolicyManager.h>


namespace rtp_plus_plus
{

using namespace media::h265;
using media::MediaSample;

namespace sctp
{

SctpH265RtpPolicyManager::SctpH265RtpPolicyManager()
{

}

void SctpH265RtpPolicyManager::setPolicy(const SctpRtpPolicy &sctpPolicy)
{
  m_sctpPolicy = sctpPolicy;
}

uint32_t SctpH265RtpPolicyManager::getRtpChannelForHevcMedia(const MediaSample& mediaSample)
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
  int32_t iFrameType = determineHevcFrameType(mediaSample);
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

uint32_t SctpH265RtpPolicyManager::getRtpChannelForHevcMedia(const std::vector<MediaSample>& vMediaSamples)
{
  const std::vector<PolicyDescriptor>& policies = m_sctpPolicy.getAdditionalRtpPolicies();
  int32_t iMostImportantFrameType = determineHevcAccessUnitImportance(vMediaSamples);
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

bool areLTSet(const PolicyDescriptor& desc)
{
  return ( desc.D_L_ID != TYPE_NOT_SET && desc.Q_T_ID != TYPE_NOT_SET);
}
bool areLTSet( int32_t iL,int32_t iT)
{
  return (iL != TYPE_NOT_SET && iT != TYPE_NOT_SET);
}


uint32_t SctpH265RtpPolicyManager::getRtpChannelForShvcMedia(const MediaSample& mediaSample)
{
  const std::vector<PolicyDescriptor>& policies = m_sctpPolicy.getAdditionalRtpPolicies();
  NalUnitType eType = getNalUnitType(mediaSample);
  // first check if there is a policy for the NAL type
  for (const PolicyDescriptor& desc: policies)
  {
    if (desc.Layer == LAYER_TYPE_NALU)
    {
      if (eType == desc.NaluType)
      {
        // check if the descriptor is for all types, or only specific ones
        if (areLTSet(desc))
        {
          // Must match on DQT
          // get DQT of media sample if possible
          int32_t iL = 0, iT = 0;
          determineShvcLayer(mediaSample, iL, iT);
          if (areLTSet(iL, iT))
          {
            if (desc.D_L_ID == iL &&
                desc.Q_T_ID == iT)
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

  int32_t iL = 0, iT = 0;
  int32_t iLayer = determineShvcLayer(mediaSample, iL, iT);
  for (const PolicyDescriptor& desc: policies)
  {
    if (iL != -1 && iT != -1)
    {
      if (desc.D_L_ID == iL &&
          desc.Q_T_ID == iT)
      {
        return desc.Channel_Id;
      }
    }
    else
    {
      // just match on layer
      if (desc.Layer == iLayer)
      {
        return desc.Channel_Id;
      }
    }
  }
  // else use default RTP policy
  VLOG(2) << "No rule found for NALU: "
          << eType << " Layer:" << iLayer
          << " LT: " << iL << ":" << iT;

  return m_sctpPolicy.getRtpPolicy().Channel_Id;
}

int32_t SctpH265RtpPolicyManager::determineHevcFrameType(const MediaSample& mediaSample)
{
  using namespace media::h265;
  NalUnitType eType = getNalUnitType(mediaSample);
  VLOG(15) << "NAL Unit Type: " << eType;
  switch (eType)
  {
    case NUT_CRA:
    case NUT_VPS:
    case NUT_SPS:
    case NUT_PPS:
    case NUT_IDR_W_RADL:
    case NUT_IDR_N_LP:
    {
      return sctp::FRAME_TYPE_I;
      break;
    }
    default:
    {
      return sctp::FRAME_TYPE_P;
      break;
    }
  }
}

int32_t SctpH265RtpPolicyManager::determineHevcAccessUnitImportance(const std::vector<MediaSample>& vMediaSamples)
{
  // start with least importance
  int iMaxFrameImportance = sctp::FRAME_TYPE_B;
  for (size_t i = 0; i < vMediaSamples.size(); ++i)
  {
    int32_t iFrameImportance = determineHevcFrameType(vMediaSamples[i]);
    VLOG(2) << "NAL Unit Type: " << media::h265::getNalUnitType(vMediaSamples[i]) << " Frame type: " << iFrameImportance;
    if (iFrameImportance < iMaxFrameImportance)
    {
      iMaxFrameImportance = iFrameImportance;
    }
  }
  return iMaxFrameImportance;
}

int32_t SctpH265RtpPolicyManager::determineShvcLayer(const MediaSample& mediaSample, int32_t& iL, int32_t& iT)
{
  using namespace media::h265;
  //NalUnitType eType = getNalUnitType(mediaSample);
  iL = 0;
  iT = 0;
  const uint8_t* data = mediaSample.getDataBuffer().data();
  iL |= (( data[0]&0x01 ) << 5) + (( data[1]&0xF8 ) >> 3);
  iT |= ( data[1]&0x07 );
  return (iL == 0) ? LAYER_TYPE_BL : LAYER_TYPE_EL;
}

}
}

