#include "CorePch.h"
#include <rtp++/rfc5285/RtpHeaderExtension.h>
#include <cpputil/Utility.h>

#define COMPONENT_LOG_LEVEL 10

namespace rtp_plus_plus
{
namespace rfc5285
{

RtpHeaderExtension::RtpHeaderExtension(uint16_t uiProfileDefined, RtpHeaderExtensionType eType, uint16_t uiExtensionLengthInWords, uint8_t uiAppBits)
  :m_eType(eType),
    m_uiProfileDefined(uiProfileDefined),
    m_uiExtensionLengthInWords(uiExtensionLengthInWords),
    m_uiAppBits(uiAppBits),
    m_uiPadding(0)
{
}

RtpHeaderExtension::~RtpHeaderExtension()
{
}

void RtpHeaderExtension::clear()
{
  m_vHeaderExtensions.clear();
}

bool RtpHeaderExtension::containsExtensions() const { return !empty(); }

bool RtpHeaderExtension::empty() const
{
  return m_vHeaderExtensions.empty();
}

std::size_t RtpHeaderExtension::getHeaderExtensionCount() const { return m_vHeaderExtensions.size(); }

const HeaderExtensionElement& RtpHeaderExtension::getHeaderExtensions(size_t index) const
{
  return m_vHeaderExtensions.at(index);
}

void RtpHeaderExtension::addHeaderExtensions(const HeaderExtensionElement& headerExtension)
{
  m_vHeaderExtensions.push_back(headerExtension);
}

const std::vector<HeaderExtensionElement>& RtpHeaderExtension::getHeaderExtensions() const { return m_vHeaderExtensions; }

uint32_t RtpHeaderExtension::removeHeaderExtension(const uint32_t uiId)
{
  for (auto it = m_vHeaderExtensions.begin(); it != m_vHeaderExtensions.end(); ++it)
  {
    if (it->getId() == uiId)
    {
      m_vHeaderExtensions.erase(it);
      return 1;
    }
  }
  return 0;
}

uint32_t RtpHeaderExtension::getSize() const
{
  if (m_uiExtensionLengthInWords == 0)
    calculateExtensionProperties();

  // if there are no header extensions, then don't count the 4 bytes of the
  // header extension header
  if (m_uiExtensionLengthInWords == 0)
  {
    return 0;
  }
  return rfc3550::MIN_EXTENSION_HEADER_SIZE + (4* m_uiExtensionLengthInWords);
}

void RtpHeaderExtension::calculateExtensionProperties() const
{
  VLOG(COMPONENT_LOG_LEVEL) << "RtpHeaderExtension::calculateExtensionProperties";
  // first check if there are any elements with a length greater than 16
  // if not, we will use the one byte header extension
  uint32_t uiTotalBytes = 0;
  for (HeaderExtensionElement element : m_vHeaderExtensions)
  {
    VLOG(COMPONENT_LOG_LEVEL) << "Element length: " << element.getDataLength();
    uiTotalBytes += element.getDataLength();
    if (element.getDataLength() > 16)
    {
      VLOG(COMPONENT_LOG_LEVEL) << "Using two byte extension header: " << element.getDataLength();
      m_eType = TWO_BYTE_HEADER;
    }
  }

  switch (m_eType)
  {
    case ONE_BYTE_HEADER:
    {
      m_uiProfileDefined = 0xBEDE;
      uiTotalBytes += m_vHeaderExtensions.size();
      break;
    }
    case TWO_BYTE_HEADER:
    {
      m_uiProfileDefined = (0x0100 << 4) | (m_uiAppBits & 0x0F);
      uiTotalBytes += 2 * m_vHeaderExtensions.size();
      break;
    }
    default:
    {
      assert(false);
    }
  }

  VLOG(COMPONENT_LOG_LEVEL) << "Profile defined: " << m_uiProfileDefined << "(" << hex(m_uiProfileDefined) << ") Total bytes: " << uiTotalBytes;

  uint32_t uiRem = uiTotalBytes % 4;
  if ( uiRem == 0)
  {
    m_uiExtensionLengthInWords = uiTotalBytes/4;
  }
  else
  {
    m_uiPadding = 4 - uiRem;
    m_uiExtensionLengthInWords = uiTotalBytes/4 + 1;
  }
}

void RtpHeaderExtension::writeExtensionData(OBitStream& ob) const
{
  // this also sets m_eType
  calculateExtensionProperties();

  ob.write( m_uiProfileDefined, 16);
  ob.write( m_uiExtensionLengthInWords, 16);
  switch (m_eType)
  {
    case ONE_BYTE_HEADER:
    {
      for (size_t i = 0; i < m_vHeaderExtensions.size(); ++i)
      {
        const HeaderExtensionElement& header = m_vHeaderExtensions[i];
        VLOG(COMPONENT_LOG_LEVEL) << "Writing extension element id " << header.getId() << " len: " << header.getDataLength();
        ob.write(header.getId(), 4);
        // NOTE that this fields stores length - 1 in one byte headers
        ob.write(header.getDataLength() - 1, 4);
        const uint8_t* pData = header.getExtensionData();
        ob.writeBytes(pData, header.getDataLength());
      }
      break;
    }
    case TWO_BYTE_HEADER:
    {
      for (size_t i = 0; i < m_vHeaderExtensions.size(); ++i)
      {
        const HeaderExtensionElement& header = m_vHeaderExtensions[i];
        VLOG(COMPONENT_LOG_LEVEL) << "Writing extension element id " << header.getId() << " len: " << header.getDataLength();
        ob.write(header.getId(), 8);
        // NOTE that this fields stores length - 1 in one byte headers
        ob.write(header.getDataLength(), 8);
        const uint8_t* pData = header.getExtensionData();
        ob.writeBytes(pData, header.getDataLength());
      }
      break;
    }
  }
  // padding
  for (size_t i = 0; i < m_uiPadding; ++i)
  {
    ob.write(0, 8);
  }
}

void RtpHeaderExtension::readExtensionData(IBitStream& ib)
{
  bool res = ib.read(m_uiProfileDefined, 16);
  assert(res);
  res = ib.read(m_uiExtensionLengthInWords, 16);
  assert(res);

  VLOG(COMPONENT_LOG_LEVEL) << "read profile defined " << m_uiProfileDefined << "(" << hex(m_uiProfileDefined) << ") length in words: " << m_uiExtensionLengthInWords;
  if (m_uiProfileDefined == PROFILE_ONE_BYTE_HEADER)
  {
    int32_t iTotalBytesToRead = m_uiExtensionLengthInWords * 4;
    while (iTotalBytesToRead > 0)
    {
      uint32_t uiId = 0;
      uint32_t uiLength = 0;
      unsigned char data[HeaderExtensionElement::MAX_HDR_EXT_SIZE];
      res = ib.read(uiId, 4);
      assert(res);
      res = ib.read(uiLength, 4);
      assert(res);
      --iTotalBytesToRead;
      if ( uiId == 15)
      {
        // invalid according to rfc5285: skip rest of headers
        LOG(WARNING) << " Invalid id for one byte extension header: " << uiId << " skipping rest of extension header: " << iTotalBytesToRead << " bytes";
        res = ib.skipBytes(iTotalBytesToRead);
        assert(res);
        break;
      }
      else if (uiId == 0 && uiLength == 0)
      {
        // this is a padding byte which we should just ignore
        continue;
      }

      VLOG(COMPONENT_LOG_LEVEL) << "Read extension element id " << uiId << " len: " << uiLength + 1;
      // sanity check: one byte header extension length = len field + 1
      if (uiLength + 1 > (uint32_t) iTotalBytesToRead)
      {
        LOG(WARNING) << " Error parsing one byte extension header: Length " << uiLength << " larger than bytes remaining " << iTotalBytesToRead << " skipping rest of extension header: " << iTotalBytesToRead << " bytes";
        res = ib.skipBytes(iTotalBytesToRead);
        assert(res);
        break;
      }
      uint8_t* pData = (uint8_t*)data;
      res = ib.readBytes(pData, uiLength + 1);
      assert(res);

      HeaderExtensionElement headerExtension(uiId, uiLength + 1, data);
      m_vHeaderExtensions.push_back(headerExtension);

      iTotalBytesToRead -= (uiLength + 1);
    }
  }
  else if ((m_uiProfileDefined >> 4) == 0x100)
  {
    int32_t iTotalBytesToRead = m_uiExtensionLengthInWords * 4;
    VLOG(COMPONENT_LOG_LEVEL) << "Parsing two byte header extensions: Total bytes: " << iTotalBytesToRead;
    while (iTotalBytesToRead > 0)
    {
      VLOG(COMPONENT_LOG_LEVEL) << "bytes left: " << iTotalBytesToRead;
      uint32_t uiId = 0;
      uint32_t uiLength = 0;
      unsigned char data[HeaderExtensionElement::MAX_HDR_EXT_SIZE];
      res = ib.read(uiId, 8);
      assert(res);
      --iTotalBytesToRead;
      if (iTotalBytesToRead == 0)
      {
        if (uiId != 0)
        {
          LOG(WARNING) << "Malformed extension, this should be a padding byte but has value " << uiId;
        }
        continue;
      }
      res = ib.read(uiLength, 8);
      assert(res);
      --iTotalBytesToRead;
      if (uiId == 0 && uiLength == 0)
      {
        // this is a padding byte which we should just ignore
        continue;
      }

      VLOG(COMPONENT_LOG_LEVEL) << "Read extension element id " << uiId << " len: " << uiLength;
      if (uiLength > (uint32_t) iTotalBytesToRead)
      {
        LOG(WARNING) << " Error parsing two byte extension header: Length " << uiLength << " larger than bytes remaining " << iTotalBytesToRead << " skipping rest of extension header: " << iTotalBytesToRead << " bytes";
        res = ib.skipBytes(iTotalBytesToRead);
        assert(res);
        break;
      }
      uint8_t* pData = (uint8_t*)data;
      res = ib.readBytes(pData, uiLength);
      assert(res);

      HeaderExtensionElement headerExtension(uiId, uiLength, data);
      m_vHeaderExtensions.push_back(headerExtension);

      iTotalBytesToRead -= uiLength;
    }
  }
  else
  {
    // for now we'll just skip the header
    LOG(WARNING) << " Unsupported header extension profile: " << m_uiProfileDefined << "(" << hex(m_uiProfileDefined) << ") skipping extension header: " << m_uiExtensionLengthInWords << " words";
    res = ib.skipBytes(m_uiExtensionLengthInWords * 4);
    assert(res);
  }
}

} // rfc5285
} // rtp_plus_plus
