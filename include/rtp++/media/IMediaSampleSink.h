#pragma once
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{
namespace media
{

class IMediaSampleSink
{
public:

  virtual ~IMediaSampleSink()
  {

  }

  virtual void send(const MediaSample& mediaSample) = 0;
  virtual void send(const std::vector<MediaSample>& vMediaSamples) = 0;
  virtual void send(const MediaSample& mediaSample, const std::string& sMid) = 0;
  virtual void send(const std::vector<MediaSample>& vMediaSamples, const std::string& sMid) = 0;
};

}
}

