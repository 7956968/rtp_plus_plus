#pragma once
#include <ostream>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/MediaSink.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>

namespace rtp_plus_plus
{
namespace media
{
namespace h264
{

class H264AnnexBStreamWriter : public MediaSink
{
public:
  /**
   * @brief Writes H.264 NAL units to file
   */
  H264AnnexBStreamWriter(const std::string& sDest, bool bPrependParameterSets, /*bool bStreamContainsStartCodes = false, */bool bInsertAUDsIfNotPresent = false)
    :MediaSink(sDest),
      m_bPrependParameterSets(bPrependParameterSets),
      //m_bStreamContainsStartCodes(bStreamContainsStartCodes),
      m_bInsertAUDsIfNotPresent(bInsertAUDsIfNotPresent),
      m_bPrepended(false)
  {

  }

  void setParameterSets(const std::string& sSps, const std::string& sPps)
  {
    m_sSps = sSps;
    m_sPps = sPps;
  }

  virtual void write(const MediaSample& mediaSample)
  {
    writeMediaSampleNaluToStream(mediaSample, *m_out, true);
  }

  virtual void writeAu(const std::vector<MediaSample>& mediaSamples)
  {
    if (m_bPrependParameterSets)
    {
      // only write once for now: later prepend to each IDR
      if (!m_bPrepended)
      {
        NalUnitType eType = getNalUnitType(mediaSamples[0]);
        if (eType == h264::NUT_CODED_SLICE_OF_AN_IDR_PICTURE)
        {
          VLOG(2) << "First IDR: Prepending parameter sets to h264 stream";
          // Make sure parameter sets are set
          m_bPrepended = true;
          if (m_bInsertAUDsIfNotPresent)
          {
            writeAudsIfNotPresent(mediaSamples);
          }
          if (!m_sSps.empty() && !m_sPps.empty())
          {
            VLOG(2) << "Prepending parameter sets to h264 stream";
            const int8_t startcode[4] = {0, 0, 0, 1};
            m_out->write((const char*) startcode, 4);
            m_out->write(m_sSps.c_str(), m_sSps.length());
            m_out->write((const char*) startcode, 4);
            m_out->write(m_sPps.c_str(), m_sPps.length());
          }
        }
      }
    }

    if (m_bInsertAUDsIfNotPresent)
    {
      writeAudsIfNotPresent(mediaSamples);
    }

    for (const MediaSample& mediaSample : mediaSamples)
    {
      writeMediaSampleNaluToStream(mediaSample, *m_out, true);
    }
  }

  static void writeMediaSampleNaluToStream(const MediaSample& mediaSample, std::ostream& out, bool useTiming = false)
  {
    static boost::posix_time::ptime previousSampletime = mediaSample.getPresentationTime();

    // HACK: use start code length from media sample if set
    const int8_t startcode[4] = { 0, 0, 0, 1 };
    if (!useTiming)
    {
      if (!mediaSample.doesNaluContainsStartCode())
      {
        int32_t iStartCodeLength = mediaSample.getStartCodeLengthHint();
        if (iStartCodeLength == -1)
        {
          // HACK FOR NOW: test H.264 file writing: This code relies on there being AUDs in the stream to
          // write the start codes correctly
          NalUnitType eType = getNalUnitType(mediaSample);
          switch (eType)
          {
            case NUT_SEQUENCE_PARAMETER_SET:
            case NUT_PICTURE_PARAMETER_SET:
            case NUT_ACCESS_UNIT_DELIMITER:
            {
              out.write((const char*)startcode, 4);
              break;
            }
            default:
            {
              out.write((const char*)startcode + 1, 3);
              break;
            }
          }
        }
        else
        {
          assert(iStartCodeLength == 3 || iStartCodeLength == 4);
          switch (iStartCodeLength)
          {
            case 3:
            {
              out.write((const char*)startcode + 1, 3);
              break;
            }
            case 4:
            {
              out.write((const char*)startcode, 4);
              break;
            }
          }
        }
        // this will write the start code if contained in sample
        out.write((const char*)mediaSample.getDataBuffer().data(), mediaSample.getDataBuffer().getSize());
      }
    }
    else
    {
      if (!mediaSample.doesNaluContainsStartCode())
      {
        NalUnitType eType = getNalUnitType(mediaSample);
        switch (eType)
        {
          case NUT_SEQUENCE_PARAMETER_SET:
          case NUT_PICTURE_PARAMETER_SET:
          case NUT_ACCESS_UNIT_DELIMITER:
          {
            out.write((const char*)startcode, 4);
            break;
          }
          default:
          {
            if (previousSampletime != mediaSample.getPresentationTime()){
              previousSampletime = mediaSample.getPresentationTime();
              out.write((const char*)startcode, 4);
              break;
            }
            else{
              out.write((const char*)startcode + 1, 3);
              break;
            }
          }
        }
      }
      out.write((const char*)mediaSample.getDataBuffer().data(), mediaSample.getDataBuffer().getSize());
    }
  }

private:
  void writeAudsIfNotPresent(const std::vector<MediaSample>& mediaSamples)
  {
    if (!doSamplesContainAud(mediaSamples))
    {
      // WARNING: this is not a valid AUD, an actual AUD contains type information about the following NALUs
      const int8_t startcodeWithAud[6] = { 0, 0, 0, 1, 9, 47 };
      m_out->write((const char*)startcodeWithAud, 6);
    }
  }

  bool doSamplesContainAud(const std::vector<MediaSample>& mediaSamples)
  {
    bool bSampleContainsAud = false;
    // check if first media sample contains AUD
    const MediaSample& mediaSample = mediaSamples[0];
    //if (m_bStreamContainsStartCodes)
    //{
    //  // AUDs always have 4-byte start code
    //  const int8_t startcode[4] = { 0, 0, 0, 1 };
    //  if (memcmp(startcode, mediaSample.getDataBuffer().data(), 4))
    //  {
    //    NalUnitType eType = getNalUnitType(mediaSample.getDataBuffer().data()[4]);
    //    if (eType == NUT_ACCESS_UNIT_DELIMITER)
    //      bSampleContainsAud = true;
    //  }
    //}
    //else
    //{
      NalUnitType eType = getNalUnitType(mediaSample);
      if (eType == NUT_ACCESS_UNIT_DELIMITER)
        bSampleContainsAud = true;
    //}
    return bSampleContainsAud;
  }

  bool m_bPrependParameterSets;
  //bool m_bStreamContainsStartCodes;
  bool m_bInsertAUDsIfNotPresent;
  bool m_bPrepended;
  std::string m_sSps;
  std::string m_sPps;
};

} // h264
} // media
} // rtp_plus_plus
