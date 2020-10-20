#pragma once
#include <rtp++/media/IVideoCodecTransform.h>

#ifdef _WIN32
#ifdef OpenH264Codec_EXPORTS
#define Open_H264_API __declspec(dllexport)
#else
#define Open_H264_API __declspec(dllimport)
#endif
#else
#define Open_H264_API 
#endif

class ISVCEncoder;

class Open_H264_API OpenH264Codec : public rtp_plus_plus::media::IVideoCodecTransform
{
public:

  OpenH264Codec();
  ~OpenH264Codec();

  /**
   * @brief @ITransform
   */
  virtual boost::system::error_code setInputType(const rtp_plus_plus::media::MediaTypeDescriptor& in);
  /**
   * @brief @ITransform
   */
  virtual boost::system::error_code configure(const std::string& sName, const std::string& sValue);
  /**
   * @brief @ITransform
   */
  virtual boost::system::error_code initialise();
  /**
   * @brief @ITransform
   */
  virtual boost::system::error_code getOutputType(rtp_plus_plus::media::MediaTypeDescriptor& out);
  /**
   * @brief @ITransform
   */
  virtual boost::system::error_code transform(const std::vector<rtp_plus_plus::media::MediaSample>& in,
                                              std::vector<rtp_plus_plus::media::MediaSample>& out,
                                              uint32_t& uiSize);
  /**
   * @brief @ICooperativeCodec
   */
  virtual boost::system::error_code setBitrate(uint32_t uiTargetBitrate);

private:

  rtp_plus_plus::media::MediaTypeDescriptor m_in;
  rtp_plus_plus::media::MediaTypeDescriptor m_out;
  
  ISVCEncoder* m_pCodec;
  uint32_t m_uiTargetBitrate;

  uint32_t m_uiEncodingBufferSize;
  Buffer m_encodingBuffer;
};

