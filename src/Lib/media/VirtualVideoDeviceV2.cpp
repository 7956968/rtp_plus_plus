#include "CorePch.h"
#include <rtp++/media/VirtualVideoDeviceV2.h>
#include <functional>
#include <boost/asio/io_service.hpp>
#include <cpputil/MakeService.h>
#include <rtp++/application/ApplicationUtil.h>
#include <rtp++/media/h264/H264AnnexBStreamWriter.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>
#include <rtp++/media/IVideoCodecTransform.h>
#include <rtp++/media/MediaTypes.h>
#include <rtp++/media/NalUnitMediaSource.h>
#include <rtp++/media/YuvMediaSource.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/rfc6190/Rfc6190.h>
#include <rtp++/rfchevc/Rfchevc.h>
#include <rtp++/util/Base64.h>

#ifdef ENABLE_OPEN_H264
#include "OpenH264Codec.h"
#endif
#ifdef ENABLE_VPP
#include "VppH264Codec.h"
#endif
#ifdef ENABLE_X264
#include "X264Codec.h"
#endif
#ifdef ENABLE_X265
#include "X265Codec.h"
#endif

// tests to switch bitrate at some point
// #define TEST_BITRATE_SWITCHING
#ifdef TEST_BITRATE_SWITCHING

const int period = 150; // 1 switch per 5 second
const std::vector<uint32_t> default_bitrates = {500, 100, 400, 1000};

#endif

