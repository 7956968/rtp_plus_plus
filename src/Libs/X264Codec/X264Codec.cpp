#include "CorePch.h"
#include "X264Codec.h"
#include <cpputil/Conversion.h>

using namespace rtp_plus_plus::media;

X264Codec::X264Codec()
  :nals(nullptr),
    encoder(nullptr),
    m_uiTargetBitrate(0),
    m_uiEncodingBufferSize(0),
    m_uiMode(0),
    m_dCbrFactor(0.8),
    m_uiMaxSliceSize(m_uiMaxSliceSize)
{

}

X264Codec::~X264Codec()
{
  assert(encoder);
  if(encoder) {
    x264_picture_clean(&pic_in);
    memset((char*)&pic_in, 0, sizeof(pic_in));
    memset((char*)&pic_out, 0, sizeof(pic_out));

    x264_encoder_close(encoder);
    encoder = NULL;
  }
}

boost::system::error_code X264Codec::setInputType(const MediaTypeDescriptor& in)
{
  VLOG(2) << "X264Codec::checkInputType";
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

boost::system::error_code X264Codec::configure(const std::string& sName, const std::string& sValue)
{
  bool bDummy;
  if (sName == "bitrate")
  {
    m_uiTargetBitrate = convert<uint32_t>(sValue, bDummy);
    assert(bDummy);
    VLOG(2) << "Target bitrate: " << m_uiTargetBitrate << "kbps";
    assert(m_in.getFps() != 0.0);
    return boost::system::error_code();
  }
  else if (sName == "mode")
  {
    m_uiMode = convert<uint32_t>(sValue, bDummy);
    VLOG(2) << "Mode set to: " << m_uiMode;
    assert(bDummy);
    return boost::system::error_code();
  }
  else if ((sName == "cbr_factor") || (sName == "cbrf"))
  {
    m_dCbrFactor = convert<double>(sValue, bDummy);
    VLOG(2) << "CBR factor set to: " << m_dCbrFactor;
    assert(bDummy);
    return boost::system::error_code();
  }
  else if (sName == "max_slice_size")
  {
    m_uiMaxSliceSize = convert<uint32_t>(sValue, bDummy);
    VLOG(2) << "Max slice size set to: " << m_uiMaxSliceSize;
    assert(bDummy);
    return boost::system::error_code();
  }
  return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
}

void X264Codec::configureParams()
{
  x264_param_default_preset(&params, "ultrafast", "zerolatency");

  VLOG(2) << "Default level: " << params.i_level_idc;
  params.i_threads = 1;
  params.i_width = m_in.getWidth();
  params.i_height = m_in.getHeight();
  // HACK for now: we use doubles (won't work for 12.5)
  params.i_fps_num = (int)m_in.getFps();
  params.i_fps_den = 1;

  // for debugging
#if 0
  params.i_log_level = X264_LOG_DEBUG;
#endif
  // disables periodic IDR frames
  params.i_keyint_max = X264_KEYINT_MAX_INFINITE;
#if 1
  params.rc.i_rc_method = X264_RC_ABR ;
  VLOG(2) << "X264Codec::configureParams: " << m_uiTargetBitrate;
  params.rc.i_bitrate = m_uiTargetBitrate;
  // params.rc.i_bitrate = nBitRate*0.65/1000  ;
#endif

  switch (m_uiMode)
  {
    // STD
    case 0:
    {
      break;
    }
    // CBR
    case 2:
    {
      VLOG(2) << "CBR mode!";
#if 0
      params.i_nal_hrd = X264_NAL_HRD_CBR;
#endif
      params.rc.i_vbv_buffer_size = m_uiTargetBitrate;
      params.rc.i_vbv_max_bitrate = m_uiTargetBitrate*m_dCbrFactor;
      params.rc.f_rate_tolerance = 1.0 ;
      break;
    }
  }
  // TODO: look at other params for real-time
#if 0
  i_nal_hrd // #define X264_NAL_HRD_CBR             2
  b_open_gop;
  Params.rc.i_vbv_buffer_size = nBitRate/1000;
  Params.rc.i_vbv_max_bitrate = nBitRate*0.65/1000 ;
  Params.rc.f_vbv_buffer_init = 1.0 ;
  Params.rc.f_rate_tolerance = 1.0 ;
#endif

  // testing slicing NALUs into multiple slices
  if (m_uiMaxSliceSize > 0)
    params.i_slice_max_size = m_uiMaxSliceSize;
}

boost::system::error_code X264Codec::initialise()
{
  VLOG(2) << "Initialising codec, mode set to: " << m_uiMode;
  int r = 0;
  int nheader = 0;
  int header_size = 0;
  x264_picture_alloc(&pic_in, X264_CSP_I420, m_in.getWidth(), m_in.getHeight());

  configureParams();


  // create the encoder using our params
  encoder = x264_encoder_open(&params);
  if(!encoder) {
    LOG(ERROR) << "Cannot open the encoder";
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }

  // write headers
  r = x264_encoder_headers(encoder, &nals, &nheader);
  if(r < 0) {
    LOG(ERROR) << "x264_encoder_headers() failed";
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }

  header_size = nals[0].i_payload + nals[1].i_payload +nals[2].i_payload;
  // TODO
  //  if(!fwrite(nals[0].p_payload, header_size, 1, fp)) {
//    LOG(ERROR) << "Cannot write headers";
//    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
//  }
  return boost::system::error_code();
}

boost::system::error_code X264Codec::getOutputType(MediaTypeDescriptor& out)
{
  VLOG(2) << "X264Codec::getOutputType";
  out.m_eType = rtp_plus_plus::media::MediaTypeDescriptor::MT_VIDEO;
  out.m_eSubtype = rtp_plus_plus::media::MediaTypeDescriptor::MST_H264;
  out.m_uiWidth = m_in.getWidth();
  out.m_uiHeight = m_in.getHeight();
  m_out = out;
  return boost::system::error_code();
}

boost::system::error_code X264Codec::transform(const std::vector<MediaSample>& in, std::vector<MediaSample>& out, uint32_t& uiSize)
{
  VLOG(12) << "X264Codec::transform";
  assert (encoder);
  // we only handle one sample at a time
  assert(in.size() == 1);

  // set bitrate: --bitrate 3000 --vbv-maxrate 3000 --vbv-bufsize 125
#if 0
  Params.rc.i_rc_method = X264_RC_ABR ;
  Params.rc.i_bitrate = nBitRate*0.65/1000  ;
  Params.rc.i_vbv_buffer_size = nBitRate/1000;
  Params.rc.i_vbv_max_bitrate = nBitRate*0.65/1000 ;
  Params.rc.f_vbv_buffer_init = 1.0 ;
  Params.rc.f_rate_tolerance = 1.0 ;
#endif

  const MediaSample& mediaIn = in[0];
  const uint8_t* pBufferIn = mediaIn.getDataBuffer().data();
#if 1
  memcpy(pic_in.img.plane[0], (uint8_t*)pBufferIn, m_uiEncodingBufferSize);
#else
  pic_in.img.plane[0] = (uint8_t*)pBufferIn;
#endif
  pic_in.img.i_stride[0]   = m_in.getWidth();
  pic_in.img.i_stride[1]   = pic_in.img.i_stride[2] = m_in.getWidth() >> 1;  // const uint8_t* pBufferOut = m_encodingBuffer.data();

  int frame_size = x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out);
  if(frame_size)
  {
    std::ostringstream ostr;
    ostr << "Transform complete: in size: " << mediaIn.getPayloadSize() << " frame size: " << frame_size;
    uiSize = frame_size;
#if 0
    Buffer mediaData(new uint8_t[frame_size], frame_size);
    memcpy((char*)mediaData.data(), nals[0].p_payload, frame_size);
    MediaSample mediaSample;
    mediaSample.setData(mediaData);
    mediaSample.setNaluContainsStartCode(true);
    out.push_back(mediaSample);
#else
    ostr << " (";
    // we're configuring x264 in annex-b mode.
    // Each nalu has a start code which we will remove

    for (int i = 0; i < num_nals; ++i)
    {
      uint32_t uiStartCodeLen = 4;
      int8_t start_code_3[3] = {0, 0, 1};
      if (memcmp((char*)nals[i].p_payload, start_code_3, 3) == 0 )
      {
        uiStartCodeLen = 3;
      }
      else
      {
        assert((memcmp((char*)nals[i].p_payload + 1, start_code_3, 3) == 0 ));
      }

      x264_nal_t * pNal = nals + i;
      int nalu_size = pNal->i_payload - uiStartCodeLen;
      ostr << " " << nalu_size;
      Buffer mediaData(new uint8_t[nalu_size], nalu_size);
      memcpy((char*)mediaData.data(), nals[i].p_payload + uiStartCodeLen, nalu_size);
      MediaSample mediaSample;
      mediaSample.setData(mediaData);
      // we've removed the start code, setting start code hint instead
      // mediaSample.setNaluContainsStartCode(true);
      mediaSample.setStartCodeLengthHint(uiStartCodeLen);
      out.push_back(mediaSample);
    }
    ostr << ")";
#endif
    VLOG(6) << ostr.str();
  }

  //  if (info.eFrameType != videoFrameTypeSkip)
  //  {
  //    VLOG(2) << "Transform complete: in size: " << mediaIn.getPayloadSize() << " frame size: " << info.iFrameSizeInBytes
  //            << " Temporal Id: " << info.iTemporalId
  //            << " Sub seq id: " << info.iSubSeqId
  //            << " Layer num: " << info.iLayerNum
  //            << " Frame type: " << (int)info.eFrameType
  //            << " TS: " << info.uiTimeStamp;

  //    int len = 0;
  //    for (int i = 0; i < info.iLayerNum; ++i)
  //    {
  //      const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
  //      VLOG(2) << "Layer type: " << (int)layerInfo.uiLayerType
  //              << " temp id: " << (int)layerInfo.uiTemporalId
  //              << " spatial id: " << (int)layerInfo.uiSpatialId
  //              << " quality id: " << (int)layerInfo.uiQualityId
  //              << " nal count: " << layerInfo.iNalCount;

  //      uint8_t* pBuffer = layerInfo.pBsBuf;
  //      for (int j = 0; j < layerInfo.iNalCount; ++j)
  //      {
  //        uint32_t uiNaluLength = layerInfo.pNalLengthInByte[j];
  //        VLOG(2) << "Adding NALU of length " << uiNaluLength;
  //        len += uiNaluLength;
  //        Buffer mediaData(new uint8_t[uiNaluLength], uiNaluLength);
  //        memcpy((char*)mediaData.data(), pBuffer, uiNaluLength);
  //        MediaSample mediaSample;
  //        mediaSample.setData(mediaData);
  //        out.push_back(mediaSample);
  //        pBuffer += uiNaluLength;
  //      }
  //    }
  //    VLOG(2) << "Total length: " << len;

  return boost::system::error_code();
  //  }
  //  else
  //  {
  //    VLOG(2) << "Skip frame";
  //    return boost::system::error_code();
  //  }
}

boost::system::error_code X264Codec::setBitrate(uint32_t uiTargetBitrate)
{
  if (m_uiTargetBitrate != uiTargetBitrate)
  {
    m_uiTargetBitrate = uiTargetBitrate;

  #if 0
    configureParams();
    // close and re-open encoder
    x264_encoder_close(encoder);

    encoder = x264_encoder_open(&params);
    if(!encoder) {
      LOG(ERROR) << "Failed to  re-open the encoder";
      return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
    }
    return boost::system::error_code();

  #else
#if 0
  configureParams();
#else
    x264_param_t param;
    x264_encoder_parameters( encoder, &param );
    // use re-config
    param.rc.i_bitrate = m_uiTargetBitrate;
    param.rc.i_vbv_buffer_size = m_uiTargetBitrate;
    param.rc.i_vbv_max_bitrate = m_uiTargetBitrate*m_dCbrFactor;
#endif
    int res = x264_encoder_reconfig(encoder, &param);
    if (res < 0)
    {
      return boost::system::error_code(boost::system::errc::argument_out_of_domain, boost::system::generic_category());
    }
    else
    {
      return boost::system::error_code();
    }
  #endif
  }
  else
  {
    // NOOP
    return boost::system::error_code();
  }
}
