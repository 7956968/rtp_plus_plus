#pragma once
#include <cstdint>
#include <rtp++/media/MediaSample.h>
#include <rtp++/sctp/SctpRtpPolicy.h>

namespace rtp_plus_plus
{
namespace sctp
{

class SctpH264RtpPolicyManager
{
public:

  SctpH264RtpPolicyManager();

  void setPolicy(const SctpRtpPolicy& sctpPolicy);

  /**
    * @brief Attempts to find the channel using the rules for NALU, then frame type
    * and uses the default rule if no rule was found
    */
  uint32_t getRtpChannelForAvcMedia(const media::MediaSample& mediaSample);
  /**
    * @brief Attempts to find the channel using the "most important" frame type,
    * and uses the default rule if no rule was found. WARNING: this method ignores
    * individual NAL type rules
    */
  uint32_t getRtpChannelForAvcMedia(const std::vector<media::MediaSample>& vMediaSamples);
  /**
    * @brief Attempts to find the channel using the rules for NALU, then layer,
    * and uses the default rule if no rule was found
    */
  uint32_t getRtpChannelForSvcMedia(const media::MediaSample& mediaSample, uint32_t uiAccessUnitNumber);

private:
  /**
   * @brief determineAvcFrameType: determines the importance of the media sample
   * @param mediaSample Media sample which is to be analysed
   * @return FRAME_TYPE_I, FRAME_TYPE_P or FRAME_TYPE_B: frame type policy to be used
  */
  int32_t determineAvcFrameType(const media::MediaSample& mediaSample);

  /**
   * @brief determineAvcAccessUnitImportance Simplification since the vector could contain multiple types
   * For now it suffices to focus on the most important NALU in the vector
   * @param vMediaSamples vector of media samples which are to be analysed
   * @return FRAME_TYPE_I, FRAME_TYPE_P or FRAME_TYPE_B: frame type policy to be used
   */
  int32_t determineAvcAccessUnitImportance(const std::vector<media::MediaSample>& vMediaSamples);

  /**
   * @brief determineSvcLayer Determines whether this is a base or enhancement layer NALU
   * @param mediaSample
   * @param iD if the type == LAYER_TYPE_EL, the D SVC indicator
   * @param iQ if the type == LAYER_TYPE_EL, the Q SVC indicator
   * @param iT if the type == LAYER_TYPE_EL, the T SVC indicator
   * @return LAYER_TYPE_BL or LAYER_TYPE_EL
   */
  int32_t determineSvcLayer(const media::MediaSample& mediaSample, int32_t& iD, int32_t& iQ, int32_t& iT);

private:
  SctpRtpPolicy m_sctpPolicy;

  // HACK
  uint32_t m_uiLastAuIndex;
  uint32_t m_uiLevelIndex;
};

}
}
