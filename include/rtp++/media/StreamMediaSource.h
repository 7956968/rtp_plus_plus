#pragma once
#include <istream>
#include <memory>
#include <string>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <boost/shared_ptr.hpp>
#include <cpputil/ServiceThread.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/MediaStreamParser.h>

#define READ_SIZE 1024
#define DEFAULT_FRAME_RATE 30

namespace rtp_plus_plus
{
namespace media
{

// Callback for single media samples
typedef boost::function<boost::system::error_code (const MediaSample&)> ReceiveMediaCb_t;
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
class StreamMediaSource
{
public:
  enum Mode
  {
    SingleMediaSample, // Output single media sample at a time
    AccessUnit         // Output access unit at a time
  };

  StreamMediaSource(std::istream& in1, const std::string& sMediaType,
                   uint32_t uiInitialBufferSize, bool bTryLoopSource, uint32_t uiLoopCount,
                   uint32_t uiFps, bool bLimitOutputToFramerate, Mode eMode);
  ~StreamMediaSource();

  void setReceiveMediaCB(ReceiveMediaCb_t val);
  void setReceiveAccessUnitCB(ReceiveAccessUnitCb_t val);

  boost::system::error_code onStart();
  boost::system::error_code start();
  boost::system::error_code stop();
  boost::system::error_code onComplete();

private:

  int extractAndProcessMediaSamples();
  void processMediaSample(const MediaSample& mediaSample);
  bool processAccessUnitOnCompletion(const MediaSample& mediaSample);
  void outputAccessUnit();
  void flushMediaSamplesInBuffers();
  bool shouldStop() const;

private:

  boost::asio::io_service m_io;
  std::istream& m_in;
  std::string m_sMediaType;
  Buffer m_buffer;
  uint32_t m_uiCurrentPos; /// pointer for new reads
  bool m_bStopping;        /// Stopping flag

  Mode m_eMode;
  ReceiveMediaCb_t m_receiveMediaCB;
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

  /// To keep track of when to sleep if the framerate is set
  double m_dPreviousSampleTime;

  /// for statistics calculation
  double m_dLastSampleTime;
  uint64_t m_uiTotalBytes;
  uint32_t m_uiAccessUnitSize;
};

}
}

template <>
struct ServiceTraits<rtp_plus_plus::media::StreamMediaSource>
{
  static const char* name;
};

template <>
struct ServiceTraits<boost::shared_ptr<rtp_plus_plus::media::StreamMediaSource> >
{
  static const char* name;
};

typedef ServiceThread<rtp_plus_plus::media::StreamMediaSource> StreamMediaSourceService;
typedef boost::shared_ptr<ServiceThread< boost::shared_ptr<rtp_plus_plus::media::StreamMediaSource> > > StreamMediaSourceServicePtr;
