#pragma once
#include <cstdint>

namespace rtp_plus_plus
{
namespace rfc5285
{

/**
 * @brief The HeaderExtensionElement class
 */
class HeaderExtensionElement
{
public:
  static const int MAX_HDR_EXT_SIZE = 256;
  /**
   * @brief HeaderExtensionElement
   * @param uiId
   * @param uiDataLength
   * @param pData
   */
  HeaderExtensionElement(uint32_t uiId, uint32_t uiDataLength, const uint8_t* pData )
    :m_uiId(uiId),
      m_uiLength(uiDataLength)
  {
    assert(pData);
    assert(uiDataLength < MAX_HDR_EXT_SIZE - 1);
    memcpy(m_data, pData, uiDataLength);
    m_data[uiDataLength + 1] = 0;
  }
  /**
   * @brief getId getter for Id.
   * @return
   */
  uint32_t getId() const { return m_uiId; }
  /**
   * @brief This returns the length of the contained data (and not length -1 which will be read from/written to stream)
   */
  uint32_t getDataLength() const { return m_uiLength; }
  /**
   * @brief getExtensionData Getter for extension data blob
   * @return
   */
  const uint8_t* getExtensionData() const { return m_data; }
  /**
   * @brief getExtensionData Getter for extension data blob
   * @return
   */
  uint8_t* getExtensionData() { return m_data; }

private:
  uint32_t m_uiId;
  uint32_t m_uiLength;
  unsigned char m_data[MAX_HDR_EXT_SIZE];
};

} // rfc5285
} // rtp_plus_plus
