#pragma once

#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{
namespace media
{
namespace h264
{

/**
 * @brief Enumeration for various types of H.264 NAL unit types
 */
enum NalUnitType
{
  NUT_UNSPECIFIED                               = 0,
  NUT_CODED_SLICE_OF_A_NON_IDR_PICTURE          = 1,
  NUT_CODED_SLICE_DATA_PARTITION_A              = 2,
  NUT_CODED_SLICE_DATA_PARTITION_B              = 3,
  NUT_CODED_SLICE_DATA_PARTITION_C              = 4,
  NUT_CODED_SLICE_OF_AN_IDR_PICTURE             = 5,
  NUT_SUPPLEMENTAL_ENHANCEMENT_INFORMATION_SEI  = 6,
  NUT_SEQUENCE_PARAMETER_SET                    = 7,
  NUT_PICTURE_PARAMETER_SET                     = 8,
  NUT_ACCESS_UNIT_DELIMITER                     = 9,
  NUT_END_OF_SEQUENCE                           = 10,
  NUT_END_OF_STREAM                             = 11,
  NUT_FILLER_DATA                               = 12,
  NUT_SEQUENCE_PARAMETER_SET_EXTENSION          = 13,
  NUT_PREFIX_NAL_UNIT                           = 14,
  NUT_SUBSET_SEQUENCE_PARAMETER_SET             = 15,
  NUT_RESERVED_16                               = 16,
  NUT_RESERVED_17                               = 17,
  NUT_RESERVED_18                               = 18,
  NUT_CODED_SLICE_OF_AN_AUXILIARY_CODED_PICTURE_WITHOUT_PARTITIONING = 19,
  NUT_CODED_SLICE_EXT                           = 20,
  NUT_RESERVED_21                               = 21,
  NUT_RESERVED_22                               = 22,
  NUT_RESERVED_23                               = 23,
  NUT_STAP_A                                    = 24,
  NUT_STAP_B                                    = 25,
  NUT_MTAP16                                    = 26,
  NUT_MTAP24                                    = 27,
  NUT_FU_A                                      = 28,
  NUT_FU_B                                      = 29
};

/**
 * @brief Utility method to extract NAL unit type from media sample
 */
static NalUnitType getNalUnitType(const media::MediaSample& mediaSample, uint32_t startCodeLength = 0)
{
  if (!mediaSample.doesNaluContainsStartCode())
  {
    const uint8_t* pData = mediaSample.getDataBuffer().data();
    uint8_t uiNut = pData[0] & 0x1F;
    return static_cast<NalUnitType>(uiNut);
  }
  else
  {
    uint8_t startCode[3] = { 0, 0, 1 };
    const uint8_t* pData = mediaSample.getDataBuffer().data();
    if (memcmp(startCode, pData, 3) == 0)
    {
      uint8_t uiNut = pData[3] & 0x1F;
      return static_cast<NalUnitType>(uiNut);
    }
    else if(memcmp(startCode, pData + 1, 3) == 0)
    {
      uint8_t uiNut = pData[4] & 0x1F;
      return static_cast<NalUnitType>(uiNut);
    }
    else
    {
      assert(false);
      // make compiler happy
      return NUT_UNSPECIFIED;
    }
  }
}

/**
 * @brief Utility method to extract NAL unit type from uint8_t
 */
static NalUnitType getNalUnitType(uint8_t uiNalUnitHeader)
{
    uint8_t uiNut = uiNalUnitHeader & 0x1F;
    return static_cast<NalUnitType>(uiNut);
}

/**
 * @brief Utility method to return string representation of NAL unit type
 */
static std::string toString(NalUnitType eType)
{
  switch (eType)
  {
    case NUT_UNSPECIFIED:
    {
      return "NUT_UNSPECIFIED";
      break;
    }
    case NUT_CODED_SLICE_OF_A_NON_IDR_PICTURE:
    {
      return "NUT_CODED_SLICE_OF_A_NON_IDR_PICTURE";
      break;
    }
    case NUT_CODED_SLICE_DATA_PARTITION_A:
    {
      return "NUT_CODED_SLICE_DATA_PARTITION_A";
      break;
    }
    case NUT_CODED_SLICE_DATA_PARTITION_B:
    {
      return "NUT_CODED_SLICE_DATA_PARTITION_B";
      break;
    }
    case NUT_CODED_SLICE_DATA_PARTITION_C:
    {
      return "NUT_CODED_SLICE_DATA_PARTITION_C";
      break;
    }
    case NUT_CODED_SLICE_OF_AN_IDR_PICTURE:
    {
      return "NUT_CODED_SLICE_OF_AN_IDR_PICTURE";
      break;
    }
    case NUT_SUPPLEMENTAL_ENHANCEMENT_INFORMATION_SEI:
    {
      return "NUT_SUPPLEMENTAL_ENHANCEMENT_INFORMATION_SEI";
      break;
    }
    case NUT_SEQUENCE_PARAMETER_SET:
    {
      return "NUT_SEQUENCE_PARAMETER_SET";
      break;
    }
    case NUT_PICTURE_PARAMETER_SET:
    {
      return "NUT_PICTURE_PARAMETER_SET";
      break;
    }
    case NUT_ACCESS_UNIT_DELIMITER:
    {
      return "NUT_ACCESS_UNIT_DELIMITER";
      break;
    }
    case NUT_END_OF_SEQUENCE:
    {
      return "NUT_END_OF_SEQUENCE";
      break;
    }
    case NUT_END_OF_STREAM:
    {
      return "NUT_END_OF_STREAM";
      break;
    }
    case NUT_FILLER_DATA:
    {
      return "NUT_FILLER_DATA";
      break;
    }
    case NUT_SEQUENCE_PARAMETER_SET_EXTENSION:
    {
      return "NUT_SEQUENCE_PARAMETER_SET_EXTENSION";
      break;
    }
    case NUT_PREFIX_NAL_UNIT:
    {
      return "NUT_PREFIX_NAL_UNIT";
      break;
    }
    case NUT_SUBSET_SEQUENCE_PARAMETER_SET:
    {
      return "NUT_SUBSET_SEQUENCE_PARAMETER_SET";
      break;
    }
    case NUT_RESERVED_16:
    {
      return "NUT_RESERVED_16";
      break;
    }
    case NUT_RESERVED_17:
    {
      return "NUT_RESERVED_17";
      break;
    }
    case NUT_RESERVED_18:
    {
      return "NUT_RESERVED_18";
      break;
    }
    case NUT_CODED_SLICE_OF_AN_AUXILIARY_CODED_PICTURE_WITHOUT_PARTITIONING:
    {
      return "NUT_CODED_SLICE_OF_AN_AUXILIARY_CODED_PICTURE_WITHOUT_PARTITIONING";
      break;
    }
    case NUT_CODED_SLICE_EXT:
    {
      return "NUT_CODED_SLICE_EXT";
      break;
    }
    case NUT_RESERVED_21:
    {
      return "NUT_RESERVED_21";
      break;
    }
    case NUT_RESERVED_22:
    {
      return "NUT_RESERVED_22";
      break;
    }
    case NUT_RESERVED_23:
    {
      return "NUT_RESERVED_23";
      break;
    }
    case NUT_STAP_A:
    {
      return "NUT_STAP_A";
      break;
    }
    case NUT_STAP_B:
    {
      return "NUT_STAP_B";
      break;
    }
    case NUT_MTAP16:
    {
      return "NUT_MTAP16";
      break;
    }
    case NUT_MTAP24:
    {
      return "NUT_MTAP24";
      break;
    }
    case NUT_FU_A:
    {
      return "NUT_FU_A";
      break;
    }
    case NUT_FU_B:
    {
      return "NUT_FU_B";
      break;
    }
    default:
    {
      return "Invalid NAL";
    }
  }
}

} // h264
} // media
} // rtp_plus_plus
