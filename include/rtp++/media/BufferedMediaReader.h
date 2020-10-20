#pragma once
#include <cassert>
#include <cstdint>
#include <istream>
#include <memory>
#include <rtp++/media/MediaSample.h>
#include <rtp++/media/MediaStreamParser.h>

namespace rtp_plus_plus
{
namespace media
{

/**
 * Buffered media reader that reads media from a std::istream. Media is
 * parsed using the configured MediaStreamParser.
 */
class BufferedMediaReader
{
  static const uint32_t DEFAULT_READ_SIZE;
public:
  /**
   * Constructor
   * @param source The istream to be read from
   * @param uiInitialBufferSize The starting size of the internal buffer
   * @param uiReadSize The size of reads into the internal buffer
   */
  BufferedMediaReader(std::istream& source, uint32_t uiInitialBufferSize, uint32_t uiReadSize = DEFAULT_READ_SIZE);
  /**
   * Constructor
   * @param source The istream to be read from
   * @param pMediaStreamParser The media stream parser used to parse the raw data from the istream
   * @param uiInitialBufferSize The starting size of the internal buffer
   * @param uiReadSize The size of reads into the internal buffer
   */
  BufferedMediaReader(std::istream& source, std::unique_ptr<MediaStreamParser> pMediaStreamParser,
                      uint32_t uiInitialBufferSize, uint32_t uiReadSize = DEFAULT_READ_SIZE);
  /**
   * Method to configure media stream parser
   * @param pMediaStreamParser Media stream parser to be used
   */
  void setMediaStreamParser(std::unique_ptr<MediaStreamParser> pMediaStreamParser);
  /**
   * returns whether end of file has been reached
   */
  bool eof() const;
  /**
   * This method tries to read a media samples
   * if it is possible. In the case where there
   * is no more data to be read, it will return
   * a null ptr
   */
  boost::optional<MediaSample> readAndExtractMedia();
private:

  void readMoreDataIfNecessary();

  std::istream& m_in;       /// input stream
  std::unique_ptr<MediaStreamParser> m_pMediaStreamParser;
  Buffer m_buffer;          /// buffer for reads
  uint32_t m_uiReadSize;    /// Read data
  uint32_t m_uiCurrentPos;  /// pointer for new reads
  bool m_bNeedMoreData;     /// flag if we need to read more data
  bool m_bEof;              /// eof flag

};

} // media
} // rtp_plus_plus
