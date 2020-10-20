#include "CorePch.h"
#include <rtp++/media/AsyncStreamMediaSource.h>
#include <cassert>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
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

AsyncStreamMediaSource::AsyncStreamMediaSource(boost::asio::io_service& ioService,
                                               std::istream& in1, const std::string& sMediaType,
                                               uint32_t uiInitialBufferSize,
                                               bool bTryLoopSource, uint32_t uiLoopCount,
                                               uint32_t uiFps, bool bLimitOutputToFramerate)
  :m_timer(ioService),
    m_in(in1),
    m_sMediaType(sMediaType),
    m_buffer(new uint8_t[uiInitialBufferSize], uiInitialBufferSize),
    m_uiCurrentPos(0),
    m_bStopping(false),
    m_uiTotalSampleCount(0),
    m_bTryLoop(bTryLoopSource),
    m_uiLoopCount(uiLoopCount),
    m_uiCurrentLoop(0),
    m_uiFps(uiFps),
    m_uiFrameDurationUs(m_uiFps != 0 ? 1000000/m_uiFps : 0),
    m_bLimitOutputToFramerate(bLimitOutputToFramerate),
    m_dPreviousSampleTime(-1.0),
    m_dLastSampleTime(-1.0),
    m_uiTotalBytes(0),
    m_uiAccessUnitSize(0)
{
  VLOG(2) << "Creating media source: " << m_sMediaType ;
  if (sMediaType == rfc6184::H264 || sMediaType == rfc6190::H264_SVC)
  {
    // make sure framerate is valid
    m_uiFps = (m_uiFps > 0) ? uiFps : DEFAULT_FRAME_RATE;
    m_pMediaStreamParser = std::unique_ptr<h264::H264AnnexBStreamParser>(new h264::H264AnnexBStreamParser(uiFps));
  }
  else if(sMediaType == rfchevc::H265)
  {
    m_uiFps = (m_uiFps > 0) ? uiFps : DEFAULT_FRAME_RATE;
    m_pMediaStreamParser = std::unique_ptr<h265::H265AnnexBStreamParser>(new h265::H265AnnexBStreamParser(uiFps));
  }
}

AsyncStreamMediaSource::~AsyncStreamMediaSource()
{
  double dRateKbps = (8 * m_uiTotalBytes)/(m_dLastSampleTime*1000);
  VLOG(2) << LOG_MODIFY_WITH_CARE
            << " SUMMARY Read " << m_uiTotalSampleCount
            << " samples Duration: " << m_dLastSampleTime
            << " Total bytes: " << m_uiTotalBytes
            << " Rate: " << dRateKbps << " kbps";
}

void AsyncStreamMediaSource::setReceiveAccessUnitCB(ReceiveAccessUnitCb_t val){ m_accessUnitCB = val; }

void AsyncStreamMediaSource::addToAccessUnit(const MediaSample& mediaSample)
{
  m_uiTotalBytes += mediaSample.getPayloadSize();
  m_uiAccessUnitSize += mediaSample.getPayloadSize();
  m_dLastSampleTime = mediaSample.getStartTime();
  m_dPreviousSampleTime = mediaSample.getStartTime();
  m_vAccessUnitBuffer.push_back(mediaSample);
}

inline void AsyncStreamMediaSource::outputAccessUnit()
{
  assert(m_accessUnitCB);
  // set RTP marker bit flag
  if (!m_vAccessUnitBuffer.empty())
  {
    m_vAccessUnitBuffer[m_vAccessUnitBuffer.size() - 1].setMarker(true);
    VLOG(15) << "Outputting access units: " << m_vAccessUnitBuffer.size() << " Size: " << m_uiAccessUnitSize;
    m_accessUnitCB(m_vAccessUnitBuffer);
    m_vAccessUnitBuffer.clear();
    m_uiAccessUnitSize = 0;
  }
}

bool AsyncStreamMediaSource::belongsToNewAccessUnit(const MediaSample& mediaSample)
{
  return (mediaSample.getStartTime() != m_dPreviousSampleTime);
}

