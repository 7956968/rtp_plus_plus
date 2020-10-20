#include "CorePch.h"
#include <rtp++/media/StreamMediaSource.h>
#include <cassert>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/shared_ptr.hpp>
#include <rtp++/media/h264/H264AnnexBStreamParser.h>
#include <rtp++/media/h265/H265AnnexBStreamParser.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/rfchevc/Rfchevc.h>

#define READ_SIZE 1024
#define DEFAULT_FRAME_RATE 30

namespace rtp_plus_plus
{
namespace media
{

StreamMediaSource::StreamMediaSource(std::istream& in1, const std::string& sMediaType,
                                     uint32_t uiInitialBufferSize,
                                     bool bTryLoopSource, uint32_t uiLoopCount,
                                     uint32_t uiFps, bool bLimitOutputToFramerate, Mode eMode)
  :m_in(in1),
    m_sMediaType(sMediaType),
    m_buffer(new uint8_t[uiInitialBufferSize], uiInitialBufferSize),
    m_uiCurrentPos(0),
    m_bStopping(false),
    m_eMode(eMode),
    m_uiTotalSampleCount(0),
    m_bTryLoop(bTryLoopSource),
    m_uiLoopCount(uiLoopCount),
    m_uiCurrentLoop(0),
    m_uiFps(uiFps),
    m_uiFrameDurationUs(m_uiFps != 0 ? 1000000/m_uiFps : 0),
    m_bLimitOutputToFramerate(bLimitOutputToFramerate),
    m_dPreviousSampleTime(-1.0),
    m_dLastSampleTime(1.0),
    m_uiTotalBytes(0),
    m_uiAccessUnitSize(0)
{
  VLOG(2) << "Creating media source: " << m_sMediaType ;
  if (sMediaType == rfc6184::H264 || sMediaType == rfc6190::H264_SVC)
  {
    // make sure framerate is valid
    m_uiFps = (m_uiFps > 0) ? uiFps : DEFAULT_FRAME_RATE;
    m_pMediaStreamParser = std::unique_ptr<h264::H264AnnexBStreamParser>(new h264::H264AnnexBStreamParser(uiFps));
  }else if(sMediaType == rfchevc::H265)
  {
    m_uiFps = (m_uiFps > 0) ? uiFps : DEFAULT_FRAME_RATE;
    m_pMediaStreamParser = std::unique_ptr<h265::H265AnnexBStreamParser>(new h265::H265AnnexBStreamParser(uiFps));
  }
}

StreamMediaSource::~StreamMediaSource()
{
  double dRateKbps = (8 * m_uiTotalBytes)/(m_dLastSampleTime*1000);
  VLOG(2) << LOG_MODIFY_WITH_CARE
            << " SUMMARY Read " << m_uiTotalSampleCount
            << " samples Duration: " << m_dLastSampleTime
            << " Total bytes: " << m_uiTotalBytes
            << " Rate: " << dRateKbps << " kbps";
}

void StreamMediaSource::setReceiveMediaCB(ReceiveMediaCb_t val) { m_receiveMediaCB = val; }

void StreamMediaSource::setReceiveAccessUnitCB(ReceiveAccessUnitCb_t val){ m_accessUnitCB = val; }

boost::system::error_code StreamMediaSource::onStart()
{
  return boost::system::error_code();
}

int StreamMediaSource::extractAndProcessMediaSamples()
{
  int32_t iSize = 0;

  bool bMoreData = m_in.good();
  // NOTE: uiSize determines whether any bytes were consumed and should be moved up
  // iSize can be greater than 0, and pMediaSample can still be null since we always
  // pre-buffer samples until an AUD is detected in the stream
  boost::optional<MediaSample> pMediaSample = m_pMediaStreamParser->extract(m_buffer.data(),
                                                                            m_uiCurrentPos, iSize, bMoreData);
  if (pMediaSample)
  {
    processMediaSample(*pMediaSample);
  }

  while (iSize >= 0)
  {
    if (iSize == 0)
    {
      break;
    }

    // copy remaining data to front of buffer
    memmove(&m_buffer[0], &m_buffer[iSize], m_uiCurrentPos - iSize);
    // update pointer for new reads
    m_uiCurrentPos = m_uiCurrentPos - iSize;
    ++m_uiTotalSampleCount;

    pMediaSample = m_pMediaStreamParser->extract(m_buffer.data(), m_uiCurrentPos, iSize, bMoreData);
    if (pMediaSample)
    {
      processMediaSample(*pMediaSample);
    }
  }
  return iSize;
}

void StreamMediaSource::processMediaSample(const MediaSample& mediaSample)
{
  if (processAccessUnitOnCompletion(mediaSample) && m_bLimitOutputToFramerate)
  {
    boost::asio::deadline_timer t(m_io, boost::posix_time::microseconds(m_uiFrameDurationUs));
    // it's a new access unit
    VLOG(15) << "New sample: " << mediaSample.getStartTime()
            << " Old sample: " << m_dPreviousSampleTime
            << " : waiting for " << m_uiFrameDurationUs << "us";
    t.wait();
  }

  m_uiTotalBytes += mediaSample.getPayloadSize();
  m_uiAccessUnitSize += mediaSample.getPayloadSize();
  m_dLastSampleTime = mediaSample.getStartTime();
  switch (m_eMode)
  {
    case SingleMediaSample:
    {
      m_receiveMediaCB(mediaSample);
      break;
    }
    case AccessUnit:
    {
      m_vAccessUnitBuffer.push_back(mediaSample);
      break;
    }
  }
}

inline void StreamMediaSource::outputAccessUnit()
{
  assert(m_accessUnitCB);
  // set RTP marker bit flag
  if (!m_vAccessUnitBuffer.empty())
  {
    m_vAccessUnitBuffer[m_vAccessUnitBuffer.size() - 1].setMarker(true);
  }
  VLOG(15) << "Outputting access units: " << m_vAccessUnitBuffer.size() << " Size: " << m_uiAccessUnitSize;
  if (!m_vAccessUnitBuffer.empty())
    m_accessUnitCB(m_vAccessUnitBuffer);
  m_vAccessUnitBuffer.clear();
}

bool StreamMediaSource::processAccessUnitOnCompletion(const MediaSample& mediaSample)
{
  if (mediaSample.getStartTime() != m_dPreviousSampleTime)
  {
    switch (m_eMode)
    {
      case SingleMediaSample:
      {
        VLOG_IF(15, m_uiAccessUnitSize > 0) << "Access Unit Size: " << m_uiAccessUnitSize;
        break;
      }
      case AccessUnit:
      {
        outputAccessUnit();
        break;
      }
    }

    m_uiAccessUnitSize = 0;
    m_dPreviousSampleTime = mediaSample.getStartTime();
    return true;
  }
  return false;
}

boost::system::error_code StreamMediaSource::start()
{
  VLOG(2) << "Starting stdin media source";
  m_bStopping = false;

  if (!m_pMediaStreamParser)
  {
    LOG(ERROR) << "Unsupported media type: " << m_sMediaType;
    return boost::system::error_code();
  }

  switch (m_eMode)
  {
    case SingleMediaSample:
    {
      if (!m_receiveMediaCB)
      {
        LOG_FIRST_N(WARNING, 1) << "No callback configured for media samples";
        return boost::system::error_code();
      }
      break;
    }
    case AccessUnit:
    {
      if (!m_accessUnitCB)
      {
        LOG_FIRST_N(WARNING, 1) << "No callback configured for access units";
        return boost::system::error_code();
      }
      break;
    }
  }

  while (!m_bStopping)
  {
    if (!m_in.good())
    {
      if (m_bTryLoop)
      {
        LOG_FIRST_N(INFO, 1) << "In lopping mode: rewinding stream";
        m_in.clear();
        m_in.seekg(0, std::ios_base::beg);
      }
      else
      {
        break;
      }
    }
#if 0
    uint32_t uiRead = m_buffer.read(m_in, READ_SIZE);
#else

    // check if there's enough space in the buffer for the read, else resize it
    if (m_buffer.getSize() - m_uiCurrentPos < READ_SIZE )
    {
      uint32_t uiBufferSize = m_buffer.getSize() * 2;
      uint8_t* pNewBuffer = new uint8_t[uiBufferSize];
      memcpy((char*)pNewBuffer, (char*)m_buffer.data(), m_uiCurrentPos);
      m_buffer.setData(pNewBuffer, uiBufferSize);
    }

    m_in.read((char*) m_buffer.data() + m_uiCurrentPos, READ_SIZE);
    uint32_t uiRead = m_in.gcount();
    m_uiCurrentPos += uiRead;

#endif

    int iSize = extractAndProcessMediaSamples();
    if (iSize == -1)
    {
      if (shouldStop())
      {
        VLOG(2) << "Stopping: EOS returned from media stream parser";
        m_bStopping = true;
        break;
      }
      else
      {
        flushMediaSamplesInBuffers();
        VLOG(2) << "Looping: EOS returned from media stream parser";

        if (m_bLimitOutputToFramerate)
        {
          boost::asio::deadline_timer t(m_io, boost::posix_time::microseconds(m_uiFrameDurationUs));
          VLOG(15) << "Wait in loop for " << m_uiFrameDurationUs << "us";;
          t.wait();
        }
        ++m_uiCurrentLoop;
      }
    }
  }

  flushMediaSamplesInBuffers();
  return boost::system::error_code();
}

bool StreamMediaSource::shouldStop() const
{
  VLOG(2) << "Looping mode: " << m_bTryLoop << " " << m_uiCurrentLoop + 1 << "/" << m_uiLoopCount;
  if (!m_bTryLoop) return true;
  if (m_uiLoopCount != 0)
  {
    return !(m_uiCurrentLoop < m_uiLoopCount - 1);
  }
  else
  {
    // "infinite" looping with m_bTryLoop
    return false;
  }
}

void StreamMediaSource::flushMediaSamplesInBuffers()
{
  // if (m_bStopping)
  {
    // FLUSH buffers
    switch (m_eMode)
    {
      case SingleMediaSample:
      {
        std::vector<MediaSample> vBufferedMediaSamples = m_pMediaStreamParser->flush();
        // flush buffered sample
        if (!vBufferedMediaSamples.empty() && m_receiveMediaCB)
        {
          for (MediaSample mediaSample : vBufferedMediaSamples)
          {
            m_uiAccessUnitSize += mediaSample.getPayloadSize();
            m_uiTotalBytes += m_uiAccessUnitSize;
            m_dLastSampleTime = mediaSample.getStartTime();
            m_receiveMediaCB(mediaSample);
          }
        }
        break;
      }
      case AccessUnit:
      {
        // There possibly are AUs in the AU buffer as well as samples in the parser
        std::vector<MediaSample> vBufferedMediaSamples = m_pMediaStreamParser->flush();
        // process the flushed samples one by one
        for (const auto& mediaSample: vBufferedMediaSamples)
        {
          processMediaSample(mediaSample);
        }
        // finally output leftover media samples as AU
        if (!m_vAccessUnitBuffer.empty()) outputAccessUnit();
        break;
      }
    }
  }
}

boost::system::error_code StreamMediaSource::stop()
{
  m_bStopping = true;
  return boost::system::error_code();
}

boost::system::error_code StreamMediaSource::onComplete()
{
  return boost::system::error_code();
}

}
}
