#include "CorePch.h"
#include "X265Codec.h"
#include <cpputil/Conversion.h>
#include <x265.h>

using namespace rtp_plus_plus::media;

#define COMPONENT_LOG_LEVEL 12

const std::vector<std::string> TuneOptions = {"psnr", "ssim", "grain", "fastdecode", "zerolatency"};
const std::vector<std::string> PresetOptions = { "ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", "placebo" };


X265Codec::X265Codec()
  :encoder(nullptr),
    m_uiIFramePeriod(0),
    m_uiCurrentFrame(0),
    m_uiFrameBitLimit(0),
    m_bNotifyOnIFrame(false),
    m_uiEncodingBufferSize(0),
    m_uiTargetBitrate(500),
    m_sTune(TuneOptions.at(4)),
    m_sPreset(PresetOptions.at(0))
{

}

X265Codec::~X265Codec()
{
  if (pBufferIn)
  {
    delete[] pBufferIn;
  }

  if (params)
  {
    x265_param_free(params);
  }

  if(encoder)
  {
    x265_encoder_close(encoder);
    encoder = NULL;
  }

  if (pic_in)
  {
    x265_picture_free(pic_in);
  }

  if (pic_out)
  {
    x265_picture_free(pic_out);
  }
  x265_cleanup();
}

boost::system::error_code X265Codec::setInputType(const MediaTypeDescriptor& in)
{
  VLOG(COMPONENT_LOG_LEVEL) << "X265Codec::setInputType";
  if (in.m_eSubtype != rtp_plus_plus::media::MediaTypeDescriptor::MST_YUV_420P)
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }

  if (in.getHeight() == 0 || in.getWidth() == 0 || in.getFps() == 0.0)
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }

  m_in = in;

  m_uiEncodingBufferSize = m_in.getWidth() * m_in.getHeight() * 1.5;
  // buffer for incoming YUV
  pBufferIn = new uint8_t[m_uiEncodingBufferSize];
  VLOG(2) << "Encoding buffer size: " << m_uiEncodingBufferSize;
  m_encodingBuffer.setData(new uint8_t[m_uiEncodingBufferSize], m_uiEncodingBufferSize);

  return boost::system::error_code();
}

boost::system::error_code X265Codec::configure(const std::string& sName, const std::string& sValue)
{
  if (sName == "bitrate")
  {
    bool bDummy;
    VLOG(2) << "Target bitrate: " << m_uiTargetBitrate << "kbps";
    m_uiTargetBitrate = convert<uint32_t>(sValue, bDummy);
    assert(bDummy);
    assert(m_in.getFps() != 0.0);
    return boost::system::error_code();
  }
  else if (sName == "tune")
  {
    auto it = find_if(TuneOptions.begin(), TuneOptions.end(), [sValue](const std::string& sOption){ return sOption == sValue; });
    if (it == TuneOptions.end())
    {
      return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
    }
    else
    {
      VLOG(2) << "Tune: " << m_sTune;
      m_sTune == *it;
      return boost::system::error_code();
    }
  }
  else if (sName == "preset")
  {
    auto it = find_if(PresetOptions.begin(), PresetOptions.end(), [sValue](const std::string& sOption){ return sOption == sValue; });
    if (it == PresetOptions.end())
    {
      return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
    }
    else
    {
      m_sPreset == *it;
      VLOG(2) << "Preset: " << m_sPreset;
      return boost::system::error_code();
    }
  }
  return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
}

