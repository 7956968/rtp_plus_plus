#pragma once
#include <rtp++/media/MediaStreamParser.h>

#include <boost/date_time/posix_time/ptime.hpp>

namespace rtp_plus_plus
{
namespace media
{
namespace h265
{

/**
 * Basic H.265 stream parser that searches for start codes to find NAL units
 * This parser relies on the presence of Access Unit Delimiters to set time stamps
 * marker bits for frames.
 * This parser buffers the previous sample so that the marker bit can be set correctly
 */
class H265AnnexBStreamParser : public MediaStreamParser
{
public:
  // this determines whether the sampling time is used for new samples
  // or whether the framerate is used to increment the sample time
  /**
    * This enum cofnigures how media samples are timestamped
    */
  enum TimingMode
  {
    TM_NONE,
    TM_LIVE,
    TM_FRAME_RATE
  };

  /**
    * Constructor
    */
  H265AnnexBStreamParser();

  /**
    * Constructor
    * @param uint32_t uiFramerate The framerate is used to calculate timestamps if the object is using TM_FRAME_RATE
    */
  H265AnnexBStreamParser(uint32_t uiFramerate);

  /**
    * This method determines how samples are timestamped: either not at all, according to framerate or according to the system time.
    * @param [in] eMode Can be TM_NONE, TM_LIVE or TM_FRAME_RATE
    */
  void setMode(TimingMode eMode);

  /**
    * Sets the framerate used for timestamping samples if the object is in frame rate mode.
    * @param [in] uint32_t uiFramerate The framerate is used to calculate timestamps if the object is using TM_FRAME_RATE
    */
  void setFramerate(uint32_t uiFramerate);
  /**
    * Flushes the buffered sample
    * @return A pointer to the previous buffered media sample if one exists, otherwise a null pointer
    */
  std::vector<MediaSample> flush();

  /**
    * Tries to extract a media sample from the bit stream. This method outputs the
    * previously output sample once the next sample was found in the bitstream.
    * This approach is used since we need the next sample to set the marker bit
    * of the previously buffered sample correctly
    * @param [in] pBuffer Buffer containing the raw bitstream
    * @param [in] uiBufferSize Size of the buffer containing the raw sample
    * @param [out] sampleSize Size of the NEXT media sample if successful
    * @return A media sample pointer containing the previously buffered sample.
    */
  boost::optional<MediaSample> extract(const uint8_t* pBuffer, uint32_t uiBufferSize,
                                       int32_t& sampleSize, bool bMoreDataWaiting);

  /**
    * This method takes the passed in buffer and looks for all samples in the buffer.
    * It should be called to handle media samples that passed in from some event loop
    * e.g. VLC or DirectShow where each buffer may contain multple media frames.
    * Each method call should contain a new access unit in the buffer.
    * @param [in] pBuffer Buffer containing the raw bitstream
    * @param [in] uiBufferSize Size of the buffer containing the raw sample
    * @param [in] dCurrentAccessUnitPts The tiestamp that should be applied to all NAL units
    * @return A vector containing all samples found in the buffer.
    */
  std::vector<MediaSample> extractAll(const uint8_t* pBuffer, uint32_t uiBufferSize,
                                      double dCurrentAccessUnitPts);

private:

  bool searchForNextNalStartCodeAndNalUnitType(const uint8_t* pBuffer, uint32_t uiBufferSize,
                                               uint32_t uiStartPos, uint32_t& uiPos,
                                               uint8_t& uiNut, uint8_t& uiLayerId, bool & bFirstCTB, uint32_t& uiStartCodeLen);
  TimingMode m_eMode;

  // framerate
  uint32_t m_uiFramerate;
  double m_dFrameDuration;

  // used to calculate sample times
  boost::posix_time::ptime m_tStart;

  /// The marker bit on a sample can
  /// only be set once we have seen
  /// the next sample
  boost::optional<MediaSample> m_pPreviousSample;
  /// flag for setting H.264 sample
  /// times and marker bits
  bool m_bNewAccessUnit;
  double m_dCurrentAccessUnitPts;
  uint8_t m_uiNALType;

  uint32_t m_uiAccessUnitCount;
};

}
}
}

