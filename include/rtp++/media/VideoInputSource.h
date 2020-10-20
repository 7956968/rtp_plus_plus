#pragma once
#include <memory>
#include <string>
#include <vector>
#if 0
#include <boost/enable_shared_from_this.hpp>
#endif
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <cpputil/StringTokenizer.h>
#include <rtp++/media/MediaModels.h>
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{

typedef boost::function<boost::system::error_code(const media::MediaSample&)> ReceiveMediaCb_t;

/**
 * @brief The VideoInputSource class models a real video codec, which is characterised by
 * - framerate
 * - bitrate
 */
class VideoInputSource : public std::enable_shared_from_this<VideoInputSource>
{
public:
  static std::shared_ptr<VideoInputSource> create(boost::asio::io_service& ioService,
                                                  const std::string& sVideoMediaType,
                                                  uint32_t uiFps,
                                                  IVideoModel* pVideoBitrateModel,
                                                  IDataModel* pDataModel)
  {
    return std::make_shared<VideoInputSource>(ioService, sVideoMediaType, uiFps, pVideoBitrateModel, pDataModel);
  }

  /**
   * @brief VideoInputSource
   * @param sVideoMediaType
   * @param uiFps
   * @param sVideoBitrateModel
   */
  VideoInputSource(boost::asio::io_service& ioService,
                   const std::string& sVideoMediaType,
                   uint32_t uiFps,
                   IVideoModel* pVideoBitrateModel,
                   IDataModel* pDataModel)
    : m_rIoService(ioService),
    m_sVideoMediaType(sVideoMediaType),
    m_timer(ioService),
    m_uiFrameDurationUs((uiFps != 0) ? 1000000/uiFps : 25),
    m_dCurrentAccessUnitPts(0.0),
    m_dFrameDuration(1.0/uiFps),
    m_pVideoBitrateModel(pVideoBitrateModel),
    m_pDataModel(pDataModel),
    m_uiAuCount(0)
  {
    VLOG(2) << sVideoMediaType << " video input source: " << uiFps << "fps";
  }

  void setReceiveMediaCB(ReceiveMediaCb_t val) { m_receiveMediaCB = val; }

  boost::system::error_code start()
  {
    VLOG(2) << " VideoInputSource::start";
    // get NALU
    if (outputVideoFrame())
    {
      m_tStart = boost::posix_time::microsec_clock::universal_time();
      // outputNal sets m_tStart
      m_timer.expires_at(m_tStart + boost::posix_time::microseconds(m_uiFrameDurationUs));
      m_timer.async_wait(boost::bind(&VideoInputSource::handleTimeout, this, boost::asio::placeholders::error));
    }

    return boost::system::error_code();
  }

  boost::system::error_code stop()
  {
    VLOG(2) << " VideoInputSource::stop";
    // cancel timer
    m_timer.cancel();
    return boost::system::error_code();
  }

protected:
  /**
   * @brief handleTimeout
   * @param ec
   */
  void handleTimeout(const boost::system::error_code& ec)
  {
    if (!ec)
    {
      VLOG(12) << " VideoInputSource::handleTimeout";
      outputVideoFrame();
      m_timer.expires_from_now(boost::posix_time::microseconds(m_uiFrameDurationUs));
      m_timer.async_wait(boost::bind(&VideoInputSource::handleTimeout, this, boost::asio::placeholders::error));
    }
    else
    {
      // log error
      if (ec != boost::asio::error::operation_aborted)
      {
        LOG(WARNING) << "Timed out error: %1%" << ec.message();
      }
    }
  }
  /**
   * @brief outputVideoFrame
   * @return
   */
  bool outputVideoFrame()
  {
    if ( m_pVideoBitrateModel->getNextFrame(m_uiNextFrameSize, m_mediaSample) )
    {
      Buffer& buffer = const_cast<Buffer&>(m_mediaSample.getDataBuffer());
      m_pDataModel->fillBuffer(buffer);
      // TODO: set start time on media sample

    }
    if (m_receiveMediaCB)
    {
      m_receiveMediaCB(m_mediaSample);
    }
    return true;
  }

private:

  VideoInputSource(const VideoInputSource&) = delete;
  VideoInputSource& operator=(const VideoInputSource&) = delete;

private:

  boost::asio::io_service& m_rIoService;
  boost::asio::deadline_timer m_timer;

  std::string m_sVideoMediaType;

  ReceiveMediaCb_t m_receiveMediaCB;

  uint32_t m_uiFrameDurationUs;

  // TS of current AU
  double m_dCurrentAccessUnitPts;
  double m_dFrameDuration;
  // outgoing AU counter
  uint32_t m_uiAuCount;

  boost::posix_time::ptime m_tStart;

  IVideoModel* m_pVideoBitrateModel;
  IDataModel* m_pDataModel;
  media::MediaSample m_mediaSample;
  uint32_t m_uiNextFrameSize;
};

} // namespace rtp_plus_plus