void AsyncStreamMediaSource::readAccessUnit(const boost::system::error_code& ec)
{
  if(ec)
  {
    VLOG(15) << "Stopping source: m_bStopping: " << m_bStopping << " ec: " << ec.message();
  }
  // check stop flag
  if (m_bStopping)
  {
    // flush
    flushMediaSamplesInBuffers();
    VLOG(15) << "Stopping source: m_bStopping: " << m_bStopping << " ec: " << ec.message();
    return;
  }

  bool bFoundAccessUnit = false;
  boost::optional<MediaSample> pMediaSample;

  bool bNeedMoreData = false;
  if (m_uiCurrentPos == 0) bNeedMoreData = true;

  int32_t iSize = 0;
  while (!bFoundAccessUnit)
  {
    // check if we need more data
    if (bNeedMoreData)
    {
      // read some data into the buffer
      uint32_t uiRead = readDataIntoBuffer();
      if (uiRead == 0)
      {
        // EOF
        m_bStopping = true;
        break;
      }
    }

    // NOTE: uiSize determines whether any bytes were consumed and should be moved up
    // uiSize can be greater than 0, and pMediaSample can still be null since we always
    // pre-buffer samples until an AUD is detected in the stream
    pMediaSample = m_pMediaStreamParser->extract(m_buffer.data(), m_uiCurrentPos, iSize, m_in.good());

    if (iSize == -1)
    {
      // error in parser
      m_bStopping = true;
    }
    else if (iSize == 0)
    {
      bNeedMoreData = true;
    }
    else
    {
      // shift up processed data
      // copy remaining data to front of buffer
      memmove(&m_buffer[0], &m_buffer[iSize], m_uiCurrentPos - iSize);
      // update pointer for new reads
      m_uiCurrentPos = m_uiCurrentPos - iSize;
      ++m_uiTotalSampleCount;
      // we might have enough data for now
      bNeedMoreData = false;
    }

    if (pMediaSample)
    {
      if (belongsToNewAccessUnit(*pMediaSample) && m_dPreviousSampleTime != -1)
      {
        bFoundAccessUnit = true;
        break;
      }
      else
      {
        addToAccessUnit(*pMediaSample);
      }
    }
  }

  if (bFoundAccessUnit)
  {
    outputAccessUnit();
    addToAccessUnit(*pMediaSample);
    if (m_bLimitOutputToFramerate)
    {
      boost::posix_time::ptime tNext;
      if (m_tPrevious.is_not_a_date_time())
      {
        m_tPrevious = boost::posix_time::microsec_clock::universal_time();
        tNext = m_tPrevious + boost::posix_time::microseconds(m_uiFrameDurationUs);
      }
      else
      {
        tNext = m_timer.expires_at() + boost::posix_time::microseconds(m_uiFrameDurationUs);
      }
      // launch next read in fps time
      VLOG(15) << "New sample: " << pMediaSample->getStartTime()
              << " Old sample: " << m_dPreviousSampleTime
              << " : waiting for " << m_uiFrameDurationUs << "us";
//      m_timer.expires_from_now(boost::posix_time::microseconds(m_uiFrameDurationUs));
      m_timer.expires_at(tNext);
      m_timer.async_wait(boost::bind(&AsyncStreamMediaSource::readAccessUnit, this, _1) );
    }
    else
    {
      // read next AU
      m_timer.expires_from_now(boost::posix_time::microseconds(0));
      m_timer.async_wait(boost::bind(&AsyncStreamMediaSource::readAccessUnit, this, _1) );
    }
  }
  else
  {
    flushMediaSamplesInBuffers();
    VLOG(15) << "Stopping source: m_bStopping: " << m_bStopping << " ec: " << ec.message();

    LOG(WARNING) << "EOF: Didn't find AU";
    if (m_onComplete)(m_onComplete(ec));
  }
}

uint32_t AsyncStreamMediaSource::readDataIntoBuffer()
{
  // for looping
  if (!m_in.good())
  {
    if (m_bTryLoop)
    {
      VLOG(2) << "Looping mode: " << m_bTryLoop << " " << m_uiCurrentLoop + 1 << "/" << m_uiLoopCount;
      if (m_uiLoopCount != 0)
      {
        if(m_uiCurrentLoop < m_uiLoopCount - 1)
        {
          LOG_FIRST_N(INFO, 1) << "In lopping mode: rewinding stream";
          m_in.clear();
          m_in.seekg(0, std::ios_base::beg);
          ++m_uiCurrentLoop;
        }
        else
        {
          return 0;
        }
      }
      else
      {
        // "infinite" looping with m_bTryLoop
        LOG_FIRST_N(INFO, 1) << "In lopping mode: rewinding stream";
        m_in.clear();
        m_in.seekg(0, std::ios_base::beg);
      }
    }
    else
    {
      return 0;
    }
  }

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

  VLOG(10) << "Read " << uiRead << " bytes into buffer";
  return uiRead;
}

boost::system::error_code AsyncStreamMediaSource::start()
{
  VLOG(10) << "Starting stdin media source";
  m_bStopping = false;

  if (!m_pMediaStreamParser)
  {
    LOG(ERROR) << "Unsupported media type: " << m_sMediaType;
    return boost::system::error_code();
  }

  if (!m_accessUnitCB)
  {
    LOG_FIRST_N(WARNING, 1) << "No callback configured for access units";
    return boost::system::error_code();
  }

  // start first async read
  readAccessUnit(boost::system::error_code());
  return boost::system::error_code();
}

void AsyncStreamMediaSource::flushMediaSamplesInBuffers()
{
  // FLUSH buffers
  // There possibly are AUs in the AU buffer as well as samples in the parser
  std::vector<MediaSample> vBufferedMediaSamples = m_pMediaStreamParser->flush();
  // process the flushed samples one by one
  for (const auto& mediaSample: vBufferedMediaSamples)
  {
    addToAccessUnit(mediaSample);
  }
  // finally output leftover media samples as AU
  if (!m_vAccessUnitBuffer.empty()) outputAccessUnit();
}

boost::system::error_code AsyncStreamMediaSource::stop()
{
  m_bStopping = true;
  return boost::system::error_code();
}

}
}
