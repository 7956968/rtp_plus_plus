#include "CorePch.h"
#include <numeric>
#include <rtp++/media/NalUnitMediaSource.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>
#include <rtp++/media/h265/H265NalUnitTypes.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/rfchevc/Rfchevc.h>


namespace rtp_plus_plus
{
namespace media
{
const size_t READ_SIZE=20000u;

NalUnitMediaSource::NalUnitMediaSource(const std::string& sFilename, const std::string& sMediaType, bool bLoopSource, uint32_t uiLoopCount, uint32_t uiInitialBufferSize)
  :m_in(sFilename.c_str() , std::ifstream::in | std::ifstream::binary),
    m_rIn(m_in),
    m_bEos(false),
    m_bLoopSource(bLoopSource),
    m_uiLoopCount(uiLoopCount),
    m_uiCurrentLoop(0),
    m_uiCurrentAccessUnit(0),
    m_buffer(new uint8_t[uiInitialBufferSize], uiInitialBufferSize),
    m_temporaryNalUnitBuffer(new uint8_t[1000], 1000),
    m_bFirstH265NalUnit(true)
{
  if (sMediaType == rfc6184::H264 || sMediaType == rfc6190::H264_SVC)
  {
    m_eType = MT_H264;
    VLOG(5) << "H264 Nal unit source";
  }
  else if (sMediaType == rfchevc::H265)
  {
    m_eType = MT_H265;
    VLOG(5) << "H265 Nal unit source";
  }
  else
  {
    LOG(WARNING) << "Unsupported Nal unit source";
    m_bEos = true;
  }
  checkInputStream();
  parseAnnexBStream();
}



NalUnitMediaSource::NalUnitMediaSource(std::istream& in1, const std::string& sMediaType, bool bLoopSource, uint32_t uiLoopCount, uint32_t uiInitialBufferSize)
  :m_rIn(in1),
    m_bEos(false),
    m_bLoopSource(bLoopSource),
    m_uiLoopCount(uiLoopCount),
    m_uiCurrentLoop(0),
    m_uiCurrentAccessUnit(0),
    m_buffer(new uint8_t[uiInitialBufferSize], uiInitialBufferSize)
{
  checkInputStream();
  parseAnnexBStream();
}


bool NalUnitMediaSource::isGood() const
{
  if (m_bEos) return false;
  return true;
}

boost::optional<MediaSample> NalUnitMediaSource::getNextMediaSample()
{
  // we only deal in AUs
  return boost::optional<MediaSample>();
}

std::vector<MediaSample> NalUnitMediaSource::getNextAccessUnit()
{
  if (m_uiCurrentAccessUnit < m_vAccessUnitInfo.size())
  {
    AccessUnitInfo_t& auInfo = m_vAccessUnitInfo[m_uiCurrentAccessUnit];
    std::vector<MediaSample> vAu = readMediaSamples(auInfo);
    // set marker bit
    vAu[vAu.size() - 1].setMarker(true);
    ++m_uiCurrentAccessUnit;
    if (m_bLoopSource)
    {
      if (m_uiCurrentAccessUnit == m_vAccessUnitInfo.size())
      {
        if (m_uiLoopCount != 0 && m_uiCurrentLoop < m_uiLoopCount)
        {
          m_uiCurrentAccessUnit = 0;
          ++m_uiCurrentLoop;
        }
        else if (m_uiLoopCount == 0)
        {
          m_uiCurrentAccessUnit = 0;
        }
        else
        {
          m_bEos = true;
        }
      }
    }
    else
    {
      if (m_uiCurrentAccessUnit == m_vAccessUnitInfo.size())
        m_bEos = true;
    }

    return vAu;
  }
  else
  {
    return std::vector<MediaSample>();
  }
}

void NalUnitMediaSource::checkInputStream()
{
  if (!m_rIn.good())  m_bEos = true;
}

std::vector<MediaSample> NalUnitMediaSource::readMediaSamples(const AccessUnitInfo_t& auInfo)
{
  std::vector<MediaSample> vAu;

  uint32_t uiAuLength = std::accumulate(auInfo.begin(), auInfo.end(), 0, [](int iSum, const NalUnitInfo_t& nalUnitInfo)
  {
    return iSum + std::get<1>(nalUnitInfo) + std::get<3>(nalUnitInfo);
  });

  assert(!auInfo.empty());
  const NalUnitInfo_t& info = auInfo[0];

  // read NAL unit from file
  size_t uiStartCodeIndex = std::get<0>(info);

  // resize buffer if necessary
  if (m_buffer.getSize() < uiAuLength)
  {
    LOG(WARNING) << "Resizing buffer for AU. Size: " << m_buffer.getSize() << " New: " << uiAuLength + 1000;
    m_buffer.setData(new uint8_t[uiAuLength + 1000], uiAuLength + 1000);
  }

  m_rIn.seekg(uiStartCodeIndex, std::ios_base::beg);
  m_rIn.read((char*) m_buffer.data(), uiAuLength);
  size_t count = static_cast<size_t>(m_rIn.gcount());
  assert(count == uiAuLength);

  uint32_t uiNaluOffsetInBuffer = 0;
  for (const NalUnitInfo_t& nalInfo: auInfo)
  {
    uint32_t uiStartCodeLen = std::get<1>(nalInfo);
    uint32_t uiNalLen = std::get<3>(nalInfo);
    Buffer mediaData(new uint8_t[uiNalLen], uiNalLen);

    //NB: skip start code
    memcpy((char*)mediaData.data(), (char*)(m_buffer.data() + uiNaluOffsetInBuffer + uiStartCodeLen), uiNalLen);

    MediaSample mediaSample;
    mediaSample.setData(mediaData);
    // HACK to avoid parsing NAL unit later on
    mediaSample.setStartCodeLengthHint(uiStartCodeLen);
    vAu.push_back(mediaSample);

    uiNaluOffsetInBuffer += uiStartCodeLen + uiNalLen;
  }

  return vAu;
}

void NalUnitMediaSource::parseAnnexBStream()
{
  // only search for 3 byte start code
  char startCode[3] = {0, 0, 1};

  m_rIn.seekg(0, std::ios_base::end);
  size_t uiTotalFileSize = m_rIn.tellg();
  m_rIn.seekg(0, std::ios_base::beg);

  size_t uiPreviouslyProcessedData = 0;
  size_t uiDataFromPreviousRead = 0;
  // NB: index to NAL unit, not start code
  size_t uiPreviousNalUnitIndex = 0;
  
  uint32_t uiMaxSize = 0;
  while (m_rIn.good())
  {
    size_t uiRead = std::min(READ_SIZE, m_buffer.getSize() - uiDataFromPreviousRead);
    m_rIn.read((char*) m_buffer.data() + uiDataFromPreviousRead, uiRead);

    size_t count = static_cast<size_t>(m_rIn.gcount());
    size_t uiTotalDataInBuffer = uiDataFromPreviousRead + count;

    // go through read data and search for start codes
    // end at -3 since there might be a scenario where there
    // is a 4 or a 3 byte start code starting in the last 3 bytes. 
    // In the case of the 4 byte start code, we would only be able 
    // to match this after the next read
    for (size_t i = 0; i < uiTotalDataInBuffer - 3; ++i)
    {
      if (memcmp(&m_buffer[i], startCode, 3) == 0)
      {
        size_t index;
        uint32_t uiStartCodeLen;

        // check if this is a 3 byte or 4 byte start code
        if ( i > 0 && m_buffer[i-1] == 0)
        {
          // map to global offset
          index = i-1 + uiPreviouslyProcessedData;
          uiStartCodeLen = 4;
        }
        else
        {
          // map to global offset
          index = i + uiPreviouslyProcessedData;
          uiStartCodeLen = 3;
        }

        size_t uiNalUnitIndex = index + uiStartCodeLen;

        // update size in previously stored info
        if (!m_vStartCodeInfo.empty())
        {
          size_t uiSize = uiNalUnitIndex - uiPreviousNalUnitIndex - uiStartCodeLen;
          std::get<3>(m_vStartCodeInfo[m_vStartCodeInfo.size() - 1 ]) = uiSize; 
          if (uiSize > uiMaxSize) uiMaxSize = uiSize;
        }
        // store current NAL unit
        m_vStartCodeInfo.push_back(std::make_tuple(index, uiStartCodeLen, uiNalUnitIndex, 0));
        
        uiPreviousNalUnitIndex = uiNalUnitIndex;
      }
    }
    // shift data, but leave last 3 bytes in case there is a start code to be matched
    // this means that the last 3 bytes of the file will not be checked for a start code
    // but that's ok since that wouldn't make sense anyway
    uiPreviouslyProcessedData += (uiTotalDataInBuffer - 3);
    uiDataFromPreviousRead = 3;
    memmove(&m_buffer[0], &m_buffer[uiTotalDataInBuffer-3], 3);
  }

  if (m_vStartCodeInfo.empty())
  {
    m_bEos = true;
    return;
  }

  // update size of final NAL unit
  uint32_t uiSize = uiTotalFileSize - uiPreviousNalUnitIndex;
  std::get<3>(m_vStartCodeInfo[m_vStartCodeInfo.size() - 1 ]) = uiSize; 

  // adjust temp buffer
  if (uiSize > uiMaxSize) uiMaxSize = uiSize; 
  if (uiMaxSize > m_temporaryNalUnitBuffer.getSize())
    m_temporaryNalUnitBuffer.setData(new uint8_t[uiMaxSize], uiMaxSize);

  m_rIn.clear();

  AccessUnitInfo_t vCurrentAccessUnit;
  for (size_t i = 0; i < m_vStartCodeInfo.size(); ++i)
  {
    NalUnitInfo_t& info = m_vStartCodeInfo[i];
    // read NAL unit from file
    size_t uiIndex = std::get<2>(info);
    uint32_t uiSize = std::get<3>(info);
    m_rIn.seekg(uiIndex, std::ios_base::beg);
    m_rIn.read((char*) m_temporaryNalUnitBuffer.data(), uiSize);
    size_t count = static_cast<size_t>(m_rIn.gcount());
    assert(count == uiSize);

    switch (m_eType)
    {
      // At this time H264 AU detection is based on AUDs, and in the case of SVC on NAL unit types only
    case MT_H264:
      {
        using h264::NalUnitType;
        NalUnitType eType = h264::getNalUnitType(m_temporaryNalUnitBuffer[0]);

        if (eType == media::h264::NUT_ACCESS_UNIT_DELIMITER ||
            (!m_bCurrentLayerIsBaseLayer && (eType != media::h264::NUT_CODED_SLICE_EXT && eType != media::h264::NUT_RESERVED_21) ) )
        {
          finaliseAu(vCurrentAccessUnit);
          m_bCurrentLayerIsBaseLayer = true;
        }
        else if(eType == media::h264::NUT_CODED_SLICE_EXT || eType == media::h264::NUT_RESERVED_21)
        {
          m_bCurrentLayerIsBaseLayer = false;
        }

        // add AU to current AU
        vCurrentAccessUnit.push_back(info);
        break;
      }
    case MT_H265:
      {
        using h265::NalUnitType;
        NalUnitType eType;
        uint32_t uiLayerId = 0, uiTemporalId = 0;
        bool bFirstCTB = false;
        h265::getNalUnitInfo(&m_temporaryNalUnitBuffer[0], eType, uiLayerId, uiTemporalId, bFirstCTB);
#if 0
        VLOG(5) << "DBG: NALU Type: " << media::h265::toString(eType);
#endif
        if (eType == media::h265::NUT_AUD  ||
            m_bFirstH265NalUnit ||
            (bFirstCTB && uiLayerId == 0 &&
            (m_ePrevH265Type != media::h265::NUT_PPS &&
             m_ePrevH265Type != media::h265::NUT_SPS &&
             m_ePrevH265Type != media::h265::NUT_VPS))
           )
        {
          VLOG(5) << "Finalising AU NALU Type: " << media::h265::toString(eType)
                  << " first: " << m_bFirstH265NalUnit
                  << " first CTB: " << bFirstCTB
                  << " uiLayerId: " << uiLayerId
                  << " previous type: " << media::h265::toString(m_ePrevH265Type);

          finaliseAu(vCurrentAccessUnit);
        }
        else
        {
          VLOG(5) << "Not finalising AU NALU Type: " << media::h265::toString(eType)
                  << " first: " << m_bFirstH265NalUnit
                  << " first CTB: " << bFirstCTB
                  << " uiLayerId: " << uiLayerId
                  << " previous type: " << media::h265::toString(m_ePrevH265Type);
        }
        // add AU to current AU
        vCurrentAccessUnit.push_back(info);
        // store for next pass
        m_bFirstH265NalUnit = false;
        m_ePrevH265Type = eType;
        break;
      }
    }
  }

  // store final AU
  finaliseAu(vCurrentAccessUnit);
}

void NalUnitMediaSource::finaliseAu(AccessUnitInfo_t& vCurrentAccessUnit)
{
  // new AU: store previously seen AUs
  if (!vCurrentAccessUnit.empty()) // the first will be empty
  {
    m_vAccessUnitInfo.push_back(vCurrentAccessUnit);
  }
  vCurrentAccessUnit.clear();
}

} // media
} // rtp_plus_plus
