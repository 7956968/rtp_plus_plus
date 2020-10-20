#include "CorePch.h"
#include <rtp++/media/h264/H264AnnexBStreamParser.h>

#include <rtp++/media/MediaStreamParser.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>

namespace rtp_plus_plus
{
namespace media
{
namespace h264
{

H264AnnexBStreamParser::H264AnnexBStreamParser()
  :m_eMode(TM_FRAME_RATE),
    m_uiFramerate(25),
    m_dFrameDuration(1.0/m_uiFramerate),
    m_bNewAccessUnit(true),
    m_dCurrentAccessUnitPts(0.0),
    m_bCurrentLayerIsBaseLayer(true),
    m_uiAccessUnitCount(0)
{
}

H264AnnexBStreamParser::H264AnnexBStreamParser(uint32_t uiFramerate)
  :m_eMode(TM_FRAME_RATE),
    m_uiFramerate(uiFramerate > 0 ? uiFramerate : 25),
    m_dFrameDuration(1.0/m_uiFramerate),
    m_bNewAccessUnit(true),
    m_dCurrentAccessUnitPts(0.0),
    m_bCurrentLayerIsBaseLayer(true),
    m_uiAccessUnitCount(0)
{
}

void H264AnnexBStreamParser::setMode(TimingMode eMode)
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

void H264AnnexBStreamParser::setFramerate(uint32_t uiFramerate)
{
  m_uiFramerate = uiFramerate;
  m_dFrameDuration = 1.0/m_uiFramerate;
}

std::vector<MediaSample> H264AnnexBStreamParser::flush()
{
  std::vector<MediaSample> vBufferedSamples;
  if (m_pPreviousSample)
    vBufferedSamples.push_back(*m_pPreviousSample);
  return vBufferedSamples;
}

boost::optional<MediaSample> H264AnnexBStreamParser::extract(const uint8_t* pBuffer,
                                                             uint32_t uiBufferSize,
                                                             int32_t& sampleSize,
                                                             bool bMoreDataWaiting)
{
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

  int8_t start_code_3[3] = {0, 0, 1};
  int8_t start_code_4[4] = {0, 0, 0, 1};
  if (memcmp((char*)pBuffer, start_code_4, 4) != 0)
  {
    if (memcmp((char*)pBuffer, start_code_3, 3) != 0 )
    {
      // Doesn't start with start code, exit
      LOG(WARNING) << "Failed to find start code";
      sampleSize = -1;
      return boost::optional<MediaSample>();
    }
    else
    {
      VLOG(10) << "Current NAL type: " << media::h264::toString(media::h264::getNalUnitType(pBuffer[3]));
    }
  }
  else
  {
    VLOG(10) << "Current NAL type: " << media::h264::toString(media::h264::getNalUnitType(pBuffer[4]));
  }

  // This parser has the additional restriction that our stream HAS to have AUDs
  // for the timestamping and marker bit to be set correctly
  uint32_t uiIndex = 0;
  uint8_t uiNut = 0;
  // length of next start code
  uint32_t uiNextStartCodeLen = 0;
  // HACK: start searching for a start code from the 3rd position onwards to skip the first start code
  if (searchForNextNalStartCodeAndNalUnitType(pBuffer, uiBufferSize, 3, uiIndex, uiNut, uiNextStartCodeLen))
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

    media::h264::NalUnitType eNextType = static_cast<media::h264::NalUnitType>(uiNut);
    VLOG(10) << "Next NAL type: " << media::h264::toString(eNextType);

    if (eNextType == media::h264::NUT_ACCESS_UNIT_DELIMITER || (!m_bCurrentLayerIsBaseLayer && (eNextType != media::h264::NUT_CODED_SLICE_EXT && eNextType != media::h264::NUT_RESERVED_21) ) )
    {
      m_bCurrentLayerIsBaseLayer = true;
      m_bNewAccessUnit = true;
      ++m_uiAccessUnitCount;
    }
    else if(eNextType == media::h264::NUT_CODED_SLICE_EXT || eNextType == media::h264::NUT_RESERVED_21)
    {
      m_bCurrentLayerIsBaseLayer = false;
    }
    else
    {
      // set flag for NALUs following the AUD
      m_bNewAccessUnit = false;
    }

    boost::optional<MediaSample> pOutputMediaSample;
    /// output any previously buffered sample
    if (m_pPreviousSample)
    {
//      m_pPreviousSample->setMarker(m_bNewAccessUnit);
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
    // HACK to avoid parsing NAL unit later on
    mediaSample.setStartCodeLengthHint(uiStartCodeLength);

#if 0
    rfc6184::NalUnitType eType = rfc6184::getNalUnitType(mediaData[0]);
    DLOG(INFO) << "Parsed NAL unit - type: " << eType << " Start: " << m_dCurrentAccessUnitPts;
#endif
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

      m_bCurrentLayerIsBaseLayer = true;
      m_bNewAccessUnit = true;
      ++m_uiAccessUnitCount;

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
      // HACK to avoid parsing NAL unit later on
      mediaSample.setStartCodeLengthHint(uiStartCodeLength);

  #if 0
      rfc6184::NalUnitType eType = rfc6184::getNalUnitType(mediaData[0]);
      DLOG(INFO) << "Parsed NAL unit - type: " << eType << " Start: " << m_dCurrentAccessUnitPts;
  #endif
      // store reference to sample for later output
      m_pPreviousSample = boost::optional<MediaSample>(mediaSample);

      // this cast should be ok, since samples should never be that big
      sampleSize = static_cast<int32_t>(uiBufferSize);
      return pOutputMediaSample;

    }
  }
}

std::vector<MediaSample> H264AnnexBStreamParser::extractAll(const uint8_t* pBuffer,
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
      media::h264::NalUnitType eNut = media::h264::getNalUnitType(pBuffer[4]);
      VLOG(10) << "First NAL type: " << media::h264::toString(eNut);
    }
  }
  else
  {
    uiStartCodeLength = 3;
    media::h264::NalUnitType eNut = media::h264::getNalUnitType(pBuffer[3]);
    VLOG(10) << "First NAL type: " << media::h264::toString(eNut);
  }

