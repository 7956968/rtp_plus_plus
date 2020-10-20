#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <cpputil/Buffer.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>
#include <rtp++/media/h265/H265NalUnitTypes.h>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/MediaSource.h>

namespace rtp_plus_plus
{
namespace media
{

/**
 * @brief The NalUnitMediaSource class is used to retrieve NAL units from a file
 * The file is parsed on startup and the starting positions for each NAL unit are
 * stored in a vector. This is a simper, more reliable and efficient way then the
 * AsyncStreamMediaApproach used previously.
 */
class NalUnitMediaSource : public MediaSource
{
  typedef std::tuple<size_t, uint32_t, size_t, uint32_t> NalUnitInfo_t;
  typedef std::vector<NalUnitInfo_t> AccessUnitInfo_t;

public:
  /**
   * @brief NalUnitMediaSource
   * @param sFilename The name of the file containing the Annex B bitstream
   * @param sMediaType Media type identifier. Currently only H264 and H265 are supported
   * @param bLoopSource Configures the source to loop on end of stream
   * @param uiLoopCount Configures the number of times the source is looped
   * IFF bLoopSource is true. A value of 0 means that the source will loop
   * indefinitely
   */
  NalUnitMediaSource(const std::string& sFilename, const std::string& sMediaType, bool bLoopSource, uint32_t uiLoopCount, uint32_t uiInitialBufferSize = 20000);
  /**
   * @brief NalUnitMediaSource
   * @param in1 A reference to the istream that has opened the Annex B stream
   * @param bLoopSource Configures the source to loop on end of stream
   * @param uiLoopCount Configures the number of times the source is looped
   * IFF bLoopSource is true. A value of 0 means that the source will loop
   * indefinitely
   */
  NalUnitMediaSource(std::istream& in1, const std::string& sMediaType, bool bLoopSource, uint32_t uiLoopCount, uint32_t uiInitialBufferSize = 20000);
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
  void parseAnnexBStream();
  /**
   * @brief readMediaSamples
   * @param auInfo
   * @return
   */
  std::vector<MediaSample> readMediaSamples(const AccessUnitInfo_t& auInfo);
  /**
   * @brief finaliseAu Stores the NAL units collected so far as an AU
   */
  void finaliseAu(AccessUnitInfo_t& vCurrentAccessUnit);
private:

  // stream handle when reading from file
  std::ifstream m_in;

  // stream reference used for actual read
  std::istream& m_rIn;

  enum MediaType
  {
    MT_H264,
    MT_H265
  };
  MediaType m_eType;
  std::string m_Filename;

  // state of source
  bool m_bEos;
  // if source should loop
  bool m_bLoopSource;
  // total number of loops if m_bLoopSource
  uint32_t m_uiLoopCount;
  // Current loop
  uint32_t m_uiCurrentLoop;
  // Current (next) AU
  uint32_t m_uiCurrentAccessUnit;

  // buffer to read data into
  Buffer m_buffer;

  /// starting index - start code length - starting index of NAL unit header - NAL unit length (without start code)
  // vector to store the starting indices of all start codes
  std::vector<NalUnitInfo_t> m_vStartCodeInfo;
  // vector to store all AU info
  std::vector<AccessUnitInfo_t> m_vAccessUnitInfo; 

  Buffer m_temporaryNalUnitBuffer;

  // members for keeping AU state
  // H264 SVC
  bool m_bCurrentLayerIsBaseLayer;
  // HEVC
  bool m_bFirstH265NalUnit;
  h265::NalUnitType m_ePrevH265Type;
};

} // media
} // rtp_plus_plus
