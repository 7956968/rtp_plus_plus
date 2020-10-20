#pragma once
#include <ostream>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <cpputil/Buffer.h>
#include <cpputil/OBitStream.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc5285/RtpHeaderExtension.h>


/**
  RFC3550: RTP header implementation
  Having multiple extension headers according to RFC5285 still needs to be implemented

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|  CC   |M|     PT      |       sequence number         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           synchronization source (SSRC) identifier            |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|            contributing source (CSRC) identifiers             |
|                             ....                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

namespace rtp_plus_plus
{
namespace rfc3550
{

/**
 * @brief The RtpHeader class
 */
class RtpHeader
{
  friend std::ostream& operator<< (std::ostream& stream, const RtpHeader& rtpHeader);
  friend OBitStream& operator<< (OBitStream& stream, const RtpHeader& rtpHeader);

public:
  typedef boost::shared_ptr<RtpHeader> ptr;
  /**
   * @brief RtpHeader
   */
  RtpHeader()
    :m_uiVersion(RTP_VERSION_NUMBER),
    m_bPadding(false),
    m_uiCC(0),
    m_bMarker(false),
    m_uiPT(0),
    m_uiSN(0),
    m_uiRtpTimestamp(0),
    m_uiSSRC(0)
  {

  }
  /**
   * @brief RtpHeader
   * @param uiPT
   * @param uiSN
   * @param uiRtpTS
   * @param uiSSRC
   */
  RtpHeader(uint8_t uiPT, uint16_t uiSN, uint32_t uiRtpTS, uint32_t uiSSRC)
    :m_uiVersion(RTP_VERSION_NUMBER),
    m_bPadding(false),
    m_uiCC(0),
    m_bMarker(false),
    m_uiPT(uiPT),
    m_uiSN(uiSN),
    m_uiRtpTimestamp(uiRtpTS),
    m_uiSSRC(uiSSRC)
  {

  }
  /**
   * @brief RtpHeader
   * @param bPadding
   * @param bMarker
   * @param uiPT
   * @param uiSN
   * @param uiRtpTS
   * @param uiSSRC
   */
  RtpHeader(bool bPadding, bool bMarker, uint8_t uiPT, uint16_t uiSN, uint32_t uiRtpTS, uint32_t uiSSRC)
    :m_uiVersion(RTP_VERSION_NUMBER),
    m_bPadding(bPadding),
    m_uiCC(0),
    m_bMarker(bMarker),
    m_uiPT(uiPT),
    m_uiSN(uiSN),
    m_uiRtpTimestamp(uiRtpTS),
    m_uiSSRC(uiSSRC)
  {

  }
  /**
   * @brief getVersion
   * @return
   */
  uint8_t getVersion() const { return m_uiVersion; }
  bool hasPadding() const { return m_bPadding; }
  bool hasExtension() const { return m_headerExtension.containsExtensions(); }
  uint8_t getCC() const { return m_uiCC; } 
  bool isMarkerSet() const { return m_bMarker; }
  uint8_t getPayloadType() const { return m_uiPT; }
  uint16_t getSequenceNumber() const { return m_uiSN; }
  uint32_t getRtpTimestamp() const { return m_uiRtpTimestamp; }
  uint32_t getSSRC() const { return m_uiSSRC; }
  std::vector<uint32_t> getCSRCs() const { return m_vCCs; }

  std::size_t getHeaderExtensionCount() const { return m_headerExtension.getHeaderExtensionCount(); }
  const rfc5285::RtpHeaderExtension& getHeaderExtension() const { return m_headerExtension; }
  rfc5285::RtpHeaderExtension& getHeaderExtension(){ return m_headerExtension; }
  std::vector<rfc5285::HeaderExtensionElement> getHeaderExtensions() const { return m_headerExtension.getHeaderExtensions(); }

  void setPadding(bool bVal) { m_bPadding = bVal; }
  void addRtpHeaderExtension(const rfc5285::HeaderExtensionElement& headerExtension)
  {
    m_headerExtension.addHeaderExtensions(headerExtension);
  }
  void setMarkerBit(bool bVal) { m_bMarker = bVal; }
  void setPayloadType(uint8_t uiPt) { m_uiPT = uiPt; }
  void setSequenceNumber(uint16_t uiSN) { m_uiSN = uiSN; }
  void setRtpTimestamp(uint32_t uiRtpTimestamp) {m_uiRtpTimestamp = uiRtpTimestamp; }
  void setSSRC(uint32_t uiSSRC) { m_uiSSRC = uiSSRC; }
  void addContributingSource(uint32_t uiCSRC) { m_vCCs.push_back(uiCSRC); }

  uint32_t getSize() const
  {
    return MIN_RTP_HEADER_SIZE +
           (4 * m_vCCs.size()) + 
           (m_headerExtension.containsExtensions() ? m_headerExtension.getSize() : 0);
  }

protected:

  uint8_t m_uiVersion;
  bool m_bPadding;
  bool m_bExtension;
  uint8_t m_uiCC; 
  bool m_bMarker;
  uint8_t m_uiPT;
  uint16_t m_uiSN; 
  uint32_t m_uiRtpTimestamp;
  /**
  SSRC: 32 bits
  
  The SSRC field identifies the synchronization source.  This
  identifier SHOULD be chosen randomly, with the intent that no two
  synchronization sources within the same RTP session will have the
  same SSRC identifier.  An example algorithm for generating a
  random identifier is presented in Appendix A.6.  Although the
  probability of multiple sources choosing the same identifier is
  low, all RTP implementations must be prepared to detect and
  resolve collisions.  Section 8 describes the probability of
  collision along with a mechanism for resolving collisions and
  detecting RTP-level forwarding loops based on the uniqueness of
  the SSRC identifier.  If a source changes its source transport
  address, it must also choose a new SSRC identifier to avoid being
  interpreted as a looped source (see Section 8.2).
  */
  uint32_t m_uiSSRC;

  std::vector<uint32_t> m_vCCs;

  /**
   * @brief m_vHeaderExtensions RFC5285 header extensions
   */
  rfc5285::RtpHeaderExtension m_headerExtension;
};


std::ostream& operator<< (std::ostream& ostr, const RtpHeader& rtpHeader);
OBitStream& operator<< (OBitStream& ostr, const RtpHeader& rtpHeader);

} // rfc3550
} // rtp_plus_plus