boost::system::error_code X265Codec::initialise()
{
  const int iFrameRate = 30;

  params = x265_param_alloc();
  if (!params)
  {
    LOG(ERROR) << "Failed to allocate x65 params";
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }

  int res = x265_param_default_preset(params, m_sPreset.c_str(), m_sTune.c_str());
  if (res < 0)
  {
    LOG(ERROR) << "Invalid preset/tune name";
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }

  // optionally set profile
  res = x265_param_apply_profile(params, "main");
  if (res < 0)
  {
    LOG(ERROR) << "Invalid profile name";
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }

  // params->ti_threads = 1;
  params->sourceWidth = m_in.getWidth();
  params->sourceHeight = m_in.getHeight();
  // HACK for now: we use doubles (won't work for 12.5)
  params->fpsNum = (int)m_in.getFps();
  params->fpsDenom = 1;
  params->internalCsp = X265_CSP_I420;
  params->bRepeatHeaders = true;
  params->bEnableAccessUnitDelimiters = false;
#if 1
  params->rc.bitrate = m_uiTargetBitrate;
  params->rc.rateControlMode = X265_RC_ABR;
  //params->rc.bitrate = 0;
  //params->rc.rateControlMode = X265_RC_CQP;
#endif

  encoder = x265_encoder_open(params);
  if (!encoder)
  {
    LOG(ERROR) << "Failed to x265_encoder_open";
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }

  uint32_t uiNalCount = 0;
  int header_size = x265_encoder_headers(encoder, &nals, &uiNalCount);


  pic_in = x265_picture_alloc();
  x265_picture_init(params, pic_in);

  // pic.planes
  pic_in->stride[0] = m_in.getWidth();
  pic_in->stride[1] = pic_in->stride[2] = m_in.getWidth() >> 1;  // const uint8_t* pBufferOut = m_encodingBuffer.data();
  pic_in->bitDepth = 8;
  pic_in->planes[0] = (uint8_t*)pBufferIn;
  pic_in->planes[1] = (uint8_t*)pic_in->planes[0] + pic_in->stride[0] * m_in.getHeight();
  pic_in->planes[2] = (uint8_t*)pic_in->planes[1] + ((m_in.getWidth() * m_in.getHeight()) >> 2);

  // not currently using out pic
  pic_out = x265_picture_alloc();
  x265_picture_init(params, pic_out);
  return boost::system::error_code();
}

boost::system::error_code X265Codec::getOutputType(MediaTypeDescriptor& out)
{
  VLOG(COMPONENT_LOG_LEVEL) << "VppH264Codec::getOutputType";
  out.m_eType = rtp_plus_plus::media::MediaTypeDescriptor::MT_VIDEO;
  out.m_eSubtype = rtp_plus_plus::media::MediaTypeDescriptor::MST_H265;
  out.m_uiWidth = m_in.getWidth();
  out.m_uiHeight = m_in.getHeight();
  m_out = out;
  return boost::system::error_code();
}

boost::system::error_code X265Codec::transform(const std::vector<MediaSample>& in, std::vector<MediaSample>& out, uint32_t& uiSize)
{
  VLOG(COMPONENT_LOG_LEVEL) << "X265Codec::transform";
  assert (encoder);
  // we only handle one sample at a time
  assert(in.size() == 1);
  const MediaSample& mediaIn = in[0];
  // const uint8_t* pBufferIn = mediaIn.getDataBuffer().data();
  memcpy(pBufferIn, mediaIn.getDataBuffer().data(), m_uiEncodingBufferSize);

#if 0
  VLOG(COMPONENT_LOG_LEVEL) << "X265Codec::transform calling memcpy";
  memcpy(pic_in->planes[0], (uint8_t*)pBufferIn, m_uiEncodingBufferSize);
#endif
  uint32_t uiNalCount = 0;
  //int frame_size = x265_encoder_encode(encoder, &nals, &uiNalCount, pic_in, pic_out);
#if 1
  int frame_size = x265_encoder_encode(encoder, &nals, &uiNalCount, pic_in, NULL);
  // int frame_size = x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out);
  if(frame_size > 0)
  {
    uiSize = frame_size;
    uint32_t uiLen = 0;
    for (size_t i = 0; i < uiNalCount; ++i)
    {
      uiLen += nals[i].sizeBytes;
      VLOG(2) << "Transform complete: "
              << " i: " << i
              << " NALU type: " << nals[i].type
              << " frame size: " << nals[i].sizeBytes;
      Buffer mediaData(new uint8_t[nals[i].sizeBytes], nals[i].sizeBytes);
      memcpy((char*)mediaData.data(), nals[i].payload, nals[i].sizeBytes);
      MediaSample mediaSample;
      mediaSample.setData(mediaData);
      out.push_back(mediaSample);
    }
  }
  else
  {
    if (frame_size == 0)
    {
      VLOG(2) << "NULL NAL Units" << mediaIn.getPayloadSize();
    }
    else
    {
      VLOG(2) << "Transform failed: in size: " << mediaIn.getPayloadSize() << " frame size: " << frame_size;
    }
  }
#endif
  return boost::system::error_code();
}

boost::system::error_code X265Codec::setBitrate(uint32_t uiTargetBitrate)
{
  params->rc.bitrate = uiTargetBitrate;
  int res = x265_encoder_reconfig(encoder, params);
  if (res < 0)
  {
    return boost::system::error_code(boost::system::errc::argument_out_of_domain, boost::system::generic_category());
  }
  else
  {
    return boost::system::error_code();
  }

  return boost::system::error_code(boost::system::errc::argument_out_of_domain, boost::system::generic_category());
}