namespace rtp_plus_plus
{
namespace media
{

std::shared_ptr<VirtualVideoDeviceV2> VirtualVideoDeviceV2::create(boost::asio::io_service& ioService,
                                                                   const std::string& sDeviceName,
                                                                   const std::string& sMediaType,
                                                                   uint32_t uiFps, bool bLimitRate,
                                                                   bool bLoop, uint32_t uiLoopCount,
                                                                   uint32_t uiWidth, uint32_t uiHeight,
                                                                   const std::string& sVideoCodecImpl,
                                                                   const std::vector<std::string>& videoCodecParameters)
{
  if (!FileUtil::fileExists(sDeviceName))
  {
    LOG(WARNING) << "Failed to initialise virtual video device from file: " << sDeviceName;
    return std::shared_ptr<VirtualVideoDeviceV2>();
  }
  else
  {
    return std::make_shared<VirtualVideoDeviceV2>(ioService, sDeviceName, sMediaType, uiFps, bLimitRate, bLoop, uiLoopCount, uiWidth, uiHeight, sVideoCodecImpl, videoCodecParameters);
  }
}

std::shared_ptr<VirtualVideoDeviceV2> VirtualVideoDeviceV2::create(boost::asio::io_service& ioService,
                                                                   const std::string& sDeviceName, const std::string &sMediaType,
                                                                   uint32_t uiFps,
                                                                   PayloadType ePayloadType, const std::string& sPacketSizes,
                                                                   uint8_t uiCharPayload, uint32_t uiTotalVideoSamples)
{
  if (sDeviceName != "gen")
  {
    LOG(WARNING) << "Failed to initialise virtual video device. Invalid device name: " << sDeviceName;
    return std::shared_ptr<VirtualVideoDeviceV2>();
  }
  else
  {
    return std::make_shared<VirtualVideoDeviceV2>(ioService, sDeviceName, sMediaType, uiFps, ePayloadType, sPacketSizes, uiCharPayload, uiTotalVideoSamples);
  }
}

VirtualVideoDeviceV2::VirtualVideoDeviceV2(boost::asio::io_service& ioService,
                                           const std::string& sDeviceName, const std::string &sMediaType,
                                           uint32_t uiFps,
                                           PayloadType ePayloadType, const std::string &sPacketSizes,
                                           uint8_t uiCharPayload, uint32_t uiTotalVideoSamples)
  :IVideoDevice(sDeviceName),
    m_rIoService(ioService),
    m_timer(ioService),
    m_pSource(std::unique_ptr<MediaSource>(new GeneratedMediaSource(ePayloadType, sPacketSizes, uiCharPayload, uiTotalVideoSamples))),
    m_eMode(TM_FRAME_RATE),
    m_uiFrameDurationUs((uiFps != 0) ? 1000000/uiFps : 25),
    m_dCurrentAccessUnitPts(0.0),
    m_dFrameDuration(1.0/uiFps),
    m_bShuttingDown(false),
    m_uiAuCount(0),
    m_pMediaSink(nullptr)
#ifdef TEST_BITRATE_SWITCHING
    ,m_iSwitchFrame(period)
#endif
{

}

VirtualVideoDeviceV2::VirtualVideoDeviceV2(boost::asio::io_service& ioService,
                                           const std::string& sDeviceName, const std::string &sMediaType,
                                           uint32_t uiFps, bool bLimitRate,
                                           bool bLoop, uint32_t uiLoopCount,
                                           uint32_t uiWidth, uint32_t uiHeight,
                                           const std::string& sVideoCodecImpl,
                                           const std::vector<std::string>& videoCodecParameters)
  :IVideoDevice(sDeviceName),
    m_rIoService(ioService),
    m_timer(ioService),
    m_pSource(nullptr),
    m_eMode(TM_FRAME_RATE),
    m_uiFrameDurationUs((uiFps != 0 && bLimitRate) ? 1000000/uiFps : 0),
    m_dCurrentAccessUnitPts(0.0),
    m_dFrameDuration(1.0/uiFps),
    m_bShuttingDown(false),
    m_uiAuCount(0),
    m_pMediaSink(nullptr)
#ifdef TEST_BITRATE_SWITCHING
    ,m_iSwitchFrame(period)
#endif
{
  // FIXME: sMediaType contains RTP session media type. File could have different type when using raw YUV input file
  boost::filesystem::path path(sDeviceName);
  std::string sFileType;
  std::string sExt = path.extension().string();
  if (sExt == ".yuv")
  {
    sFileType = media::YUV420P;
  }
  else if (sExt== ".264")
  {
    // for SVC
    sFileType = sMediaType; // rfc6184::H264;
  }
  else if (sExt== ".265")
  {
    sFileType = rfchevc::H265;
  }
  else if (sExt== ".bin")
  {
    sFileType = sMediaType;
  }
  else
  {
    // default: use RTP media type
    sFileType = sMediaType;
  }

  VLOG(2) << "Using file type: " << sFileType;

  if (sFileType == rfc6184::H264 ||
      sFileType == rfc6190::H264_SVC ||
      sFileType == rfchevc::H265)
  {
    m_pSource = std::unique_ptr<MediaSource>(new NalUnitMediaSource(sDeviceName, sMediaType, bLoop, uiLoopCount));
    bool success = app::ApplicationUtil::initialiseFileMediaSource(m_in1, sDeviceName);
    assert(success);
    // For now we only support extracting the H264 parameter sets
    if (sMediaType == rfc6184::H264 || sMediaType == rfc6190::H264_SVC)
    {
      // TODO: refactor using NalUnitMediaSource
      // there is a better/clearer way to do this using the AsyncStreamMediaSource class but for now we are just going
      // to do it manually and then reset the input stream
      // This code assumed that the SPS and PPS are at the beginning of the stream
      assert (m_in1.good());

      const int bufferSize = 20000;
      uint8_t buffer[bufferSize];
      m_in1.read((char*)buffer, bufferSize);
      // now search for the SPS and the PPS
      using namespace media::h264;
      NalUnitType ePreviousType = NUT_UNSPECIFIED;
      int32_t iPreviousIndex = -1;

      char startCode[3] = { 0, 0, 1};
      for (size_t i = 0; i < bufferSize - 3; ++i)
      {
        if (memcmp(&buffer[i], startCode, 3) == 0)
        {
          if (ePreviousType == NUT_SEQUENCE_PARAMETER_SET)
          {
            assert (iPreviousIndex != -1);
            m_sSps = std::string((const char*)&(buffer[iPreviousIndex + 3]), (i - iPreviousIndex - 3));
          }
          else if (ePreviousType == NUT_PICTURE_PARAMETER_SET)
          {
            assert (iPreviousIndex != -1);
            m_sPps = std::string((const char*)(&buffer[iPreviousIndex + 3]), (i - iPreviousIndex - 3));
          }

          if (!m_sSps.empty() && !m_sPps.empty())
          {
            break;
          }
          ePreviousType = getNalUnitType(buffer[i + 3]);
          iPreviousIndex = i;
        }
      }

      if (m_sSps.empty() || m_sPps.empty() )
      {
        LOG(WARNING) << "Failed to find SPS and PPS in first 256 bytes of H264 file";
      }
      else
      {
        char fConfigStr[7];
        sprintf(fConfigStr, "%02X%02X%02X", m_sSps.at(1), m_sSps.at(2), m_sSps.at(3));
        m_sProfileLevelId = std::string((char*)fConfigStr, 6);
        m_sSPropParameterSets = base64_encode(m_sSps) + "," + base64_encode(m_sPps);
      }

      m_in1.clear();
      m_in1.seekg(0, std::ios_base::beg);
    }
  }
  else if (sFileType == media::YUV420P)
  {
    std::string sOutFile = "out";
    std::map<std::string, std::string> params;
    for (auto item : videoCodecParameters)
    {
      auto pair = StringTokenizer::tokenize(item, "=", true, true);
      if (pair.size() == 1)
      {
        params[pair[0]] = "";
      }
      else if (pair.size() == 2)
      {
        params[pair[0]] = pair[1];
      }
      else
      {
        LOG(WARNING) << "Invalid parameter: " << item;
      }
    }

    auto it = params.find("out");
    if (it != params.end())
    {
      sOutFile = it->second;
    }

    if ( sMediaType == rfc6184::H264 )
    {
#if 0
      m_pMediaSink = new MediaSink("out.264");
#else
      m_pMediaSink = new h264::H264AnnexBStreamWriter(sOutFile + ".264", false,  true);
#endif
    }
    else if ( sMediaType == rfchevc::H265 )
    {
      m_pMediaSink = new MediaSink(sOutFile + ".265");
    }

    VLOG(2) << "initialising transform!";
    // add other option (if yuv extension is detected)
    m_pSource = std::unique_ptr<MediaSource>(new YuvMediaSource(sDeviceName, uiWidth, uiHeight, bLoop, uiLoopCount));
    bool success = app::ApplicationUtil::initialiseFileMediaSource(m_in1, sDeviceName);
    assert(success);

    if (sMediaType == rfc6184::H264)
    {
      VLOG(2) << "Loading H264 codec";
      std::vector<std::string> codecImplementations;
#ifdef ENABLE_VPP
      codecImplementations.push_back("vpp");
#endif
#ifdef ENABLE_X264
      codecImplementations.push_back("x264");
#endif
#ifdef ENABLE_OPEN_H264
      codecImplementations.push_back("openh264");
#endif

      std::unique_ptr<ITransform> pCodec;
      auto it = find_if(codecImplementations.begin(), codecImplementations.end(), [sVideoCodecImpl](const std::string& sImpl){ return sImpl == sVideoCodecImpl;  });
      if (it != codecImplementations.end())
      {
#ifdef ENABLE_VPP
        if (*it == "vpp")
        {
          pCodec = std::unique_ptr<ITransform>(new VppH264Codec());
        }
#endif
#ifdef ENABLE_X264
        else if ( *it == "x264")
        {
          pCodec = std::unique_ptr<ITransform>(new X264Codec());
        }
#endif
#ifdef ENABLE_OPEN_H264
        else if (*it == "openh264")
        {
          pCodec = std::unique_ptr<ITransform>(new OpenH264Codec());
        }
#endif
      }
      else
      {
#ifdef ENABLE_VPP
        // just use VPP codec
        pCodec = std::unique_ptr<ITransform>(new VppH264Codec());
#endif
      }

      MediaTypeDescriptor mediaIn(MediaTypeDescriptor::MT_VIDEO, MediaTypeDescriptor::MST_YUV_420P, uiWidth, uiHeight, uiFps);
      boost::system::error_code ec = pCodec->setInputType(mediaIn);
      if (ec)
      {
        LOG(ERROR) << "Error initialising transform!: " << ec.message();
        // TODO: should exit app here
      }
      else
      {
        // set parameters
#ifdef TEST_BITRATE_SWITCHING
        for (auto param : params)
        {
          // override cmd line for testing
          if (param.first == "test_bitrates")
          {
            m_vTestBitrates = StringTokenizer::tokenizeV2<uint32_t>(param.second, "|", true);
          }
          else if (param.first == "switch")
          {
            bool bDummy;
            m_iSwitchFrame = convert<int>(param.second, bDummy);
          }
        }
        if (m_vTestBitrates.empty()) m_vTestBitrates = default_bitrates;
#endif

        VLOG(2) << "Bitrates: " << ::toString(m_vTestBitrates);
        for (auto param : params)
        {
          std::string sValue = param.second;
#ifdef TEST_BITRATE_SWITCHING
          // override cmd line for testing
          if (param.first == "bitrate")
          {
            sValue = ::toString(m_vTestBitrates.at(0));
          }
#endif
          VLOG(2) << "Calling configure: Name: " << param.first << " value: " << sValue;
          ec = pCodec->configure(param.first, sValue);
          if (ec)
          {
            LOG(WARNING) << "Failed to set codec parameter: " << param.first << " value: " << param.second;
          }
        }
        
        ec = pCodec->initialise();

        if (!ec)
        {
          // initialise
          m_transforms.push_back(std::move(pCodec));
        }
        else
        {
          LOG(ERROR) << "Failed to initialise codec";
        }
      }

    }
#ifdef ENABLE_X265
    else if (sMediaType == rfchevc::H265)
    {
      VLOG(2) << "Loading H265 codec";
#ifdef ENABLE_X265
      std::unique_ptr<ITransform> pCodec = std::unique_ptr<ITransform>(new X265Codec());
      MediaTypeDescriptor mediaIn(MediaTypeDescriptor::MT_VIDEO, MediaTypeDescriptor::MST_YUV_420P, uiWidth, uiHeight, uiFps);
      boost::system::error_code ec = pCodec->setInputType(mediaIn);
      if (ec)
      {
        LOG(ERROR) << "Error initialising transform!: " << ec.message();
        // TODO: should exit app here
      }
      else
      {
#ifdef TEST_BITRATE_SWITCHING
        for (auto param : params)
        {
          // override cmd line for testing
          if (param.first == "test_bitrates")
          {
            m_vTestBitrates = StringTokenizer::tokenizeV2<uint32_t>(param.second, "|", true);
          }
          else if (param.first == "switch")
          {
            bool bDummy;
            m_iSwitchFrame = convert<int>(param.second, bDummy);
          }
        }
        if (m_vTestBitrates.empty()) m_vTestBitrates = default_bitrates;
#endif

        for (auto param : params)
        {
          std::string sValue = param.second;
#ifdef TEST_BITRATE_SWITCHING
          // override cmd line for testing
          if (param.first == "bitrate")
          {
            sValue = ::toString(m_vTestBitrates.at(0));
          }
#endif
          VLOG(2) << "Calling configure: Name: " << param.first << " value: " << sValue;
          ec = pCodec->configure(param.first, sValue);
          if (ec)
          {
            LOG(WARNING) << "Failed to set codec parameter: " << param.first << " value: " << param.second;
          }
        }

        ec = pCodec->initialise();

        if (!ec)
        {
          // initialise
          m_transforms.push_back(std::move(pCodec));
        }
        else
        {
          LOG(ERROR) << "Failed to initialise codec";
        }
      }
#endif
    }
#endif
  }
  else
  {
    // HACK for now
    m_pSource = std::unique_ptr<MediaSource>(new GeneratedMediaSource(PT_RAND, "1000", 0, 0));
  }
}

VirtualVideoDeviceV2::~VirtualVideoDeviceV2()
{
  if (m_pMediaSink)
  {
    delete m_pMediaSink;
  }

  if (m_in1.is_open())
    m_in1.close();
}

bool VirtualVideoDeviceV2::isRunning() const
{
  return true;
}

boost::system::error_code VirtualVideoDeviceV2::start()
{
  // get NALU
  if (outputNal())
  {
    // outputNal sets m_tStart
    m_timer.expires_at(m_tStart + boost::posix_time::microseconds(m_uiFrameDurationUs));
    m_timer.async_wait(boost::bind(&VirtualVideoDeviceV2::handleTimeout, this, boost::asio::placeholders::error));
  }
  else
  {
    m_onComplete(boost::system::error_code());
  }

  return boost::system::error_code();
}

boost::system::error_code VirtualVideoDeviceV2::stop()
{
  VLOG(5) << "Stopping virtual video device from file";
  m_bShuttingDown = true;
  // cancel timer
  m_timer.cancel();
  return boost::system::error_code();
}

boost::system::error_code VirtualVideoDeviceV2::setBitrate(uint32_t uiTargetBitrateKbps)
{
  if (m_transforms.empty())
  {
    return boost::system::error_code(boost::system::errc::argument_out_of_domain, boost::system::generic_category());
  }
  else
  {
    boost::system::error_code ec;
    for (auto it = m_transforms.rbegin(); it != m_transforms.rend(); ++it)
    {
      IVideoCodecTransform* pVideoTransform = dynamic_cast<IVideoCodecTransform*>(it->get());
      if (!pVideoTransform)
        continue;

      VLOG(2) << "Setting bitrate to " << uiTargetBitrateKbps << " kbps";
      ec = pVideoTransform->setBitrate(uiTargetBitrateKbps);
      if (!ec)
      {
        return ec;
      }
    }
    return boost::system::error_code(boost::system::errc::argument_out_of_domain, boost::system::generic_category());
  }
}

bool VirtualVideoDeviceV2::outputNal()
{
  if (m_pSource->isGood())
  {
    std::vector<MediaSample> vAu = m_pSource->getNextAccessUnit();

    if (vAu.empty()) return false;

#ifdef TEST_BITRATE_SWITCHING
    if (!m_vTestBitrates.empty()) // causes crash if test_bitrates aren't set
    {
      static int iFrameNumber = 1;
      static int iIndex = 0;
      if (iFrameNumber % m_iSwitchFrame == 0)
      {
        iIndex = (iIndex + 1) % m_vTestBitrates.size();
        VLOG(2) << "Switching bitrate frame: index" << iIndex << " test rates: " << ::toString(m_vTestBitrates);
        boost::system::error_code ec = setBitrate(m_vTestBitrates.at(iIndex));
        if (ec)
        {
          VLOG(2) << "Error setting bitrate to " << m_vTestBitrates.at(iIndex) << "kbps";
        }
        else
        {
          VLOG(2) << "Bitrate adjusted to " << m_vTestBitrates.at(iIndex) << "kbps";
        }
        // HACK set switch frame to big number to only have one switch per experiment (300 frames)
        m_iSwitchFrame = INT32_MAX;
      }
      ++iFrameNumber;
    }
#endif
    // simplistic media pipeline: we need the ability to scale and encode raw samples to simulate a live source
    if (!m_transforms.empty())
    {
      std::vector<MediaSample> vOut;

#define MEASURE_ENCODING_TIME
#ifdef MEASURE_ENCODING_TIME
      boost::posix_time::ptime tStart = boost::posix_time::microsec_clock::universal_time();
#endif
      uint32_t uiSize = 0;
      boost::system::error_code ec = transform(vAu, vOut, uiSize);
#ifdef MEASURE_ENCODING_TIME
      boost::posix_time::ptime tEnd = boost::posix_time::microsec_clock::universal_time();
      boost::posix_time::time_duration diff = tEnd - tStart;
      static int iFrameNo = 0;
      VLOG(2) << "Frame: " << iFrameNo++ << " Time to encode: " << diff.total_milliseconds() << "ms Size: " << uiSize;
#endif
      if (ec) return false;
      std::swap(vAu, vOut);
    }

    switch (m_eMode)
    {
      case TM_LIVE:
      {
        if (m_tStart.is_not_a_date_time())
        {
          m_tStart = boost::posix_time::microsec_clock::universal_time();
          m_dCurrentAccessUnitPts = 0.0;
        }
        else
        {
          boost::posix_time::ptime tNow = boost::posix_time::microsec_clock::universal_time();
          boost::posix_time::time_duration diff = tNow - m_tStart;
          m_dCurrentAccessUnitPts = diff.total_seconds() + (diff.fractional_seconds()/1000000.0);
        }
        break;
      }
      case TM_FRAME_RATE:
      {
        // also set m_tStart here since it's used as the base time for the clock
        if (m_tStart.is_not_a_date_time())
          m_tStart = boost::posix_time::microsec_clock::universal_time();
        m_dCurrentAccessUnitPts = m_uiAuCount * m_dFrameDuration;
        break;
      }
    }

    // timestamp samples
    for (MediaSample& mediaSample: vAu)
    {
      mediaSample.setStartTime(m_dCurrentAccessUnitPts);
    }

    // HACK for openH264 outputting empty NAL units
    if (vAu.empty())
    {
      LOG(WARNING) << "Error: empty NAL";
      return true;
    }

    m_accessUnitCB(vAu);

    if (m_pMediaSink)
    {
      m_pMediaSink->writeAu(vAu);
    }

    ++m_uiAuCount;

    return true;
  }
  else
  {
    return false;
  }
}

void VirtualVideoDeviceV2::handleTimeout(const boost::system::error_code& ec)
{
  if (!ec)
  {
    if (outputNal())
    {
      // start next callback
      if (!m_bShuttingDown)
      {
        boost::posix_time::ptime  tNext = m_timer.expires_at() + boost::posix_time::microseconds(m_uiFrameDurationUs);
        m_timer.expires_at(tNext);
        m_timer.async_wait(boost::bind(&VirtualVideoDeviceV2::handleTimeout, this, _1));
      }
      else
      {
        VLOG(2) << "Shutting down";
      }
    }
    else
    {
      LOG(WARNING) << "EOF";
      if (m_onComplete)
        m_onComplete(boost::system::error_code());
    }
  }
  else
  {
    if (ec != boost::asio::error::operation_aborted)
    {
      LOG(WARNING) << "Error in timeout: " << ec.message();
    }
    else
    {
      // it means the timer was cancelled
      VLOG(5) << "Timer cancelled: " << ec.message();
    }
  }
}

boost::system::error_code VirtualVideoDeviceV2::getParameter(const std::string& sParamName, std::string& sParamValue)
{
  if (sParamName == rfc6184::PROFILE_LEVEL_ID)
  {
    if (m_sProfileLevelId.empty())
    {
      // we had failed to extract the SPS/PPS
      return boost::system::error_code(boost::system::errc::bad_file_descriptor, boost::system::get_generic_category());
    }
    else
    {
     sParamValue = m_sProfileLevelId;
     return boost::system::error_code();
    }
  }
  else if (sParamName == rfc6184::SPROP_PARAMETER_SETS)
  {
    if (m_sSPropParameterSets.empty())
    {
      // we had failed to extract the SPS/PPS
      return boost::system::error_code(boost::system::errc::bad_file_descriptor, boost::system::get_generic_category());
    }
    else
    {
      sParamValue = m_sSPropParameterSets;
      return boost::system::error_code();
    }
  }
  else
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::get_generic_category());
  }
}

boost::system::error_code VirtualVideoDeviceV2::setParameter(const std::string& sParamName, const std::string& sParamValue)
{
  return boost::system::error_code(boost::system::errc::not_supported, boost::system::get_generic_category());
}

boost::system::error_code VirtualVideoDeviceV2::transform(const std::vector<MediaSample>& vMediaSamples, std::vector<MediaSample>& vTransformed, uint32_t& uiSize)
{
  boost::system::error_code ec;
  std::vector<MediaSample> vIn = vMediaSamples;
  for (auto& transform : m_transforms)
  {
    std::vector<MediaSample> vOut;
    ec = transform->transform(vIn, vOut, uiSize);
    if (ec)
    {
      LOG(WARNING) << "Error in media transform";
      return ec;
    }
    std::swap(vIn, vOut);
  }
  std::swap(vIn, vTransformed);
  return ec;
}

} // media
} // rtp_plus_plus
