#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <cpputil/Buffer.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/MediaSource.h>

namespace rtp_plus_plus
{
namespace media
{

/**
 * @brief
 */
class YuvMediaSource : public MediaSource
{
public:
  /**
   * @brief YuvMediaSource
   * @param sFilename The name of the file containing the Annex B bitstream
   * @param sMediaType Media type identifier. Currently only H264 and H265 are supported
   * @param bLoopSource Configures the source to loop on end of stream
   * @param uiLoopCount Configures the number of times the source is looped
   * IFF bLoopSource is true. A value of 0 means that the source will loop
   * indefinitely
   */
  YuvMediaSource(const std::string& sFilename, const uint32_t uiWidth, const uint32_t uiHeight, bool bLoopSource, uint32_t uiLoopCount, uint32_t uiInitialBufferSize = 20000);
  /**
   * @brief YuvMediaSource
   * @param in1 A reference to the istream that has opened the Annex B stream
   * @param bLoopSource Configures the source to loop on end of stream
   * @param uiLoopCount Configures the number of times the source is looped
   * IFF bLoopSource is true. A value of 0 means that the source will loop
   * indefinitely
   */
  YuvMediaSource(std::istream& in1, const uint32_t uiWidth, const uint32_t uiHeight, bool bLoopSource, uint32_t uiLoopCount, uint32_t uiInitialBufferSize = 20000);
  /**
   * @brief isGood This method can be used to check if NAL units can be read. If the end of
   * stream is reached and the source is not configured to loop or if the maximum loop count
   * is reached, this will return false. This will also return false if the file could not be
   * found or if there was an error parsing the file.
   * @return if the NalUnitMediaSource is in a state to be read from
   */
  bool isGood() const;
  /**
   * @brief Overridden from MediaSource. getNextMediaSample() does not apply to a NAL unit source as
   * we only want to deal with entire AUs
   * @return A null pointer
   */
  boost::optional<MediaSample> getNextMediaSample();
  /**
   * @brief Overridden from MediaSource. getNextAccessUnit() returns the next access unit in the source.
   * @return The next access unit in the source and an empty vector if isGood() == false
   */
  std::vector<MediaSample> getNextAccessUnit();

private:
  /**
   * @brief checkInputStream makes sure that the stream is in a good state and otherwise updates m_bEos
   */
  void checkInputStream();
  /**
   * @brief parseAnnexBStream parses the stream and extracts the NAL unit and access unit info
   */
  void parseStream();
  /**
   * @brief readMediaSample
   * @return
   */
  std::vector<MediaSample> readMediaSample();
private:

  // stream handle when reading from file
  std::ifstream m_in;

  // stream reference used for actual read
  std::istream& m_rIn;

  enum MediaType
  {
    MT_YUV_420P,
  };
  MediaType m_eType;
  std::string m_sFilename;
  uint32_t m_uiWidth;
  uint32_t m_uiHeight;

  // state of source
  bool m_bEos;
  // if source should loop
  bool m_bLoopSource;
  // total number of loops if m_bLoopSource
  uint32_t m_uiLoopCount;
  // Current loop
  uint32_t m_uiCurrentLoop;
  // frame size
  uint32_t m_uiYuvFrameSize;
  // total frames
  uint32_t m_uiTotalFrames;
  // current frame
  uint32_t m_uiCurrentFrame;
  // buffer to read data into
  Buffer m_buffer;
};

} // media
} // rtp_plus_plus
