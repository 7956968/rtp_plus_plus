#include "CorePch.h"
#include "OpenH264Codec.h"
#include <codec_api.h>
#include <cpputil/Conversion.h>

using namespace rtp_plus_plus::media;

OpenH264Codec::OpenH264Codec()
  :m_pCodec(nullptr),
    m_uiTargetBitrate(0),
    m_uiEncodingBufferSize(0)
{
  int rv = WelsCreateSVCEncoder (&m_pCodec);
  assert (rv == 0);
  assert (m_pCodec != NULL);
}

OpenH264Codec::~OpenH264Codec()
{
  assert(m_pCodec);
  if (m_pCodec) {
    m_pCodec->Uninitialize();
    WelsDestroySVCEncoder (m_pCodec);
  }
}

boost::system::error_code OpenH264Codec::setInputType(const MediaTypeDescriptor& in)
{
  VLOG(2) << "OpenH264Codec::checkInputType";
  if (in.m_eSubtype != rtp_plus_plus::media::MediaTypeDescriptor::MST_YUV_420P)
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }

  m_in = in;

  m_uiEncodingBufferSize = in.getWidth() * in.getHeight() * 1.5;
  VLOG(2) << "Encoding buffer size: " << m_uiEncodingBufferSize;
  m_encodingBuffer.setData(new uint8_t[m_uiEncodingBufferSize], m_uiEncodingBufferSize);

  return boost::system::error_code();
}

boost::system::error_code OpenH264Codec::configure(const std::string& sName, const std::string& sValue)
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
  return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
}

boost::system::error_code OpenH264Codec::initialise()
{
#if 0
  SEncParamBase param;
  memset (&param, 0, sizeof (SEncParamBase));
  param.iUsageType = CAMERA_VIDEO_REAL_TIME;
  param.fMaxFrameRate = (uint32_t)m_in.getFps();
  param.iPicWidth = m_in.getWidth();
  param.iPicHeight = m_in.getHeight();
  VLOG(2) << "Initialising target bitrate to " << m_uiTargetBitrate << " kbps";
  param.iTargetBitrate = m_uiTargetBitrate * 1000; // convert to kbps

  m_pCodec->Initialize (&param);
#else
  SEncParamExt param;
  m_pCodec->GetDefaultParams (&param);
  param.iUsageType = CAMERA_VIDEO_REAL_TIME;
  param.fMaxFrameRate = (uint32_t)m_in.getFps();
  param.iPicWidth = m_in.getWidth();
  param.iPicHeight = m_in.getHeight();
  param.iTargetBitrate = m_uiTargetBitrate * 1000;
  //param.bEnableDenoise = denoise;
  param.bEnableDenoise = false;
  param.iSpatialLayerNum = 1;
  //SM_DYN_SLICE don't support multi-thread now
#if 0
  if (sliceMode != SM_SINGLE_SLICE && sliceMode != SM_DYN_SLICE)
      param.iMultipleThreadIdc = 2;
#endif
  // RG: this prevents IDR generation on scene change
#if 1
  param.bEnableSceneChangeDetect = false;
#endif
  param.bEnableFrameSkip = false;
  for (int i = 0; i < param.iSpatialLayerNum; i++) {
      param.sSpatialLayers[i].iVideoWidth = m_in.getWidth() >> (param.iSpatialLayerNum - 1 - i);
      param.sSpatialLayers[i].iVideoHeight = m_in.getHeight() >> (param.iSpatialLayerNum - 1 - i);
      param.sSpatialLayers[i].fFrameRate = (uint32_t)m_in.getFps();
      param.sSpatialLayers[i].iSpatialBitrate = param.iTargetBitrate;
      param.sSpatialLayers[i].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;
#if 0
      param.sSpatialLayers[i].sSliceCfg.uiSliceMode = sliceMode;
      if (sliceMode == SM_DYN_SLICE) {
          param.sSpatialLayers[i].sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 600;
          param.uiMaxNalSize = 1500;
      }
#endif
  }
  param.iTargetBitrate *= param.iSpatialLayerNum;
  m_pCodec->InitializeExt (&param);
  int videoFormat = videoFormatI420;
  m_pCodec->SetOption (ENCODER_OPTION_DATAFORMAT, &videoFormat);
#endif

  return boost::system::error_code();
}

boost::system::error_code OpenH264Codec::getOutputType(MediaTypeDescriptor& out)
{
  VLOG(2) << "OpenH264Codec::getOutputType";
  out.m_eType = rtp_plus_plus::media::MediaTypeDescriptor::MT_VIDEO;
  out.m_eSubtype = rtp_plus_plus::media::MediaTypeDescriptor::MST_H264;
  out.m_uiWidth = m_in.getWidth();
  out.m_uiHeight = m_in.getHeight();
  m_out = out;
  return boost::system::error_code();
}

