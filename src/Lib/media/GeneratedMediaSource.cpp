#include "CorePch.h"
#include <rtp++/media/GeneratedMediaSource.h>
#include <boost/asio.hpp>
#include <rtp++/RtpTime.h>
#include <rtp++/application/ApplicationUtil.h>

namespace rtp_plus_plus
{
namespace media
{

GeneratedMediaSource::GeneratedMediaSource(PayloadType ePayloadType, const std::string& sPacketSizes, uint8_t uiCharPayload, uint32_t uiTotalVideoSamples)
  : m_ePayloadType(ePayloadType),
  m_vPacketSizes(app::ApplicationUtil::parseMediaSampleSizeString(sPacketSizes)),
  m_uiCharPayload(uiCharPayload),
  m_uiTotalVideoSamples(uiTotalVideoSamples),
  m_uiOutputCount(0),
  m_uiSizeIndex(0)
{
  srand(static_cast<unsigned int>(std::time(NULL)));
  initialiseMediaSamples();
}

bool GeneratedMediaSource::isGood() const
{
  // no limit set
  if (m_uiTotalVideoSamples == 0) return true;
  else return m_uiOutputCount < m_uiTotalVideoSamples;
}

boost::optional<MediaSample> GeneratedMediaSource::getNextMediaSample()
{
  if (!isGood()) return boost::optional<MediaSample>();

  m_mediaSample = m_vSamples[m_uiSizeIndex];
  m_uiSizeIndex = (m_uiSizeIndex + 1) % m_vSamples.size();

  switch (m_ePayloadType)
  {
    case PT_NTP:
    {
      // add NTP timestamp into payload
      // this requires at least 12 bytes = 'NTP:'(4 bytes) + NTP TS (8 bytes)
      if (m_mediaSample.getPayloadSize() >= 12)
      {
        uint32_t uiMsw = 0, uiLsw = 0;
        RtpTime::getNTPTimeStamp(uiMsw, uiLsw);
        uint32_t uiMswH = htonl(uiMsw);
        uint32_t uiLswH = htonl(uiLsw);
        uint8_t* pData = const_cast<uint8_t*>(m_mediaSample.getDataBuffer().data());
        memcpy(pData + 4, &uiMswH, sizeof(uiMswH));
        memcpy(pData + 8, &uiLswH, sizeof(uiLswH));
        static unsigned g_uiTotalPacketSent = 0;
        // counter for debugging
        if (m_mediaSample.getPayloadSize() >= (12 + sizeof(unsigned)))
        {
          memcpy(pData + 12, &g_uiTotalPacketSent, sizeof(unsigned));
          ++g_uiTotalPacketSent;
        }
      }
      break;
    }
    default:
      break;
  }
  m_uiOutputCount++;
  return boost::optional<MediaSample>(m_mediaSample);
}

std::vector<MediaSample> GeneratedMediaSource::getNextAccessUnit()
{
  std::vector<MediaSample> samples;
  boost::optional<MediaSample> sample = getNextMediaSample();
  if (sample)
    samples.push_back(*sample);
  return samples;
}

void GeneratedMediaSource::initialiseMediaSamples()
{
  for (size_t i = 0; i < m_vPacketSizes.size(); ++i)
  {
    uint32_t uiPacketSize = m_vPacketSizes[i];
    // create buffer
    Buffer buffer(new uint8_t[uiPacketSize], uiPacketSize);
    char* pBuffer = reinterpret_cast<char*>(const_cast<uint8_t*>(buffer.data()));

    switch (m_ePayloadType)
    {
      case PT_CHAR:
      {
        memset(pBuffer, m_uiCharPayload, uiPacketSize);
        break;
      }
      case PT_RAND:
      {
        for (size_t j = 4; j < uiPacketSize; ++j)
        {
          pBuffer[j] = rand() % 256;
        }
        break;
      }
      case PT_NTP:
      {
        // removing randomisation for debugging
        memset(pBuffer, 0, uiPacketSize);
#ifdef RANDOMISE
        // for NTP we also fill with random
        for (size_t j = 0; j < uiPacketSize; ++j)
        {
          pBuffer[j] = rand() % 256;
        }
#endif

        // check that the buffer is actually big enough
        if (uiPacketSize > 12)
        {
          pBuffer[0] = 'n';
          pBuffer[1] = 't';
          pBuffer[2] = 'p';
          pBuffer[3] = ':';
        }

        break;
      }
    }

    MediaSample mediaSample;
    mediaSample.setData(buffer);
    m_vSamples.push_back(mediaSample);
  }

  assert(!m_vSamples.empty());
}

} // media
} // rtp_plus_plus
