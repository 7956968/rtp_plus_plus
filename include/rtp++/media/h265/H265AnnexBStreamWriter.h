#pragma once
#include <ostream>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/MediaSink.h>
#include <rtp++/media/h265/H265NalUnitTypes.h>

namespace rtp_plus_plus
{
namespace media
{
namespace h265
{

class H265AnnexBStreamWriter : public MediaSink
{
public:
  H265AnnexBStreamWriter(const std::string& sDest, bool bPrependParameterSets)
    :MediaSink(sDest),
      m_bPrependParameterSets(bPrependParameterSets)
  {

  }

  virtual void write(const MediaSample& mediaSample)
  {
    writeMediaSampleNaluToStream(mediaSample, *m_out, true);
  }

  virtual void writeAu(const std::vector<MediaSample>& mediaSamples)
  {
    for (const MediaSample& mediaSample : mediaSamples)
    {
      writeMediaSampleNaluToStream(mediaSample, *m_out, true);
    }
  }

  static void writeMediaSampleNaluToStream(const MediaSample& mediaSample, std::ostream& out, bool useTiming = false)
  {
    // HACK FOR NOW: test H.264 file writing: This code relies on there being AUDs in the stream to
    // write the start codes correctly
    static uint32_t previousSampletime = mediaSample.getRtpTime();
    const int8_t startcode[4] = {0, 0, 0, 1};
    if(!useTiming){
      int32_t iStartCodeLength = mediaSample.getStartCodeLengthHint();
      if (iStartCodeLength == -1)
      {
        // HACK FOR NOW: test H.264 file writing: This code relies on there being AUDs in the stream to
        // write the start codes correctly
        NalUnitType eType = getNalUnitType(mediaSample);
        switch (eType)
        {
          case NUT_VPS:
          case NUT_SPS:
          case NUT_PPS:
          case NUT_AUD:
          {
            out.write((const char*) startcode, 4);
            break;
          }
          default:
          {
            out.write((const char*) startcode + 1, 3);
            break;
          }
        }
        out.write((const char*) mediaSample.getDataBuffer().data(), mediaSample.getDataBuffer().getSize());
      }
      else
      {
        assert(iStartCodeLength == 3 || iStartCodeLength == 4);
        switch (iStartCodeLength)
        {
          case 3:
          {
            out.write((const char*) startcode + 1, 3);
            break;
          }
          case 4:
          {
            out.write((const char*) startcode, 4);
            break;
          }
        }
        out.write((const char*) mediaSample.getDataBuffer().data(), mediaSample.getDataBuffer().getSize());
      }
    }else{
      NalUnitType eType = getNalUnitType(mediaSample);
      switch (eType)
      {
        case NUT_VPS:
        case NUT_SPS:
        case NUT_PPS:
        case NUT_AUD:
        {
          out.write((const char*) startcode, 4);
          break;
        }
        default:
        {
          if(previousSampletime!=mediaSample.getRtpTime()){
            previousSampletime = mediaSample.getRtpTime();
            out.write((const char*) startcode, 4);
            break;
          }else{
            out.write((const char*) startcode + 1, 3);
            break;
          }
        }
      }
      out.write((const char*) mediaSample.getDataBuffer().data(), mediaSample.getDataBuffer().getSize());
    }

  }

protected:
  bool m_bPrependParameterSets;
};

}
}
}