  // This parser has the additional restriction that our stream HAS to have AUDs
  // for the timestamping and marker bit to be set correctly
  uint32_t uiIndex = 0;
  uint8_t uiNut = 0;
  // previous AU starts at 1st position
  uint32_t uiLastIndex = 0;
  // start searching for a start code from the 3rd position onwards to skip the first start code
  uint32_t uiStartPos = 3;
  // length of next start code
  uint32_t uiNextStartCodeLen = 0;
  while (searchForNextNalStartCodeAndNalUnitType(pBuffer, uiBufferSize, uiStartPos, uiIndex, uiNut, uiNextStartCodeLen))
  {
    // reset access unit flag that was set when the next AUD was spotted
    if (m_bNewAccessUnit)
    {
      m_bNewAccessUnit = false;
    }

    media::h264::NalUnitType eNextType = media::h264::getNalUnitType(uiNut);
    VLOG(10) << "Next NAL type: " << media::h264::toString(eNextType);

    uint32_t uiSize = uiIndex - uiStartCodeLength - uiLastIndex;
    Buffer mediaData(new uint8_t[uiSize], uiSize);
    // skip start code
    memcpy((char*)mediaData.data(), (char*)(pBuffer + uiLastIndex + uiStartCodeLength), uiSize);
    MediaSample mediaSample;
    mediaSample.setData(mediaData);
    mediaSample.setStartTime(m_dCurrentAccessUnitPts);
    // HACK to avoid parsing NAL unit later on
    mediaSample.setStartCodeLengthHint(uiStartCodeLength);

#if 0
    uint8_t uiType = mediaData[0] & 0x1F;
    DLOG(INFO) << "Parsed NAL unit - type: " << static_cast<rfc6184::NalUnitType>(uiType) << " Start: " << m_dCurrentAccessUnitPts;
#endif
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
  // HACK to avoid parsing NAL unit later on
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

bool H264AnnexBStreamParser::searchForNextNalStartCodeAndNalUnitType(const uint8_t* pBuffer, uint32_t uiBufferSize, uint32_t uiStartPos, uint32_t& uiPos, uint8_t& uiNut, uint32_t& uiStartCodeLen)
{
  int8_t start_code_3[3] = {0, 0, 1};
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
        uiNut = pBuffer[i+3] & 0x1F;
        uiStartCodeLen = 4;
      }
      else
      {
        uiPos = i;
        uiNut = pBuffer[i+3] & 0x1F;
        uiStartCodeLen = 3;
      }
      return true;
    }
  }
  return false;
}

}
}
}
