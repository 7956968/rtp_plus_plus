#pragma once
#include <memory>
#include <vector>
#include <boost/foreach.hpp>
#include <cpputil/IBitStream.h>
#include <cpputil/OBitStream.h>
#include <rtp++/RtpPacketGroup.h>
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{

/**
  * This base class makes it possible to send any data in RTP
  * without worrying about the payload specific packetization
  * It implements simple one to one packetization i.e. each media
  * sample creates an RTP packet and vice versa.
  * There is no support for aggregation or fragmentation.
  * Payload specific packetizers can be implemented and created
  * in the RtpSession
  */
class PayloadPacketiserBase
{
public:
  typedef std::unique_ptr<PayloadPacketiserBase> ptr;

  static std::unique_ptr<PayloadPacketiserBase> create();
  virtual ~PayloadPacketiserBase();

  /**
    * @brief returns info about the last packetisation if the subclass has implemented it
    * @return A vector containing the packet offsets for each media sample in the access unit
    */
  std::vector<std::vector<uint32_t> > getPacketisationInfo() const { return m_vLastPacketisationInfo; }

  // not every call to packetise will result in a packet list
  // some calls will results in no rtp packets e.g. if the profile supports
  // interleaving
  virtual std::vector<RtpPacket> packetise(const media::MediaSample& mediaSample);

  /**
   * @brief packetise should be called for an access unit or media samples with the same presentation time.
   * @param vMediaSamples
   * @return
   */
  virtual std::vector<RtpPacket> packetise(const std::vector<media::MediaSample>& vMediaSamples);

  /**
   * @brief Depacketises an RTP packet group and returns a vector of media samples.
   * The rtpPacketGroup parameter should contain media data relating to one presentation time.
   * @param rtpPacketGroup The RTP data that needs to be depacketised into media samples.
   * @return A vector of media samples that were extracted from rtpPacketGroup
   */
  virtual std::vector<media::MediaSample> depacketize(const RtpPacketGroup& rtpPacketGroup);

protected:
  // for debugging/simulations
  std::vector<std::vector<uint32_t> > m_vLastPacketisationInfo;
};

}

