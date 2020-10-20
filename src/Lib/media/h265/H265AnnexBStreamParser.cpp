#include "CorePch.h"
#include <rtp++/media/h265/H265AnnexBStreamParser.h>

#include <rtp++/media/MediaStreamParser.h>
#include <rtp++/media/h265/H265NalUnitTypes.h>

namespace rtp_plus_plus
{
namespace media
{
namespace h265
{

H265AnnexBStreamParser::H265AnnexBStreamParser()
  :m_eMode(TM_FRAME_RATE),
    m_uiFramerate(25),
    m_dFrameDuration(1.0/m_uiFramerate),
    m_bNewAccessUnit(true),
    m_dCurrentAccessUnitPts(0.0),
    m_uiNALType(47),
  m_uiAccessUnitCount(0)
{
}

H265AnnexBStreamParser::H265AnnexBStreamParser(uint32_t uiFramerate)
  :m_eMode(TM_FRAME_RATE),
    m_uiFramerate(uiFramerate > 0 ? uiFramerate : 25),
    m_dFrameDuration(1.0/m_uiFramerate),
    m_bNewAccessUnit(true),
    m_dCurrentAccessUnitPts(0.0),
    m_uiNALType(47),
    m_uiAccessUnitCount(0)
{
}

void H265AnnexBStreamParser::setMode(TimingMode eMode)
{
  m_eMode = eMode;
  switch (m_eMode)
  {
    case TM_FRAME_RATE:
    {
      m_dFrameDuration = 1.0/m_uiFramerate;
      break;
    }
    default:
      break;
  }
}

void H265AnnexBStreamParser::setFramerate(uint32_t uiFramerate)
{
  m_uiFramerate = uiFramerate;
  m_dFrameDuration = 1.0/m_uiFramerate;
}

std::vector<MediaSample> H265AnnexBStreamParser::flush()
{
  std::vector<MediaSample> vBufferedSamples;
  if (m_pPreviousSample)
    vBufferedSamples.push_back(*m_pPreviousSample);
  return vBufferedSamples;
}

boost::optional<MediaSample> H265AnnexBStreamParser::extract(const uint8_t* pBuffer,
                                                             uint32_t uiBufferSize,
                                                             int32_t& sampleSize,
                                                             bool bMoreDataWaiting)
{
  static uint16_t ui_DON = 0;
  if (m_tStart.is_not_a_date_time())
    m_tStart = boost::posix_time::microsec_clock::universal_time();

  // early exit
  if (uiBufferSize < 5)
  {
    // signal EOF if there is no more data
    if (!bMoreDataWaiting)
      sampleSize = -1;
    else
      sampleSize = 0;
    if (m_pPreviousSample)
    {
      boost::optional<MediaSample> pOutputMediaSample;
      /// output any previously buffered sample
      if (m_pPreviousSample)
      {
        m_pPreviousSample->setMarker(m_bNewAccessUnit);
        pOutputMediaSample = m_pPreviousSample;
        m_pPreviousSample = boost::none;
      }
      return pOutputMediaSample;
    }
    return boost::optional<MediaSample>();
  }

  NalUnitType eType;
  uint32_t uiStartCodeLen = 0;
  if (!getNalUnitType(pBuffer, eType, uiStartCodeLen))
  {
    // Doesn't start with start code, exit
    LOG(WARNING) << "Failed to get NAL unit type";
    sampleSize = -1;
    return boost::optional<MediaSample>();
  }
  else
  {
    VLOG(10) << "Current NAL type: " << toString(eType);
  }

  // This parser has the additional restriction that our stream HAS to have AUDs
  // for the timestamping and marker bit to be set correctly
  int8_t start_code_3[3] = {0, 0, 1};
  uint32_t uiIndex = 0;
  uint8_t uiNut = 0;
  uint8_t uiLayerId = 0;
  bool bFirstCTB;
  // length of next start code
  uint32_t uiNextStartCodeLen = 0;
  // HACK: start searching for a start code from the 3rd position onwards to skip the first start code
  if (searchForNextNalStartCodeAndNalUnitType(pBuffer, uiBufferSize, 3, uiIndex, uiNut, uiLayerId, bFirstCTB , uiNextStartCodeLen))
  {
    // reset access unit flag that was set when the next AUD was spotted
    if (m_bNewAccessUnit)
    {
      m_bNewAccessUnit = false;
      switch (m_eMode)
      {
        case TM_NONE:
        {
          break;
        }
        case TM_LIVE:
        {
          boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
          boost::posix_time::time_duration diff = tNow - m_tStart;
          m_dCurrentAccessUnitPts = diff.total_seconds() + (diff.fractional_seconds()/1000000.0);
          break;
        }
        case TM_FRAME_RATE:
        {
          m_dCurrentAccessUnitPts = m_uiAccessUnitCount * m_dFrameDuration;
          break;
        }
      }
    }

    NalUnitType eNextType = static_cast<NalUnitType>(uiNut);
    NalUnitType ePrevType = static_cast<NalUnitType>(m_uiNALType);
    VLOG(10) << "Next NAL type: " << toString(eNextType);

    if (eNextType == media::h265::NUT_AUD  ||
        (bFirstCTB && uiLayerId == 0 &&
        (ePrevType != media::h265::NUT_PPS &&
         ePrevType != media::h265::NUT_SPS &&
         ePrevType != media::h265::NUT_VPS))
       )
    {
      m_bNewAccessUnit = true;
      ++m_uiAccessUnitCount;
    }
    /*else if(eNextType == media::h265::NUT_VPS || eNextType == media::h265::NUT_SPS || eNextType == media::h265::NUT_PPS)
    {
        //Not necessary to do anything rigth now since the parameters sets are always in the beginning
    }*/
    else
    {
      // set flag for NALUs following the AUD
      m_bNewAccessUnit = false;
    }
    m_uiNALType = uiNut;
    boost::optional<MediaSample> pOutputMediaSample;
    /// output any previously buffered sample
    if (m_pPreviousSample)
    {
      m_pPreviousSample->setMarker(m_bNewAccessUnit);
      pOutputMediaSample = m_pPreviousSample;
      m_pPreviousSample = boost::none;
    }

    uint32_t uiStartCodeLength = 4;
    if (memcmp((char*)pBuffer, start_code_3, 3) == 0)
      uiStartCodeLength = 3;
    uint32_t uiSize = uiIndex - uiStartCodeLength;
    Buffer mediaData(new uint8_t[uiSize], uiSize);
    // skip start code
    memcpy((char*)mediaData.data(), (char*)(pBuffer + uiStartCodeLength), uiSize);
    MediaSample mediaSample;
    mediaSample.setData(mediaData);
    mediaSample.setStartTime(m_dCurrentAccessUnitPts);
    mediaSample.setDecodingOrderNumber(ui_DON++);
    mediaSample.setStartCodeLengthHint(uiStartCodeLength);

    // store reference to sample for later output
    m_pPreviousSample = boost::optional<MediaSample>(mediaSample);
    // this cast should be ok, since samples should never be that big
    sampleSize = static_cast<int32_t>(uiIndex);
    return pOutputMediaSample;
  }
  else
  {
    if (bMoreDataWaiting)
    {
      // return 0 so that more data can be read from the file or stream
      sampleSize = 0;
      return boost::optional<MediaSample>();
    }
    else
    {
      // the end of the stream has been reached so if we got this far there was a start code
      // and the buffer contains the complete final sample

      // TODO: refactor: copied from above
      boost::optional<MediaSample> pOutputMediaSample;
      /// output any previously buffered sample
      if (m_pPreviousSample)
      {
        m_pPreviousSample->setMarker(m_bNewAccessUnit);
        pOutputMediaSample = m_pPreviousSample;
      }

      uint32_t uiStartCodeLength = 4;
      if (memcmp((char*)pBuffer, start_code_3, 3) == 0)
        uiStartCodeLength = 3;
      uint32_t uiSize = uiBufferSize - uiStartCodeLength;
      Buffer mediaData(new uint8_t[uiSize], uiSize);
      // skip start code
      memcpy((char*)mediaData.data(), (char*)(pBuffer + uiStartCodeLength), uiSize);
      MediaSample mediaSample;
      mediaSample.setData(mediaData);
      mediaSample.setStartTime(m_dCurrentAccessUnitPts);
      mediaSample.setDecodingOrderNumber(ui_DON++);
      mediaSample.setStartCodeLengthHint(uiStartCodeLength);

      // store reference to sample for later output
      m_pPreviousSample = boost::optional<MediaSample>(mediaSample);

      // this cast should be ok, since samples should never be that big
      sampleSize = static_cast<int32_t>(uiBufferSize);
      return pOutputMediaSample;

    }
  }
}

std::vector<MediaSample> H265AnnexBStreamParser::extractAll(const uint8_t* pBuffer,
                                                            uint32_t uiBufferSize,
                                                            double dCurrentAccessUnitPts)
{
  std::vector<MediaSample> vSamples;

  // early exit
  if (uiBufferSize < 5)
  {
    return vSamples;
  }

  uint32_t uiStartCodeLength = 4;
  int8_t start_code_3[3] = {0, 0, 1};
  int8_t start_code_4[4] = {0, 0, 0, 1};
  if (memcmp((char*)pBuffer, start_code_3, 3) != 0 )
  {
    if (memcmp((char*)pBuffer, start_code_4, 4) != 0)
    {
      // Doesn't start with start code, exit
      LOG(WARNING) << "Failed to find start code";
      return vSamples;
    }
    else
    {
      media::h265::NalUnitType eNut = media::h265::getNalUnitType( (pBuffer[4]<<8) + pBuffer[5]);
      VLOG(10) << "First NAL type: " << media::h265::toString(eNut);
    }
  }
  else
  {
    uiStartCodeLength = 3;
    media::h265::NalUnitType eNut = media::h265::getNalUnitType( (pBuffer[3]<<8 ) + pBuffer[4]);
    VLOG(10) << "First NAL type: " << media::h265::toString(eNut);
  }

  // This parser has the additional restriction that our stream HAS to have AUDs
  // for the timestamping and marker bit to be set correctly
  uint32_t uiIndex = 0;
  uint8_t uiNut = 0;
  uint8_t uiLayerId = 0;
  bool bFirstCTB;
  // previous AU starts at 1st position
  uint32_t uiLastIndex = 0;
  // start searching for a start code from the 3rd position onwards to skip the first start code
  uint32_t uiStartPos = 3;
  // length of next start code
  uint32_t uiNextStartCodeLen = 0;
  while (searchForNextNalStartCodeAndNalUnitType(pBuffer, uiBufferSize, uiStartPos, uiIndex, uiNut,uiLayerId,bFirstCTB, uiNextStartCodeLen))
  {
    // reset access unit flag that was set when the next AUD was spotted
    if (m_bNewAccessUnit)
    {
      m_bNewAccessUnit = false;
    }

    media::h265::NalUnitType eNextType = media::h265::getNalUnitType(uiNut);
    VLOG(10) << "Next NAL type: " << media::h265::toString(eNextType);

    uint32_t uiSize = uiIndex - uiStartCodeLength - uiLastIndex;
    Buffer mediaData(new uint8_t[uiSize], uiSize);
    // skip start code
    memcpy((char*)mediaData.data(), (char*)(pBuffer + uiLastIndex + uiStartCodeLength), uiSize);
    MediaSample mediaSample;
    mediaSample.setData(mediaData);
    mediaSample.setStartTime(m_dCurrentAccessUnitPts);
    mediaSample.setStartCodeLengthHint(uiStartCodeLength);

    vSamples.push_back(mediaSample);

    uiLastIndex = uiIndex;
    uiStartPos = uiLastIndex + 3;
    uiStartCodeLength = uiNextStartCodeLen;
  }

  // push_back last or only sample
  uint32_t uiSize = uiBufferSize - uiStartCodeLength - uiLastIndex;
  Buffer mediaData(new uint8_t[uiSize], uiSize);
  // skip start code
  memcpy((char*)mediaData.data(), (char*)(pBuffer + uiLastIndex + uiStartCodeLength), uiSize);
  MediaSample mediaSample;
  mediaSample.setData(mediaData);
  mediaSample.setStartTime(m_dCurrentAccessUnitPts);
  // set marker on last sample
  mediaSample.setMarker(true);
  mediaSample.setStartCodeLengthHint(uiStartCodeLength);
  vSamples.push_back(mediaSample);

  std::ostringstream ostr;
  for(size_t i = 0; i < vSamples.size(); ++i)
  {
    ostr << vSamples[i].getDataBuffer().getSize() << " ";
  }
  VLOG(5) << "Found " << vSamples.size() << " samples in buffer. Sizes: " << ostr.str();
  return vSamples;
}

bool
H265AnnexBStreamParser::searchForNextNalStartCodeAndNalUnitType(const uint8_t* pBuffer,
                                                                uint32_t uiBufferSize,
                                                                uint32_t uiStartPos,
                                                                uint32_t& uiPos,
                                                                uint8_t& uiNut,
                                                                uint8_t& uiLayerId,
                                                                bool & bFirstCTB,
                                                                uint32_t& uiStartCodeLen)
{
  int8_t start_code_3[3] = {0, 0, 1};
  bFirstCTB = false;
  // assume start code at the beginning
  for (size_t i = uiStartPos; i < uiBufferSize - 4; ++i)
  {
    if (memcmp((char*)&pBuffer[i], start_code_3, 3) == 0)
    {
      // found next NAL start code: check if this is a 3 or 4 byte start code
      if (pBuffer[i-1] == 0)
      {
        // 4 byte start code
        uiPos = i - 1;
        uiStartCodeLen = 4;
      }
      else
      {
        uiPos = i;
        uiStartCodeLen = 3;
      }
      uiNut = ( pBuffer[i+3] & 0x7E ) >> 1;
      uiLayerId = ( (pBuffer[i+3] & 0x01 ) << 5) + ((pBuffer[i+4] & 0xF8 ) >> 3);
      NalUnitType eNut = static_cast<media::h265::NalUnitType>(uiNut);
      bFirstCTB = (( pBuffer[i+5] & 0x80 ) !=0)*
          (eNut != media::h265::NUT_VPS)*
          (eNut!=media::h265::NUT_SPS)*
          (eNut!=media::h265::NUT_PPS)*
          (eNut!=media::h265::NUT_PREFIX_SEI)*
          (eNut!=media::h265::NUT_SUFFIX_SEI)*
          (eNut!=media::h265::NUT_FD);
      return true;
    }
  }
  return false;
}

}
}
}