boost::system::error_code OpenH264Codec::transform(const std::vector<MediaSample>& in, std::vector<MediaSample>& out, uint32_t& uiSize)
{
  VLOG(12) << "OpenH264Codec::transform";
  assert (m_pCodec);
  // we only handle one sample at a time
  assert(in.size() == 1);
  const MediaSample& mediaIn = in[0];
  // const uint8_t* pBufferIn = mediaIn.getDataBuffer().data();
  // const uint8_t* pBufferOut = m_encodingBuffer.data();

  SFrameBSInfo info;
  memset (&info, 0, sizeof (SFrameBSInfo));

#if 0
  SBitrateInfo newEncoderBitRate;
  newEncoderBitRate.iLayer = SPATIAL_LAYER_ALL;
  newEncoderBitRate.iBitrate = 500;
  int res = m_pCodec->SetOption (ENCODER_OPTION_BITRATE, &newEncoderBitRate);
  if (res == 0)
  {
    VLOG(2) << "Updated bitrate";
  }
  else
  {
    LOG(WARNING) << "Failed to update bitrate";
  }
#endif

  SSourcePicture pic;
  memset (&pic, 0, sizeof (SSourcePicture));
  pic.iPicWidth    = m_in.getWidth();
  pic.iPicHeight   = m_in.getHeight();
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0]   = m_in.getWidth();
  pic.iStride[1]   = pic.iStride[2] = m_in.getWidth() >> 1;
  pic.pData[0]     = (uint8_t*)mediaIn.getDataBuffer().data();
  pic.pData[1]     = pic.pData[0] + m_in.getWidth() * m_in.getHeight();
  pic.pData[2]     = pic.pData[1] + (m_in.getWidth() * m_in.getHeight() >> 2);

#if 0
  int traceLevel = WELS_LOG_DETAIL;
  m_pCodec->SetOption(ENCODER_OPTION_TRACE_LEVEL, &traceLevel);
#endif
  int rv = m_pCodec->EncodeFrame (&pic, &info);
  assert(rv == cmResultSuccess);

  if (info.eFrameType != videoFrameTypeSkip)
  {
    VLOG(6) << "Transform complete: in size: " << mediaIn.getPayloadSize() << " frame size: " << info.iFrameSizeInBytes
            << " Temporal Id: " << info.iTemporalId
            << " Sub seq id: " << info.iSubSeqId
            << " Layer num: " << info.iLayerNum
            << " Frame type: " << (int)info.eFrameType
            << " TS: " << info.uiTimeStamp;

    int len = 0;
    for (int i = 0; i < info.iLayerNum; ++i)
    {
      const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
      VLOG(6) << "Layer type: " << (int)layerInfo.uiLayerType
              << " temp id: " << (int)layerInfo.uiTemporalId
              << " spatial id: " << (int)layerInfo.uiSpatialId
              << " quality id: " << (int)layerInfo.uiQualityId
              << " nal count: " << layerInfo.iNalCount;

      uint8_t* pBuffer = layerInfo.pBsBuf;
      for (int j = 0; j < layerInfo.iNalCount; ++j)
      {
        uint32_t uiNaluLength = layerInfo.pNalLengthInByte[j];
        VLOG(2) << "Adding NALU of length " << uiNaluLength;
        len += uiNaluLength;
        Buffer mediaData(new uint8_t[uiNaluLength], uiNaluLength);
        memcpy((char*)mediaData.data(), pBuffer, uiNaluLength);
        MediaSample mediaSample;
        mediaSample.setData(mediaData);
        mediaSample.setNaluContainsStartCode(true);
        out.push_back(mediaSample);
        pBuffer += uiNaluLength;
      }
    }
    VLOG(6) << "Total length: " << len;
    uiSize = len;
    return boost::system::error_code();
  }
  else
  {
    VLOG(2) << "Skip frame";
    return boost::system::error_code();
  }
}

boost::system::error_code OpenH264Codec::setBitrate(uint32_t uiTargetBitrate)
{
  if (m_uiTargetBitrate != uiTargetBitrate)
  {
#if 0
    return boost::system::error_code(boost::system::errc::argument_out_of_domain, boost::system::generic_category());
#else
    SBitrateInfo newEncoderBitRate;
    newEncoderBitRate.iLayer = SPATIAL_LAYER_ALL;
    newEncoderBitRate.iBitrate = uiTargetBitrate * 1000;

    int res = m_pCodec->SetOption (ENCODER_OPTION_BITRATE, &newEncoderBitRate);
    if (res == 0)
    {
      VLOG(2) << "Updated bitrate";
    }
    else
    {
      LOG(WARNING) << "Failed to update bitrate";
    }
    return boost::system::error_code();
#endif
    // return boost::system::error_code(boost::system::errc::argument_out_of_domain, boost::system::generic_category());
  }
  else
  {
    // NOOP
    return boost::system::error_code();
  }
}
