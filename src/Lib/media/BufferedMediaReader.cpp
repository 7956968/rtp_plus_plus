#include "CorePch.h"
#include <rtp++/media/BufferedMediaReader.h>

#include <cassert>
#include <istream>
#include <memory>

#include <rtp++/media/MediaStreamParser.h>

namespace rtp_plus_plus
{
namespace media
{

const uint32_t BufferedMediaReader::DEFAULT_READ_SIZE = 1024;

BufferedMediaReader::BufferedMediaReader(std::istream& source, uint32_t uiInitialBufferSize,
                                         uint32_t uiReadSize)
  :m_in(source),
    m_buffer(new uint8_t[uiInitialBufferSize], uiInitialBufferSize),
    m_uiReadSize(uiReadSize),
    m_uiCurrentPos(0),
    m_bNeedMoreData(true),
    m_bEof(false)
{

}

BufferedMediaReader::BufferedMediaReader(std::istream& source,
                                         std::unique_ptr<MediaStreamParser> pMediaStreamParser,
                                         uint32_t uiInitialBufferSize,
                                         uint32_t uiReadSize)
  :m_in(source),
    m_pMediaStreamParser(std::move(pMediaStreamParser)),
    m_buffer(new uint8_t[uiInitialBufferSize], uiInitialBufferSize),
    m_uiReadSize(uiReadSize),
    m_uiCurrentPos(0),
    m_bNeedMoreData(true),
    m_bEof(false)
{

}

void BufferedMediaReader::setMediaStreamParser(std::unique_ptr<MediaStreamParser> pMediaStreamParser)
{
  m_pMediaStreamParser = std::move(pMediaStreamParser);
}

bool BufferedMediaReader::eof() const
{
  // check validity of media stream parser
  if (!m_pMediaStreamParser) return true;
  // end of file can be signaled if the input stream is not good
  if (!m_in.good()) return true;
  // or if the media stream parser has discovered the eof
  return m_bEof;
}

boost::optional<MediaSample> BufferedMediaReader::readAndExtractMedia()
{
  boost::optional<MediaSample> pMediaSample;
  while ((!pMediaSample) && !eof())
  {
    readMoreDataIfNecessary();
    int32_t iSize = 0;
    // NOTE: iSize determines whether any bytes were consumed and should be moved up
    pMediaSample = m_pMediaStreamParser->extract(m_buffer.data(), m_uiCurrentPos, iSize, false);
    if (iSize > 0)
    {
      // copy remaining data to front of buffer
      memmove(&m_buffer[0], &m_buffer[iSize], m_uiCurrentPos - iSize);
      // update pointer for new reads
      m_uiCurrentPos = m_uiCurrentPos - iSize;
    }
    else
    {
      m_bEof = (iSize == -1) ? true : false;
      m_bNeedMoreData = (iSize == 0) ? true : false;
    }
  }
  return pMediaSample;
}

void BufferedMediaReader::readMoreDataIfNecessary()
{
  if (m_bNeedMoreData)
  {
    // check if there's enough space in the buffer for the read, else resize it
    if (m_buffer.getSize() - m_uiCurrentPos < m_uiReadSize )
    {
      uint32_t uiBufferSize = m_buffer.getSize() * 2;
      uint8_t* pNewBuffer = new uint8_t[uiBufferSize];
      memcpy((char*)pNewBuffer, (char*)m_buffer.data(), m_uiCurrentPos);
      m_buffer.setData(pNewBuffer, uiBufferSize);
    }

    m_in.read((char*) m_buffer.data() + m_uiCurrentPos, m_uiReadSize);
    std::streamsize uiRead = m_in.gcount();
    m_uiCurrentPos += uiRead;
  }
}

}
}
