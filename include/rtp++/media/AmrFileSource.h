#pragma once
#include <string>
#include <media/MediaStreamParser.h>

namespace rtp_plus_plus
{
namespace media
{

class AmrFileSource : public MediaStreamParser
{
  AmrFileSource(const std::string& sFile)
    :m_sFileName(sFile)
  {
    // read AMR header
    const char amrHeader[] = { 0x23, 0x21, 0x41, 0x4D, 0x52, 0x0A };

  }

  virtual std::vector<MediaSample> flush()
  {

  }

  virtual boost::optional<MediaSample> extract(const uint8_t* pBuffer, uint32_t uiBufferSize,
                                               int32_t& sampleSize, bool bMoreDataWaiting)
  {
  }

  virtual std::vector<MediaSample> extractAll(const uint8_t* pBuffer, uint32_t uiBufferSize,
                                              double dCurrentAccessUnitPts)
  {

  }

private:
  std::string m_sFileName;

};

} // media
} // rtp_plus_plus
