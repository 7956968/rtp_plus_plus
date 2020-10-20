#include "CorePch.h"
#include "VppH264Codec.h"
#include <ICodecv2.h>
#include <H264v2.h>
#include <cpputil/Conversion.h>

using namespace rtp_plus_plus::media;

using rtp_plus_plus::media::MediaTypeDescriptor;

#define D_IN_COLOUR_RGB24         "0"
#define D_IN_COLOUR_YUV420P       "16"
#define D_IN_COLOUR_YUV420P8      "17"

#define D_MAX_QUALITY             31 // ERROR, double check?
#define D_MAX_QUALITY_H264        51

#define D_IMAGE_WIDTH             "width"
#define D_IMAGE_HEIGHT            "height"
#define D_QUALITY                 "quality"
#define D_IN_COLOUR               "incolour"
#define D_OUT_COLOUR              "outcolour"
#define D_TARGET_WIDTH            "targetwidth"
#define D_TARGET_HEIGHT           "targetheight"
#define D_TARGET_FILE             "targetfile"
#define D_TOP_CROP                "top"
#define D_BOTTOM_CROP             "bottom"
#define D_LEFT_CROP               "left"
#define D_RIGHT_CROP              "right"
#define D_X_FRAMES_PER_SECOND     "xframespersecond"
#define D_FRAMES_PER_X_SECONDS    "framesperxseconds"
#define D_CONCAT_ORIENTATION      "concatorientation"
#define D_MODE_OF_OPERATION       "modeofoperation"
#define D_MODE_OF_OPERATION_H264  "mode of operation"
#define D_RATE_CONTROL_MODEL_TYPE "rate control model type"
#define D_FRAME_BIT_LIMIT         "framebitlimit"
#define D_STARTCHANNEL            "startchannel"
#define D_NOTIFYONIFRAME          "notifyoniframe"
#define D_SWITCH_FRAME_PERIOD     "switchframeperiod"
#define D_STREAM_USING_TCP        "streamusingtcp"
#define D_I_PICTURE_MULTIPLIER    "ipicturemultiplier"
#define D_AUTO_IFRAME_DETECT_FLAG "autoiframedetectflag"
#define D_IFRAME_PERIOD           "iframeperiod"
#define D_USE_MS_H264             "usems264"
#define D_H264_TYPE               "h264type"

// Rate control-related parameters
#define D_MAX_BITS_PER_FRAME        "max bits per frame"
#define D_NUM_RATE_CONTROL_FRAMES   "num rate control frames"
#define D_MAX_DISTORTION            "max distortion"
#define D_IPICTURE_DMAX_MULTIPLIER  "ipicture dmax multiplier"
#define D_IPICTURE_DMAX_FRACTION    "ipicture dmax fraction"
#define D_MINIMUM_INTRA_QP          "minimum intra qp"
#define D_MINIMUM_INTER_QP          "minimum inter qp"
#define D_RATE_OVERSHOOT_PERCENT    "rate overshoot percent"

