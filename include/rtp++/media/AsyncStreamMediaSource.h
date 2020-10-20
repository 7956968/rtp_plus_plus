#pragma once
#include <istream>
#include <memory>
#include <string>
#include <vector>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>
#include <boost/system/error_code.hpp>
#include <boost/shared_ptr.hpp>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/MediaStreamParser.h>

#define READ_SIZE 1024
#define DEFAULT_FRAME_RATE 30

namespace rtp_plus_plus
{
namespace media
{

// Callback for access units
typedef boost::function<boost::system::error_code (const std::vector<MediaSample>&)> ReceiveAccessUnitCb_t;

/**
  * DEPRECATED: NalUnitMediaSource is used now for parsing AnnexB H.264 and H.265 streams
  * This media source reads from an std istream. This could be a std::cin or a file
  * The source has to mark access unit boundaries in order to be able to set the sample time
  * as well as the marker bit of the media sample. This is used in the RTP packetization.
  * The rules for determining the end of an access unit are in the standard
  * But for simplicity sake, we have configured ffmpeg to output Access Unit Delimiters
  * in the stream:
  * THIS CODE WILL NOT FUNCTION CORRECTLY IF AUDs ARE NOT PRESENT.
  */
class AsyncStreamMediaSource
{
public:
  typedef boost::function<void (boost::system::error_code)> CompletionHandler_t;

  AsyncStreamMediaSource(boost::asio::io_service& ioService,
                         std::istream& in1, const std::string& sMediaType,
                         uint32_t uiInitialBufferSize, bool bTryLoopSource, uint32_t uiLoopCount,
                         uint32_t uiFps, bool bLimitOutputToFramerate);
  ~AsyncStreamMediaSource();

  void setReceiveAccessUnitCB(ReceiveAccessUnitCb_t val);
  /// Completion handler
  void setCompletionHandler(CompletionHandler_t onComplete) { m_onComplete = onComplete; }

  boost::system::error_code start();
  boost::system::error_code stop();

private:
  void readAccessUnit(const boost::system::error_code& ec);
  // returns how much data was read from the stream
  uint32_t readDataIntoBuffer();
  bool belongsToNewAccessUnit(const MediaSample& mediaSample);
  void addToAccessUnit(const MediaSample& mediaSample);
  void outputAccessUnit();
  void flushMediaSamplesInBuffers();

private:

  boost::asio::deadline_timer m_timer;
  std::istream& m_in;
  std::string m_sMediaType;
  Buffer m_buffer;
  uint32_t m_uiCurrentPos; /// pointer for new reads
  bool m_bStopping;        /// Stopping flag

  ReceiveAccessUnitCb_t m_accessUnitCB;

  std::unique_ptr<MediaStreamParser> m_pMediaStreamParser;
  uint32_t m_uiTotalSampleCount;
  std::vector<MediaSample> m_vAccessUnitBuffer;

  /// for file sources
  bool m_bTryLoop;
  uint32_t m_uiLoopCount;
  uint32_t m_uiCurrentLoop;

  /// Framerate of source
  uint32_t m_uiFps;
  uint32_t m_uiFrameDurationUs;
  /// This forces the source to delay access units by the fps interval.
  /// This can be useful when reading media from file
  bool m_bLimitOutputToFramerate;
  boost::posix_time::ptime m_tPrevious;

  /// To keep track of when to sleep if the framerate is set
  double m_dPreviousSampleTime;

  /// for statistics calculation
  double m_dLastSampleTime;
  uint64_t m_uiTotalBytes;
  uint32_t m_uiAccessUnitSize;

  // on end of file
  CompletionHandler_t m_onComplete;
};

}
}

