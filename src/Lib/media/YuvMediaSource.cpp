#include "CorePch.h"
#include <numeric>
#include <rtp++/media/YuvMediaSource.h>

namespace rtp_plus_plus
{
namespace media
{
const size_t READ_SIZE=20000u;

YuvMediaSource::YuvMediaSource(const std::string& sFilename, const uint32_t uiWidth, const uint32_t uiHeight, bool bLoopSource, uint32_t uiLoopCount, uint32_t uiInitialBufferSize)
  :m_in(sFilename.c_str() , std::ifstream::in | std::ifstream::binary),
    m_rIn(m_in),
    m_eType(MT_YUV_420P),
    m_sFilename(sFilename),
    m_uiWidth(uiWidth),
    m_uiHeight(uiHeight),
    m_bEos(false),
    m_bLoopSource(bLoopSource),
    m_uiLoopCount(uiLoopCount),
    m_uiCurrentLoop(0),
    m_uiYuvFrameSize(uiWidth * uiHeight * 1.5),
    m_uiTotalFrames(0),
    m_uiCurrentFrame(0),
    m_buffer(new uint8_t[m_uiYuvFrameSize], m_uiYuvFrameSize)
{
  checkInputStream();
  parseStream();
}



YuvMediaSource::YuvMediaSource(std::istream& in1, const uint32_t uiWidth, const uint32_t uiHeight, bool bLoopSource, uint32_t uiLoopCount, uint32_t uiInitialBufferSize)
  :m_rIn(in1),
    m_eType(MT_YUV_420P),
    m_uiWidth(uiWidth),
    m_uiHeight(uiHeight),
    m_bEos(false),
    m_bLoopSource(bLoopSource),
    m_uiLoopCount(uiLoopCount),
    m_uiCurrentLoop(0),
    m_uiYuvFrameSize(uiWidth * uiHeight * 1.5),
    m_uiTotalFrames(0),
    m_uiCurrentFrame(0),
    m_buffer(new uint8_t[m_uiYuvFrameSize], m_uiYuvFrameSize)
{
  VLOG(2) << "YUV properties width: " << m_uiWidth
          << " height: " << m_uiHeight
          << " YUV frame size: " << m_uiYuvFrameSize
          << " Total frames: " << m_uiTotalFrames;

  checkInputStream();
  parseStream();
}


bool YuvMediaSource::isGood() const
{
  if (m_bEos) return false;
  return true;
}

boost::optional<MediaSample> YuvMediaSource::getNextMediaSample()
{
  // we only deal in AUs
  return boost::optional<MediaSample>();
}

std::vector<MediaSample> YuvMediaSource::readMediaSample()
{
  std::vector<MediaSample> mediaSamples;

  m_rIn.seekg(m_uiCurrentFrame * m_uiYuvFrameSize, std::ios_base::beg);
  m_rIn.read((char*) m_buffer.data(), m_uiYuvFrameSize);
  size_t count = static_cast<size_t>(m_rIn.gcount());
  assert(count == m_uiYuvFrameSize);
  Buffer mediaData(new uint8_t[m_uiYuvFrameSize], m_uiYuvFrameSize);
  memcpy((char*)mediaData.data(), (char*)(m_buffer.data()), m_uiYuvFrameSize);
  MediaSample mediaSample;
  mediaSample.setData(mediaData);
  mediaSamples.push_back(mediaSample);
  return mediaSamples;
}

std::vector<MediaSample> YuvMediaSource::getNextAccessUnit()
{
  if (m_uiCurrentFrame < m_uiTotalFrames)
  {
    std::vector<MediaSample> vFrame = readMediaSample();
    ++m_uiCurrentFrame;
    return vFrame;
  }
  else
  {
    if (m_bLoopSource)
    {
      if (m_uiLoopCount != 0 && m_uiCurrentLoop < m_uiLoopCount)
      {
        m_uiCurrentFrame = 0;
        ++m_uiCurrentLoop;
        return readMediaSample();
      }
      else if (m_uiLoopCount == 0)
      {
        m_uiCurrentFrame = 0;
        return readMediaSample();
      }
      else
      {
        m_bEos = true;
      }
    }
    else
    {
      m_bEos = true;
    }
  }

  return std::vector<MediaSample>();
}

void YuvMediaSource::checkInputStream()
{
  if (!m_rIn.good())  m_bEos = true;
}

void YuvMediaSource::parseStream()
{
  m_rIn.seekg(0, std::ios_base::end);
  size_t uiTotalFileSize = m_rIn.tellg();
  m_rIn.seekg(0, std::ios_base::beg);
  m_uiTotalFrames = uiTotalFileSize / m_uiYuvFrameSize;
  VLOG(12) << "YUV properties width: " << m_uiWidth
          << " height: " << m_uiHeight
          << " YUV frame size: " << m_uiYuvFrameSize
          << " Total frames: " << m_uiTotalFrames;
}

} // media
} // rtp_plus_plus