bool configureH264CodecParameters(ICodecv2* pCodec)
{
  if (!pCodec) return false;
  //Set default quality to 16
  pCodec->SetParameter(D_QUALITY, "16");
  //16 = YUV
  pCodec->SetParameter(D_IN_COLOUR, D_IN_COLOUR_YUV420P8);
  //16 = YUV
  //pCodec->SetParameter(D_OUT_COLOUR, "16");
  // This multiplies the bit size of every i-frame by 2
  pCodec->SetParameter(D_I_PICTURE_MULTIPLIER, "1");

  pCodec->SetParameter(D_SWITCH_FRAME_PERIOD, "12");

  // using Keith's new rate control
  pCodec->SetParameter(D_MODE_OF_OPERATION_H264, "2");

  // default rate control values
  pCodec->SetParameter(D_MAX_BITS_PER_FRAME, "1000000");
  pCodec->SetParameter(D_NUM_RATE_CONTROL_FRAMES, "16");
  pCodec->SetParameter(D_MAX_DISTORTION, "30000");
  pCodec->SetParameter(D_IPICTURE_DMAX_MULTIPLIER, "2");
  pCodec->SetParameter(D_IPICTURE_DMAX_FRACTION, "0");
  pCodec->SetParameter(D_MINIMUM_INTRA_QP, "16");
  pCodec->SetParameter(D_MINIMUM_INTER_QP, "4");
  pCodec->SetParameter(D_RATE_OVERSHOOT_PERCENT, "100");

  // always flip the image on windows to get it into the desired YUV format
  pCodec->SetParameter("flip", "1");
  // NB: New codec settings for auto-iframe detection: These settings need to correlate to the settings of the DECODER
  pCodec->SetParameter("unrestrictedmotion", "1");
  pCodec->SetParameter("extendedpicturetype", "1");
  pCodec->SetParameter("unrestrictedmotion", "1");
  pCodec->SetParameter("advancedintracoding", "1");
  pCodec->SetParameter("alternativeintervlc", "1");
  pCodec->SetParameter("modifiedquant", "1");
  pCodec->SetParameter("autoipicture", "1");
  pCodec->SetParameter("ipicturefraction", "0");

  return true;
}

#define RCMT_QUAD 0
#define RCMT_POW 1
#define RCMT_LOG 2


VppH264Codec::VppH264Codec()
  :m_pCodec(nullptr),
    m_uiIFramePeriod(0),
    m_uiCurrentFrame(0),
    m_uiFrameBitLimit(0),
    m_uiMode(2),
    m_uiRateControlModelType(RCMT_POW),
    m_bNotifyOnIFrame(false),
    m_uiEncodingBufferSize(0)
{
  H264v2Factory factory;
  m_pCodec = factory.GetCodecInstance();
}

VppH264Codec::~VppH264Codec()
{
  assert(m_pCodec);
  m_pCodec->Close();
  H264v2Factory factory;
  factory.ReleaseCodecInstance(m_pCodec);
}

boost::system::error_code VppH264Codec::setInputType(const MediaTypeDescriptor& in)
{
  VLOG(2) << "VppH264Codec::setInputType";
  if (in.m_eSubtype != rtp_plus_plus::media::MediaTypeDescriptor::MST_YUV_420P)
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }

  if (in.getHeight() == 0 || in.getWidth() == 0 || in.getFps() == 0.0)
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
  }

  m_in = in;

  m_uiEncodingBufferSize = in.getWidth() * in.getHeight() * 1.5;
  VLOG(2) << "Encoding buffer size: " << m_uiEncodingBufferSize;

  m_encodingBuffer.setData(new uint8_t[m_uiEncodingBufferSize], m_uiEncodingBufferSize);
  return boost::system::error_code();
}

boost::system::error_code VppH264Codec::configure(const std::string& sName, const std::string& sValue)
{
  if (sName == "bitrate")
  {
    bool bDummy;
    uint32_t uiKbps = convert<uint32_t>(sValue, bDummy);
    assert(bDummy);
    assert(m_in.getFps() != 0.0);
    m_uiFrameBitLimit = static_cast<uint32_t>((uiKbps * 1000) / m_in.getFps());
    VLOG(2) << "Target bitrate: " << uiKbps  << "kbps - setting frame bit limit to " << m_uiFrameBitLimit;
    return boost::system::error_code();
  }
  else if (sName == "mode")
  {
    bool bDummy;
    uint32_t uiMode = convert<uint32_t>(sValue, bDummy);
    assert(bDummy);
    if (uiMode >= 0 && uiMode <= 2)
    {
      VLOG(2) << "Mode of operation: " << m_uiMode;
      m_uiMode = uiMode;
      return boost::system::error_code();
    }
    else
    {
      LOG(ERROR) << "Unsupported mode of operation: " << m_uiMode;
      return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
    }
  }
  else if (sName == "rcmt")
  {
    bool bDummy;
    uint32_t uiMode = convert<uint32_t>(sValue, bDummy);
    assert(bDummy);
    if (uiMode >= 0 && uiMode <= 2)
    {
      m_uiRateControlModelType = uiMode;
      VLOG(2) << "Rate control model type: " << m_uiRateControlModelType;
      return boost::system::error_code();
    }
    else
    {
      LOG(ERROR) << "Unsupported rate control model type: " << m_uiRateControlModelType;
      return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
    }
  }
  else
  {
    // try configuring mode on codec
    int res = m_pCodec->SetParameter(sName.c_str(), sValue.c_str());
    if (res == 1)
    {
      return boost::system::error_code();
    }
    else
    {
      return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
    }
  }
  return boost::system::error_code(boost::system::errc::not_supported, boost::system::generic_category());
}

