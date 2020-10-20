#pragma once
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <cpputil/OBitStream.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc5285/HeaderExtensionElement.h>
#include <rtp++/rfc5285/Rfc5285.h>

/* RFC3550 */
/**
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      defined by profile       |           length              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        header extension                       |
|                             ....                              |

If the X bit in the RTP header is one, a variable-length header
extension MUST be appended to the RTP header, following the CSRC list
if present.  The header extension contains a 16-bit length field that
counts the number of 32-bit words in the extension, excluding the
four-octet extension header (therefore zero is a valid length).  Only
a single extension can be appended to the RTP data header.  To allow
multiple interoperating implementations to each experiment
independently with different header extensions, or to allow a
particular implementation to experiment with more than one type of
header extension, the first 16 bits of the header extension are left
open for distinguishing identifiers or parameters.  The format of
these 16 bits is to be defined by the profile specification under
which the implementations are operating.  This RTP specification does
not define any header extensions itself. 
*/

const uint16_t PROFILE_ONE_BYTE_HEADER = 0xBEDE;
const uint16_t DEFAULT_PROFILE_DEFINED = PROFILE_ONE_BYTE_HEADER;

namespace rtp_plus_plus
{
namespace rfc5285
{

/**
 * @brief The RtpHeaderExtension class eases the process of adding header extensions to an RTP header.
 * On packetisation the implementation selects the one or two byte header extension depending
 * on the size of the extension elements i.e. if all extension elements are <= 16, then the one byte
 * header extension is used.
 */
class RtpHeaderExtension
{
  friend OBitStream& operator<< (OBitStream& stream, const RtpHeaderExtension& extension);
public:
  /**
   * @brief RtpHeaderExtension Constructor
   * @param uiProfileDefined
   * @param eType
   * @param uiExtensionLengthInWords
   */
  RtpHeaderExtension(uint16_t uiProfileDefined = DEFAULT_PROFILE_DEFINED, RtpHeaderExtensionType eType = ONE_BYTE_HEADER, uint16_t uiExtensionLengthInWords = 0, uint8_t uiAppBits = 0);
  /**
   * @brief RtpHeaderExtension Destructor
   */
  ~RtpHeaderExtension();
  /**
   * @brief clear
   */
  void clear();
  /**
   * @brief containsExtensions
   * @return
   */
  bool containsExtensions() const;
  /**
   * @brief empty
   * @return
   */
  bool empty() const;
  /**
   * @brief getHeaderExtensionCount
   * @return
   */
  std::size_t getHeaderExtensionCount() const;
  /**
   * @brief addHeaderExtensions
   * @param headerExtension
   * @return
   */
  void addHeaderExtensions(const HeaderExtensionElement& headerExtension);
  /**
  * @brief getHeaderExtensions
  * @param index
  * @return
  */
 const HeaderExtensionElement& getHeaderExtensions(size_t index) const;
  /**
   * @brief getHeaderExtensions
   * @return
   */
 const std::vector<HeaderExtensionElement>& getHeaderExtensions() const;
 /**
  * @brief removeHeaderExtensions removes extension elements with specified id
  * @param uiId The id of the extension elements to be removed
  * @return 1 if  the extension header element was removed, 0 otherwise
  */
 uint32_t removeHeaderExtension(const uint32_t uiId);
 /**
  * @brief getSize returns the length of the entire extension header in bytes
  * @return
  */
  uint32_t getSize() const;
  /**
   * @brief getProfileDefined
   * @return
   */
  uint16_t getProfileDefined() const { return m_uiProfileDefined; }
  /**
   * @brief setProfileDefined
   * @param uiProfileDefined
   */
  void setProfileDefined(uint16_t uiProfileDefined) { m_uiProfileDefined = uiProfileDefined; }
  /**
   * @brief getExtensionLengthInWords
   * @return the number of 32 bit words in the extension payload
   */
  uint16_t getExtensionLengthInWords() const { return m_uiExtensionLengthInWords; }
  /**
   * @brief calculateExtensionProperties determines whether to use one or two byte extension,
   * profile, extension length in words
   */
  void calculateExtensionProperties() const;
  /**
   * @brief writeExtensionData
   * @param ob
   */
  void writeExtensionData(OBitStream& ob) const;
  /**
   * @brief readExtensionData
   * @param ib
   */
  void readExtensionData(IBitStream& ib);
protected:

  mutable RtpHeaderExtensionType m_eType;
  mutable uint16_t m_uiProfileDefined;
  uint8_t m_uiAppBits;
  mutable uint16_t m_uiExtensionLengthInWords; // excludes the standard 4 octet header
  mutable uint32_t m_uiPadding;

  std::vector<HeaderExtensionElement> m_vHeaderExtensions;
};

OBitStream& operator<< (OBitStream& ostr, const RtpHeaderExtension& extension);

} // rfc5285
} // rtp_plus_plus
