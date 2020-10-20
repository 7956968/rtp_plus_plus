#pragma once
#include <cpputil/FileUtil.h>
#include <rtp++/RtpJitterBuffer.h>
#include <rtp++/RtpTime.h>
#include <rtp++/media/BufferedMediaReader.h>
#include <rtp++/media/h264/H264AnnexBStreamParser.h>
#include <rtp++/rfc6184/Rfc6184Packetiser.h>

#define RFC6184_TEST_LOG_LEVEL 10

namespace rtp_plus_plus
{
namespace test
{

using media::MediaSample;

// currently not compiling on MSVC 2010: errors in lambda scoping
// compiling on MSVC 2012: errors in lambda scoping
static void test_Rfc6184(const char* szFile)
{
  std::ifstream in1(szFile, std::ios_base::in | std::ios_base::binary);
  media::BufferedMediaReader mediaReader(in1, std::unique_ptr<media::h264::H264AnnexBStreamParser>(new media::h264::H264AnnexBStreamParser()), 10000, 1024);

  std::vector<MediaSample> mediaSamples;

  // read all samples in file
  while (!mediaReader.eof())
  {
    boost::optional<MediaSample> pMediaSample = mediaReader.readAndExtractMedia();
    if (pMediaSample)
    {
      VLOG(RFC6184_TEST_LOG_LEVEL) << "test_Rfc6184 read media sample: " << pMediaSample->getStartTime() << " size: " << pMediaSample->getPayloadSize();

      // ignore AUDs
      Buffer payload = pMediaSample->getDataBuffer();
      uint32_t type = payload[0] & 0x1F;
      media::h264::NalUnitType eType = static_cast<media::h264::NalUnitType>(type);
      if (eType != media::h264::NUT_ACCESS_UNIT_DELIMITER)
        mediaSamples.push_back(*pMediaSample);
#if 0
      else
        LOG(INFO) << "Skipping AUD";
#endif
#if 0
      DLOG(INFO) << "Sample time: " << pMediaSample->getStartTime();
#endif
    }
#if 1
    // DEBUGGING: break after first FU
    if (mediaSamples.size() == 20 ) break;
#endif
  }
  VLOG(RFC6184_TEST_LOG_LEVEL) << "Read: " << mediaSamples.size() << " media samples";

  std::unique_ptr<rfc6184::Rfc6184Packetiser> pPacketiser = std::unique_ptr<rfc6184::Rfc6184Packetiser>(new rfc6184::Rfc6184Packetiser());
  pPacketiser->setPacketizationMode(rfc6184::Rfc6184Packetiser::NON_INTERLEAVED_MODE);

  std::vector<RtpPacket> rtpPackets;
  uint32_t uiSequenceNumber = 0;

  for (MediaSample& mediaSample : mediaSamples)
  {
    VLOG(RFC6184_TEST_LOG_LEVEL) << "Packetising media sample of size: " << mediaSample.getPayloadSize();
    std::ostringstream ostr;
    ostr << "Sample size: " << mediaSample.getPayloadSize() << "->RTP [";
    std::vector<RtpPacket> packets = pPacketiser->packetise(mediaSample);
    VLOG(RFC6184_TEST_LOG_LEVEL) << "Packetised media sample of size: " << mediaSample.getPayloadSize() << " into " << packets.size() << " samples";
#if 1
    for (RtpPacket& rtpPacket : packets)
    {
#if 1
      // set sequence number: this is required for insertion into the RTP playout buffer
      VLOG(RFC6184_TEST_LOG_LEVEL) << "RTP SN: " << uiSequenceNumber;
      rtpPacket.setExtendedSequenceNumber(uiSequenceNumber++);
      rtpPacket.getHeader().setRtpTimestamp(RtpTime::convertToRtpTimestamp(mediaSample.getStartTime(), 90000, 0));
      rtpPackets.push_back(rtpPacket);
      /// for debugging
      Buffer data = rtpPacket.getPayload();
      IBitStream in(data);
      uint32_t forbidden = 0, nri = 0, type = 0;
      bool res = in.read(forbidden, 1);
      assert(res);
      res = in.read(nri, 2);
      assert(res);
      res = in.read(type, 5 );
      assert(res);
      media::h264::NalUnitType eType = static_cast<media::h264::NalUnitType>(type);
      ostr << "(" << forbidden << ", " << nri << ", " << media::h264::toString(eType);
      VLOG(RFC6184_TEST_LOG_LEVEL) << "(" << forbidden << ", " << nri << ", " << media::h264::toString(eType);
      if (eType == media::h264::NUT_FU_A)
      {
         uint32_t fu_start = 0, fu_end = 0, fu_reserved = 0, fu_type = 0;
         in.read(fu_start,    1);
         in.read(fu_end,      1);
         in.read(fu_reserved, 1);
         in.read(fu_type,     5);
         ostr << "=[" << fu_start << "|" << fu_end << "|" << fu_reserved << "|" << media::h264::toString((media::h264::NalUnitType)fu_type) << "]";
      }
      ostr << ")";
      VLOG(RFC6184_TEST_LOG_LEVEL) << "Done RTP SN: " << uiSequenceNumber;
      /// End debugging
#else
      // RG: only storing FU for debugging
      // set sequence number: this is required for insertion into the RTP playout buffer
      rtpPacket.setExtendedSequenceNumber(uiSequenceNumber++);
      rtpPacket.getHeader().setRtpTimestamp(convertToRtpTimestamp(pMediaSample->getStartTime(), 90000, 0));
      /// for debugging
      Buffer data = rtpPacket.getPayload();
      IBitStream in(data);
      uint32_t forbidden = 0, nri = 0, type = 0;
      in.read(forbidden, 1);
      in.read(nri, 2);
      in.read(type, 5 );
      rfc6184::NalUnitType eType = static_cast<rfc6184::NalUnitType>(type);
      ostr << "(" << forbidden << ", " << nri << ", " << rfc6184::toString(eType);
      if (eType == rfc6184::NUT_FU_A)
      {
         rtpPackets.push_back(rtpPacket);
         uint32_t fu_start = 0, fu_end = 0, fu_reserved = 0, fu_type = 0;
         in.read(fu_start,    1);
         in.read(fu_end,      1);
         in.read(fu_reserved, 1);
         in.read(fu_type,     5);
         ostr << "=[" << fu_start << "|" << fu_end << "|" << fu_reserved << "|" << media::h264::toString((media::h264::NalUnitType)fu_type) << "]";
      }
      ostr << ")";
      /// End debugging
#endif
    }
#endif
    ostr << "]";
    VLOG(5) << ostr.str();
    rtpPackets.insert(rtpPackets.end(), packets.begin(), packets.end());
  }
  VLOG(5) << "Packetised " << rtpPackets.size() << " RTP packets";

  RtpJitterBuffer playoutBuffer;
  uint32_t uiSampleCount = 0;
  // add all RTP packets into playout buffer for timestamp grouping
  std::for_each(rtpPackets.begin(), rtpPackets.end(), [&playoutBuffer, &uiSampleCount](RtpPacket& rtpPacket)
  {
    // dummy
    boost::posix_time::ptime tPlayout;
    boost::posix_time::ptime tPresentation;
    bool bRtcpSync = false;
    bool bDup = false;
    uint32_t uiLate = 0;
    if (playoutBuffer.addRtpPacket(rtpPacket, tPresentation, bRtcpSync, tPlayout, uiLate, bDup))
      ++uiSampleCount;
  });

  std::vector<MediaSample> depacketisedMediaSamples;
  for (size_t i = 0; i < uiSampleCount; ++i)
  {
    const RtpPacketGroup nextNode = playoutBuffer.getNextPlayoutBufferNode();
#if 1
    // DEBUGGING
    const std::list<RtpPacket>& rtpPackets = nextNode.getRtpPackets();
    for (std::list<RtpPacket>::const_iterator it = rtpPackets.begin(); it != rtpPackets.end(); ++it)
    {
        std::ostringstream ostr;
        ostr << "SN: " << it->getSequenceNumber();
        IBitStream in(it->getPayload());
        uint32_t forbidden = 0, nri = 0, type = 0;
        in.read(forbidden, 1);
        in.read(nri, 2);
        in.read(type, 5 );
        media::h264::NalUnitType eType = static_cast<media::h264::NalUnitType>(type);
        ostr << " (" << forbidden << ", " << nri << ", " << media::h264::toString(eType);
        if (eType == media::h264::NUT_FU_A)
        {
            uint32_t fu_start = 0, fu_end = 0, fu_reserved = 0, fu_type = 0;
            in.read(fu_start,    1);
            in.read(fu_end,      1);
            in.read(fu_reserved, 1);
            in.read(fu_type,     5);
            ostr << "=[" << fu_start << "|" << fu_end << "|" << fu_reserved << "|" << media::h264::toString((media::h264::NalUnitType)fu_type) << "]";
        }
        ostr << ")";
        VLOG(RFC6184_TEST_LOG_LEVEL) << ostr.str();
    }
    // END DEBUGGING
#endif
    std::vector<MediaSample> samples = pPacketiser->depacketize(nextNode);
    depacketisedMediaSamples.insert(depacketisedMediaSamples.end(), samples.begin(), samples.end());
  }

  VLOG(RFC6184_TEST_LOG_LEVEL) << "Checking sizes: " << mediaSamples.size() << "=" << depacketisedMediaSamples.size() ;
  BOOST_CHECK_EQUAL( mediaSamples.size(), depacketisedMediaSamples.size());
  for (size_t i = 0; i < mediaSamples.size(); ++i)
  {
      Buffer data1 = mediaSamples[i].getDataBuffer();
      Buffer data2  = depacketisedMediaSamples[i].getDataBuffer();
      // compare sizes
      BOOST_CHECK_EQUAL(data1.getSize(), data2.getSize());
      // compare data
      BOOST_CHECK_EQUAL(memcmp(&data1[0], &data2[0], data1.getSize()), 0);
      VLOG(RFC6184_TEST_LOG_LEVEL) << "Checking sample sizes: " << data1.getSize() << "=" << data2.getSize() ;
  }
}

BOOST_AUTO_TEST_SUITE(Rfc4585Test)
BOOST_AUTO_TEST_CASE(test_Rfc6184Packetisation)
{
  VLOG(RFC6184_TEST_LOG_LEVEL) << "test_Rfc6184Packetisation";
  std::unique_ptr<rfc6184::Rfc6184Packetiser> pPacketiser = std::unique_ptr<rfc6184::Rfc6184Packetiser>(new rfc6184::Rfc6184Packetiser());
  pPacketiser->setPacketizationMode(rfc6184::Rfc6184Packetiser::NON_INTERLEAVED_MODE);
  MediaSample largeMediaSample;
  uint8_t* pLargeData = new uint8_t[5000];
  memset(pLargeData, 'A', 5000);
  largeMediaSample.setData(Buffer(pLargeData, 5000));
  std::vector<RtpPacket> vPackets = pPacketiser->packetise(largeMediaSample);
  BOOST_CHECK_EQUAL( vPackets.size(), 4);
}
/**
  * The purpose of this unit test is to make sure that the packetisation
  * and depacketisation processes are correct. To do this, ffmpeg_rtp.264
  * is read from file, packetised, inserted into an RtpPlayoutBuffer to group
  * packets according to TS, and then depacketised
  */
BOOST_AUTO_TEST_CASE(test_Rfc6184NoFu)
{
  VLOG(RFC6184_TEST_LOG_LEVEL) << "test_Rfc6184NoFu";
  test_Rfc6184("data/rtp_aud_no_fu.264");
}

BOOST_AUTO_TEST_CASE(test_Rfc6184WithFu)
{
  VLOG(RFC6184_TEST_LOG_LEVEL) << "test_Rfc6184WithFu";
  test_Rfc6184("data/rtp_aud_fu.264");
}

BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