boost::system::error_code VppH264Codec::initialise()
{
  m_pCodec->Close();

  bool res = configureH264CodecParameters(m_pCodec);
  assert(res);
  int success = m_pCodec->SetParameter(D_IMAGE_WIDTH, toString(m_in.getWidth()).c_str());
  assert(success);
  success = m_pCodec->SetParameter(D_IMAGE_HEIGHT, toString(m_in.getHeight()).c_str());
  assert(success);
  success = m_pCodec->SetParameter(D_MODE_OF_OPERATION_H264, toString(m_uiMode).c_str());
  assert(success);
  success = m_pCodec->SetParameter(D_RATE_CONTROL_MODEL_TYPE, toString(m_uiRateControlModelType).c_str());
  assert(success);

#if 0
  const uint32_t uiFps = 30;
  m_uiFrameBitLimit = 500000 / uiFps;
#endif

  // TODO: for parameter set generation
#if 0
  m_pCodec->SetParameter((char *)"seq param set", "0");
  m_pCodec->SetParameter((char *)"pic param set", "0");

  m_pCodec->SetParameter((char *)("generate param set on open"), "0");
  m_pCodec->SetParameter((char *)("picture coding type"), "2");       ///< Seq param set = H264V2_SEQ_PARAM.
  m_pSeqParamSet = new unsigned char[100];
  if (!m_pCodec->Code(NULL, m_pSeqParamSet, 100 * 8))
  {
    if (m_pSeqParamSet) delete[] m_pSeqParamSet; m_pSeqParamSet = NULL;
    SetLastError(m_pCodec->GetErrorStr(), true);
    return hr;
  }
  else
  {
    m_uiSeqParamSetLen = m_pCodec->GetCompressedByteLength();
    m_pSeqParamSet[m_uiSeqParamSetLen] = 0;
    m_sSeqParamSet = std::string((const char*)m_pSeqParamSet, m_uiSeqParamSetLen);
    int nCheck = strlen((char*)m_pSeqParamSet);
  }

  m_pCodec->SetParameter((char *)("picture coding type"), "3"); ///< Pic param set = H264V2_PIC_PARAM.
  m_pPicParamSet = new unsigned char[100];
  if (!m_pCodec->Code(NULL, m_pPicParamSet, 100 * 8))
  {
    if (m_pSeqParamSet) delete[] m_pSeqParamSet; m_pSeqParamSet = NULL;
    if (m_pPicParamSet) delete[] m_pPicParamSet; m_pPicParamSet = NULL;
    SetLastError(m_pCodec->GetErrorStr(), true);
    return E_FAIL;
  }
  else
  {
    m_uiPicParamSetLen = m_pCodec->GetCompressedByteLength();
    m_pPicParamSet[m_uiPicParamSetLen] = 0;
    m_sPicParamSet = std::string((const char*)m_pPicParamSet, m_uiPicParamSetLen);
    int nCheck = strlen((char*)m_pPicParamSet);
  }

  // RG: copied from codec anayser, do we need this?
  // reset codec for standard operation
  m_pCodec->SetParameter((char *)("picture coding type"), "0"); ///< I-frame = H264V2_INTRA.
  m_pCodec->SetParameter((char *)("generate param set on open"), "1");

  if (!m_pCodec->Open())
  {
    //Houston: we have a failure
    char* szErrorStr = m_pCodec->GetErrorStr();
    printf("%s\n", szErrorStr);
    SetLastError(szErrorStr, true);
    return E_FAIL;
  }
#endif

  m_pCodec->SetParameter((char *)("picture coding type"), "0"); ///< I-frame = H264V2_INTRA.
  m_pCodec->SetParameter((char *)("generate param set on open"), "1");

  if (!m_pCodec->Open())
  {
    return boost::system::error_code(boost::system::errc::argument_out_of_domain, boost::system::generic_category());
  }

  return boost::system::error_code();
}

