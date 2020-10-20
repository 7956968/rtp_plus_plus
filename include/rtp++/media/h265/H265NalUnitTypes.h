#pragma once

#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{
namespace media
{
namespace h265
{

/**
  * Enumeration for various types of H.265 NAL unit types
  */
enum NalUnitType
{
  NUT_TRAIL_N                                   = 0,
  NUT_TRAIL_R                                   = 1,
  NUT_TSA_N                                     = 2,
  NUT_TSA_R                                     = 3,
  NUT_STSA_N                                    = 4,
  NUT_STSA_R                                    = 5,
  NUT_RADL_N                                    = 6,
  NUT_RADL_R                                    = 7,
  NUT_RASL_N                                    = 8,
  NUT_RASL_R                                    = 9,
  NUT_RSV_VCL_N10                               = 10,
  NUT_RSV_VCL_R11                               = 11,
  NUT_RSV_VCL_N12                               = 12,
  NUT_RSV_VCL_R13                               = 13,
  NUT_RSV_VCL_N14                               = 14,
  NUT_RSV_VCL_R15                               = 15,
  NUT_BLA_W_LP                                  = 16,
  NUT_BLA_W_RADL                                = 17,
  NUT_BLA_N_LD                                  = 18,
  NUT_IDR_W_RADL                                = 19,
  NUT_IDR_N_LP                                  = 20,
  NUT_CRA                                       = 21,
  NUT_RSV_IRAP_VCL22                            = 22,
  NUT_RSV_IRAP_VCL23                            = 23,
  NUT_RSV_VCL24                                 = 24,
  NUT_RSV_VCL25                                 = 25,
  NUT_RSV_VCL26                                 = 26,
  NUT_RSV_VCL27                                 = 27,
  NUT_RSV_VCL28                                 = 28,
  NUT_RSV_VCL29                                 = 29,
  NUT_RSV_VCL30                                 = 30,
  NUT_RSV_VCL31                                 = 31,
  NUT_VPS                                       = 32,
  NUT_SPS                                       = 33,
  NUT_PPS                                       = 34,
  NUT_AUD                                       = 35,
  NUT_EOS                                       = 36,
  NUT_EOB                                       = 37,
  NUT_FD                                        = 38,
  NUT_PREFIX_SEI                                = 39,
  NUT_SUFFIX_SEI                                = 40,
  NUT_RSV_47                                    = 47,
  NUT_AP                                        = 48,
  NUT_FU                                        = 49
};

/**
  * Utility method to extract NAL unit type from media sample
  */
static NalUnitType getNalUnitType(const MediaSample& mediaSample)
{
    const uint8_t* pData = mediaSample.getDataBuffer().data();
    uint8_t uiNut = ( pData[0] & 0x7E ) >> 1;
    return static_cast<NalUnitType>(uiNut);
}

/**
  * Utility method to extract NAL unit type from uint8_t
  */
static NalUnitType getNalUnitType(uint16_t uiNalUnitHeader)
{
    uint8_t uiNut = ( uiNalUnitHeader & 0x7E00 ) >> 9;
    return static_cast<NalUnitType>(uiNut);
}

/**
  * Utility method to extract NAL unit type from uint8_t*
  * This method expects a pointer to a NAL unit, that start with
  * a start code
  */
static bool getNalUnitType(const uint8_t* pNalUnitHeader, NalUnitType& eType, uint32_t& uiStartCodeLen)
{
  int8_t start_code_3[3] = {0, 0, 1};
  int8_t start_code_4[4] = {0, 0, 0, 1};
  if (memcmp((char*)pNalUnitHeader, start_code_4, 4) != 0)
  {
    if (memcmp((char*)pNalUnitHeader, start_code_3, 3) != 0 )
    {
      return false;
    }
    else
    {
      uiStartCodeLen = 3;
      eType = static_cast<NalUnitType>((pNalUnitHeader[3] >> 1) & 0x3F);
      return true;
    }
  }
  else
  {
    uiStartCodeLen = 4;
    eType = static_cast<NalUnitType>((pNalUnitHeader[4] >> 1) & 0x3F);
    return true;
  }
}

/**
 * @brief getNalUnitInfo returns all the info about the H265 Nal unit
 * @param pNalUnitHeader Pointer to NAL unit *without* start code
 * @param eType
 * @param uiLayerId
 * @param uiTemporalId
 * @param bFirstCtb
 */
static void getNalUnitInfo(const uint8_t* pNalUnitHeader, NalUnitType& eType,
                           uint32_t& uiLayerId, uint32_t& uiTemporalId, bool& bFirstCTB)
{
  eType = static_cast<media::h265::NalUnitType>( (pNalUnitHeader[0] & 0x7E ) >> 1);
  uiLayerId = static_cast<uint32_t>((pNalUnitHeader[0] & 0x01 ) << 5) + ((pNalUnitHeader[1] & 0xF8 ) >> 3);
  uiTemporalId = static_cast<uint32_t>(pNalUnitHeader[1]&0x07);
  bFirstCTB = (( pNalUnitHeader[2] & 0x80 ) != 0) &&
      (eType != media::h265::NUT_VPS) &&
      (eType != media::h265::NUT_SPS) &&
      (eType != media::h265::NUT_PPS) &&
      (eType != media::h265::NUT_PREFIX_SEI) &&
      (eType != media::h265::NUT_SUFFIX_SEI) &&
      (eType != media::h265::NUT_FD);
}

/**
  * Utility method to extract NAL unit type, layer and temporal ID from media sample
  */
static void getNalUnitInfo(const MediaSample& mediaSample, NalUnitType& eType,
                           uint32_t& uiLayerId, uint32_t& uiTemporalId)
{
  eType = getNalUnitType(mediaSample);
  uint8_t layerId = 0;
  uint8_t temporalId = 0;
  const uint8_t* data = mediaSample.getDataBuffer().data();
  layerId = (( data[0]&0x01 ) << 5) + (( data[1]&0xF8 ) >> 3);
  temporalId = ( data[1]&0x07 );
  uiLayerId = static_cast<uint32_t>(layerId);
  uiTemporalId = static_cast<uint32_t>(temporalId);
}

/**
  * Utility method to return string representation of NAL unit type
  */
inline std::string toString(NalUnitType eType)
{
  switch (eType)
  {
    case NUT_TRAIL_N:
    {
      return "NUT_TRAIL_N";
      break;
    }
    case NUT_TRAIL_R:
    {
      return "NUT_TRAIL_R";
      break;
    }
    case NUT_TSA_N:
    {
      return "NUT_TSA_N";
      break;
    }
    case NUT_TSA_R:
    {
      return "NUT_TSA_R";
      break;
    }
    case NUT_STSA_N:
    {
      return "NUT_STSA_N";
      break;
    }
    case NUT_STSA_R:
    {
      return "NUT_STSA_R";
      break;
    }
    case NUT_RADL_N:
    {
      return "NUT_RADL_N";
      break;
    }
    case NUT_RADL_R:
    {
      return "NUT_RADL_R";
      break;
    }
    case NUT_RASL_N:
    {
      return "NUT_RASL_N";
      break;
    }
    case NUT_RASL_R:
    {
      return "NUT_RASL_R";
      break;
    }
    case NUT_RSV_VCL_N10:
    {
      return "NUT_RSV_VCL_N10";
      break;
    }
    case NUT_RSV_VCL_R11:
    {
      return "NUT_RSV_VCL_R11";
      break;
    }
    case NUT_RSV_VCL_N12:
    {
      return "NUT_RSV_VCL_N12";
      break;
    }
    case NUT_RSV_VCL_R13:
    {
      return "NUT_RSV_VCL_R13";
      break;
    }
    case NUT_RSV_VCL_N14:
    {
      return "NUT_RSV_VCL_N14";
      break;
    }
    case NUT_RSV_VCL_R15:
    {
      return "NUT_RSV_VLC_R15";
      break;
    }
    case NUT_BLA_W_LP:
    {
      return "NUT_BLA_W_LP";
      break;
    }
    case NUT_BLA_W_RADL:
    {
      return "NUT_BLA_W_RADL";
      break;
    }
    case NUT_BLA_N_LD:
    {
      return "NUT_BLA_N_LD";
      break;
    }
    case NUT_IDR_W_RADL:
    {
      return "NUT_IDR_W_RADL";
      break;
    }
    case NUT_IDR_N_LP:
    {
      return "NUT_IDR_N_LP";
      break;
    }
    case NUT_CRA:
    {
      return "NUT_CRA";
      break;
    }
    case NUT_RSV_IRAP_VCL22:
    {
      return "NUT_RSV_IRAP_VCL22";
      break;
    }
    case NUT_RSV_IRAP_VCL23:
    {
      return "NUT_RSV_IRAP_VCL23";
      break;
    }
    case NUT_RSV_VCL24:
    {
      return "NUT_RSV_VCL24";
      break;
    }
    case NUT_RSV_VCL25:
    {
      return "NUT_RSV_VCL25";
      break;
    }
    case NUT_RSV_VCL26:
    {
      return "NUT_RSV_VCL26";
      break;
    }
    case NUT_RSV_VCL27:
    {
      return "NUT_RSV_VCL27";
      break;
    }
    case NUT_RSV_VCL28:
    {
      return "NUT_RSV_VCL28";
      break;
    }
    case NUT_RSV_VCL29:
    {
      return "NUT_RSV_VCL29";
      break;
    }
    case NUT_RSV_VCL30:
    {
      return "NUT_RSV_VCL30";
      break;
    }
    case NUT_RSV_VCL31:
    {
      return "NUT_RSV_VCL31";
      break;
    }
    case NUT_VPS:
    {
      return "NUT_VPS";
      break;
    }
    case NUT_SPS:
    {
      return "NUT_SPS";
      break;
    }
    case NUT_PPS:
    {
      return "NUT_PPS";
      break;
    }
    case NUT_AUD:
    {
      return "NUT_AUD";
      break;
    }
    case NUT_EOS:
    {
       return "NUT_EOS";
       break;
    }
    case NUT_EOB:
    {
      return "NUT_EOB";
      break;
    }
    case NUT_FD:
    {
      return "NUT_FD";
      break;
    }
    case NUT_PREFIX_SEI:
    {
      return "NUT_PREFIX_SEI";
      break;
    }
    case NUT_SUFFIX_SEI:
    {
      return "NUT_SUFFIX_SEI";
      break;
    }
    case NUT_AP:
    {
      return "NUT_AP";
      break;
    }
    case NUT_FU:
    {
      return "NUT_FU";
      break;
    }
    default:
    {
      return "Invalid NAL";
    }
  }
}

}
}
}
