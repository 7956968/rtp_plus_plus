#include "CorePch.h"
#include <rtp++/media/MediaSink.h>

namespace rtp_plus_plus
{
namespace media
{

MediaSink::MediaSink(const std::string& sDest)
  :m_pFileOut(0),
    m_out(&std::cout)
{
  VLOG(2) << "Writing incoming media to " << sDest;
  if (sDest != "cout")
  {
    m_pFileOut = new std::ofstream(sDest.c_str(), std::ofstream::binary);
    m_out = m_pFileOut;
  }
}

MediaSink::~MediaSink()
{
  if (m_pFileOut)
  {
    m_pFileOut->close();
    delete m_pFileOut;
  }
}

void MediaSink::write(const MediaSample& mediaSample)
{
  m_out->write((const char*) mediaSample.getDataBuffer().data(), mediaSample.getDataBuffer().getSize());
}

void MediaSink::writeAu(const std::vector<MediaSample>& mediaSamples)
{
  for (const MediaSample& mediaSample : mediaSamples)
  {
    m_out->write((const char*) mediaSample.getDataBuffer().data(), mediaSample.getDataBuffer().getSize());
  }
}

} // media
} // rtp_plus_plus