boost::system::error_code VppH264Codec::getOutputType(MediaTypeDescriptor& out)
{
  VLOG(2) << "VppH264Codec::getOutputType";
  out.m_eType = rtp_plus_plus::media::MediaTypeDescriptor::MT_VIDEO;
  out.m_eSubtype = rtp_plus_plus::media::MediaTypeDescriptor::MST_H264;
  out.m_uiWidth = m_in.m_uiWidth;
  out.m_uiHeight = m_in.m_uiHeight;
  m_out = out;
  return boost::system::error_code();
}

boost::system::error_code VppH264Codec::transform(const std::vector<MediaSample>& in, std::vector<MediaSample>& out, uint32_t& uiSize)
{
  VLOG(12) << "VppH264Codec::transform";
  assert(m_pCodec);
  // we only handle one sample at a time
  assert(in.size() == 1);
  const MediaSample& mediaIn = in[0];
  const uint8_t* pBufferIn = mediaIn.getDataBuffer().data();
  const uint8_t* pBufferOut = m_encodingBuffer.data();

  if (m_pCodec->Ready())
  {
    if (m_uiIFramePeriod)
    {
      ++m_uiCurrentFrame;
      if (m_uiCurrentFrame%m_uiIFramePeriod == 0)
      {
        m_pCodec->Restart();
      }
    }

    int nFrameBitLimit = 0;
    if (m_uiFrameBitLimit == 0)
      // An encoded frame can never be bigger than an raw format frame
      nFrameBitLimit = m_uiEncodingBufferSize * 8 /*8 bits*/;
    else
      nFrameBitLimit = m_uiFrameBitLimit;

    int MAX_ATTEMPTS = 5;
    int iAttempts = 0;
    do
    {
    VLOG(12) << "Coding with frame bit limit to " << nFrameBitLimit;
    // #define MEASURE_ENCODING_TIME
#ifdef MEASURE_ENCODING_TIME
    boost::posix_time::ptime tStart = boost::posix_time::microsec_clock::universal_time();
#endif
    int nResult = m_pCodec->Code((void*)pBufferIn, (void*)pBufferOut, nFrameBitLimit);
#ifdef MEASURE_ENCODING_TIME
    boost::posix_time::ptime tEnd = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::time_duration diff = tEnd - tStart;
    VLOG(2) << "Time to encode: " << diff.total_milliseconds() << "ms";
#endif
    if (nResult)
    {
      //Encoding was successful
      int iEncodedLength = m_pCodec->GetCompressedByteLength();
      VLOG(6) << "Transform complete: in size: " << mediaIn.getPayloadSize() << " out size: " << iEncodedLength;
      uiSize = (uint32_t)iEncodedLength;

      // check if an i-frame was encoded
      int nLen = 0;
      char szBuffer[20];
      m_pCodec->GetParameter("last pic coding type",&nLen, szBuffer );
      if (strcmp(szBuffer, "0") == 0 /*H264V2_INTRA*/)
      {
        VLOG(12) << "I-Frame";
        const uint8_t* pData = pBufferOut;
        std::vector<uint32_t> lengths;
        std::vector<uint32_t> indices;
        for (int i = 0; i < iEncodedLength - 4; ++i)
        {
          uint8_t startCode[3] = { 0, 0, 1 };
          if (memcmp(startCode, pData + i, 3) == 0)
          {
            indices.push_back(i + 3);
            if ((i > 0) && ((int)pData[i - 1] == 0))
            {
              // four byte start code
              lengths.push_back(4);
            }
            else
            {
              // three byte start code
              lengths.push_back(3);
            }
            VLOG(12) << "Found NAL start index: " << indices[indices.size()-1] << " SC len: " << lengths[lengths.size() -1 ];
          }
        }
        indices.push_back(iEncodedLength);
        lengths.push_back(0);

        for (size_t i = 0; i < indices.size() - 1; ++i)
        {
          int iLength = indices[i + 1] - indices[i] - lengths[i + 1];
          VLOG(12) << i << " adding NAL of length: " << iLength << " start code len: " << lengths[i] << " index: " << indices[i];
          Buffer mediaData(new uint8_t[iLength], iLength);
          //NB: do we need to skip start code?
          memcpy((char*)mediaData.data(), (char*)(&pData[indices[i]]), iLength);
          MediaSample mediaSample;
          mediaSample.setData(mediaData);
          mediaSample.setStartCodeLengthHint(lengths[i]);
          out.push_back(mediaSample);
        }
      }
      else
      {
        // The VPP codec only outputs one NALU per P-frame
        VLOG(12) << "P-Frame";
        const uint8_t* pData = pBufferOut;
        uint8_t startCode[3] = { 0, 0, 1 };
        int iLength = 0;
        if (memcmp(startCode, pData, 3) == 0)
        {
          // 3 byte start code
          iLength = 3;
        }
        else if (memcmp(startCode, pData + 1, 3) == 0)
        {
          iLength = 4;
        }
        else
        {
          LOG(ERROR) << "Unable to find start code in NALU";
        }
        Buffer mediaData(new uint8_t[iEncodedLength - iLength], iEncodedLength - iLength);
        //NB: do we need to skip start code?
        memcpy((char*)mediaData.data(), (char*)(m_encodingBuffer.data() + iLength), iEncodedLength - iLength);
        MediaSample mediaSample;
        mediaSample.setData(mediaData);
        out.push_back(mediaSample);
        mediaSample.setStartCodeLengthHint(iLength);
      }
      return boost::system::error_code();
    }
    else
    {
      //An error has occurred
      std::string sError = m_pCodec->GetErrorStr();
      sError += ". Requested frame bit limit=" + toString(nFrameBitLimit) + ".";
      m_pCodec->Restart();
      LOG(ERROR) << sError;
      nFrameBitLimit = nFrameBitLimit * 1.5; // increase frame bit limit
      ++iAttempts;
    }
    } while (iAttempts < MAX_ATTEMPTS);

    return boost::system::error_code(boost::system::errc::argument_out_of_domain, boost::system::generic_category());
  }
  else
  {
    LOG(WARNING) << "Error, codec not ready!";
    return boost::system::error_code(boost::system::errc::argument_out_of_domain, boost::system::generic_category());
  }
}

boost::system::error_code VppH264Codec::setBitrate(uint32_t uiTargetBitrate)
{
  assert(m_in.getFps() != 0.0);
  m_uiFrameBitLimit = static_cast<uint32_t>((uiTargetBitrate * 1000) / m_in.getFps());
  VLOG(2) << "Target bitrate: " << uiTargetBitrate  << "kbps - setting frame bit limit to " << m_uiFrameBitLimit;
  return boost::system::error_code();
  // return boost::system::error_code(boost::system::errc::argument_out_of_domain, boost::system::generic_category());
}
