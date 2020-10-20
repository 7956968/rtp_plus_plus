#pragma once
#include <ostream>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/MediaSink.h>

namespace rtp_plus_plus
{
namespace media
{

class AmrFileSink : public MediaSink
{
public:
  AmrFileSink(const std::string& sDest)
    :MediaSink(sDest)
  {
    // write header
    const char amrHeader[] = { 0x23, 0x21, 0x41, 0x4D, 0x52, 0x0A };
    m_out->write(amrHeader, 6);
  }
};

} // media
} // rtp_plus_plus
