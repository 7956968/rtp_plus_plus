#pragma once
#include <fstream>
#include <boost/filesystem.hpp>
#include <rtp++/media/AsyncStreamMediaSource.h>
#include <rtp++/media/BufferedMediaReader.h>
#include <rtp++/media/h264/H264AnnexBStreamParser.h>
#include <rtp++/media/StreamMediaSource.h>
#include <rtp++/media/VirtualVideoDeviceV2.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/application/ApplicationUtil.h>

#define MEDIA_TEST_LOG_LEVEL 5
#define MEDIA_TEST_LOG_LEVEL_VERBOSE 5

namespace bfs = boost::filesystem;

namespace rtp_plus_plus
{
namespace test {

using media::MediaSample;

static uint32_t g_uiAccessUnitIndex = 1;

static boost::system::error_code handleVideoSample(const MediaSample& mediaSample, std::ofstream& out)
{
  media::h264::H264AnnexBStreamWriter::writeMediaSampleNaluToStream(mediaSample, out);
  return boost::system::error_code();
}

static boost::system::error_code handleVideoAccessUnit(const std::vector<MediaSample>& mediaSamples, std::ofstream& out)
{
  std::ostringstream ostr;
  ostr << LOG_MODIFY_WITH_CARE
       << " AU #: " << g_uiAccessUnitIndex++;

  for (const MediaSample& mediaSample: mediaSamples)
  {
    media::h264::H264AnnexBStreamWriter::writeMediaSampleNaluToStream(mediaSample, out);

    media::h264::NalUnitType eType = media::h264::getNalUnitType(mediaSample);
    ostr << " NUT: " << eType << " (" << mediaSample.getPayloadSize() << ") ";
  }

  VLOG(MEDIA_TEST_LOG_LEVEL_VERBOSE) << ostr.str();
  return boost::system::error_code();
}

static void test_H264AnnexBStreamParser(const char* szFile)
{
  VLOG(MEDIA_TEST_LOG_LEVEL) << "test_H264AnnexBStreamParser: " << szFile;

  std::ifstream in1(szFile, std::ios_base::in | std::ios_base::binary);
  media::BufferedMediaReader mediaReader(in1, std::unique_ptr<media::h264::H264AnnexBStreamParser>(new media::h264::H264AnnexBStreamParser()), 10000, 1024);

  std::vector<MediaSample> mediaSamples;

  // read all samples in file
  while (!mediaReader.eof())
  {
    boost::optional<MediaSample> pMediaSample = mediaReader.readAndExtractMedia();
    if (pMediaSample)
    {
      mediaSamples.push_back(*pMediaSample);
    }
  }
  VLOG(MEDIA_TEST_LOG_LEVEL) << "Read: " << mediaSamples.size() << " media samples";
}

static void testMediaSource(const std::string& sMediaSource,
                            const std::string& sMediaSink,
                            media::StreamMediaSource::Mode eMode)
{
  VLOG(MEDIA_TEST_LOG_LEVEL) << "testMediaSource: " << sMediaSource << " " << sMediaSink;

  std::ifstream in1;
  bool bRes = app::ApplicationUtil::initialiseFileMediaSource(in1, sMediaSource);
  BOOST_CHECK_EQUAL( bRes, true);

  // Test media source in NAL unit mode
  media::StreamMediaSource mediaSource(in1, rfc6184::H264, 10000,
                                       false, 0,
                                       25, false,
                                       eMode);

  std::ofstream out1(sMediaSink.c_str(), std::ios_base::out | std::ios_base::binary);
  switch (eMode)
  {
    case media::StreamMediaSource::SingleMediaSample:
    {
      mediaSource.setReceiveMediaCB(boost::bind(&handleVideoSample, _1, boost::ref(out1)));
      break;
    }
    case media::StreamMediaSource::AccessUnit:
    {
      mediaSource.setReceiveAccessUnitCB(boost::bind(&handleVideoAccessUnit, _1, boost::ref(out1)));
      break;
    }
  }

  mediaSource.start();
  out1.close();
  BOOST_CHECK_EQUAL(bfs::file_size(sMediaSource), bfs::file_size(sMediaSink));
}

static void testAsyncMediaSource(const std::string& sMediaSource,
                                 const std::string& sMediaSink)
{
  VLOG(MEDIA_TEST_LOG_LEVEL) << "test_AsyncMediaSource: " << sMediaSource << " " << sMediaSink;

  std::ifstream in1;
  bool bRes = app::ApplicationUtil::initialiseFileMediaSource(in1, sMediaSource);
  BOOST_CHECK_EQUAL( bRes, true);

  boost::asio::io_service ioService;
  // Test media source in NAL unit mode
  media::AsyncStreamMediaSource mediaSource(ioService, in1, rfc6184::H264, 10000,
                                       false, 0,
                                       25, false);

  VLOG(2) << "AsyncStreamMediaSource: Starting test";
  std::ofstream out1(sMediaSink.c_str(), std::ios_base::out | std::ios_base::binary);
  mediaSource.setReceiveAccessUnitCB(boost::bind(&handleVideoAccessUnit, _1, boost::ref(out1)));

  mediaSource.start();
  VLOG(2) << "AsyncStreamMediaSource: Starting io_service";
  ioService.run();
  VLOG(2) << "AsyncStreamMediaSource: Done";

  out1.close();
  BOOST_CHECK_EQUAL(bfs::file_size(sMediaSource), bfs::file_size(sMediaSink));
}

/// This test checks if the looping works correctly
static void testAsyncMediaSourceLoopOnce(const std::string& sMediaSource,
                                 const std::string& sMediaSink)
{
  VLOG(MEDIA_TEST_LOG_LEVEL) << "testAsyncMediaSourceLoopOnce: " << sMediaSource << " " << sMediaSink;
  std::ifstream in1;
  bool bRes = app::ApplicationUtil::initialiseFileMediaSource(in1, sMediaSource);
  BOOST_CHECK_EQUAL( bRes, true);

  boost::asio::io_service ioService;
  // Test media source in NAL unit mode
  media::AsyncStreamMediaSource mediaSource(ioService, in1, rfc6184::H264, 10000,
                                       true, 2,
                                       25, false);

  VLOG(2) << "AsyncStreamMediaSource: Starting test";
  std::ofstream out1(sMediaSink.c_str(), std::ios_base::out | std::ios_base::binary);
  mediaSource.setReceiveAccessUnitCB(boost::bind(&handleVideoAccessUnit, _1, boost::ref(out1)));

  mediaSource.start();
  VLOG(2) << "AsyncStreamMediaSource: Starting io_service";
  ioService.run();
  VLOG(2) << "AsyncStreamMediaSource: Done: 2*source: " << bfs::file_size(sMediaSource)
          << " sink: " << bfs::file_size(sMediaSink);

  out1.close();
  BOOST_CHECK_EQUAL(2 * bfs::file_size(sMediaSource), bfs::file_size(sMediaSink));
}

static void testVirtualVideoDeviceV2(const std::string& sMediaSource,
                                 const std::string& sMediaSink)
{
  VLOG(MEDIA_TEST_LOG_LEVEL) << "testVirtualVideoDeviceV2: " << sMediaSource << " " << sMediaSink;

  boost::asio::io_service ioService;
  // Test media source in NAL unit mode
  media::VirtualVideoDeviceV2 mediaSource(ioService, sMediaSource, rfc6184::H264, 25, false, false);

  VLOG(2) << "VirtualVideoDeviceV2: Starting test";
  std::ofstream out1(sMediaSink.c_str(), std::ios_base::out | std::ios_base::binary);
  mediaSource.setReceiveAccessUnitCB(boost::bind(&handleVideoAccessUnit, _1, boost::ref(out1)));

  mediaSource.start();
  VLOG(2) << "VirtualVideoDeviceV2: Starting io_service";
  ioService.run();
  VLOG(2) << "VirtualVideoDeviceV2: Done";

  out1.close();
  BOOST_CHECK_EQUAL(bfs::file_size(sMediaSource), bfs::file_size(sMediaSink));
}

BOOST_AUTO_TEST_SUITE(MediaSource)
BOOST_AUTO_TEST_CASE( tc_test_MediaSource )
{
  VLOG(MEDIA_TEST_LOG_LEVEL) << "test_MediaSource";
  testMediaSource("data/rtp_aud_fu.264", "data/test_MediaSource.264", media::StreamMediaSource::SingleMediaSample);
  testMediaSource("data/rtp_aud_fu.264", "data/test_MediaSource_AU.264", media::StreamMediaSource::AccessUnit);
}

BOOST_AUTO_TEST_CASE( tc_test_AsyncMediaSource )
{
  VLOG(MEDIA_TEST_LOG_LEVEL) << "test_AsyncMediaSource";
  testAsyncMediaSource("data/352x288p30_KristenAndSara.264", "data/test_AsyncMediaSource.264");

  g_uiAccessUnitIndex = 1;
  testAsyncMediaSourceLoopOnce("data/352x288p30_KristenAndSara.264", "data/test_AsyncMediaSourceLoopOnce.264");
}

BOOST_AUTO_TEST_CASE( tc_test_VirtualVideoDeviceSource )
{
  VLOG(MEDIA_TEST_LOG_LEVEL) << "test_AsyncMediaSource";
  testVirtualVideoDeviceV2("data/352x288p30_KristenAndSara.264", "data/test_VirtualVideoDeviceV2.264");
}

BOOST_AUTO_TEST_CASE( tc_test_H264AnnexBStreamParser_no_FU)
{
  test_H264AnnexBStreamParser("data/rtp_aud_no_fu.264");
}

BOOST_AUTO_TEST_CASE( tc_test_H264AnnexBStreamParser_FU)
{
  test_H264AnnexBStreamParser("data/rtp_aud_fu.264");
}

BOOST_AUTO_TEST_CASE( tc_test_H264AnnexBStreamParser_FFMPEF)
{
  test_H264AnnexBStreamParser("data/ffmpeg_rtp.264");
}

BOOST_AUTO_TEST_SUITE_END()

}// test
} // rtp_plus_plus
