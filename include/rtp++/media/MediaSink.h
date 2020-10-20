#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{
namespace media
{

class MediaSink
{
public:

  MediaSink(const std::string& sDest);
  virtual ~MediaSink();

  virtual void write(const MediaSample& mediaSample);
  virtual void writeAu(const std::vector<MediaSample>& mediaSamples);

protected:

  std::ofstream* m_pFileOut;
  std::ostream* m_out;
};

} // media
} // rtp_plus_plus

