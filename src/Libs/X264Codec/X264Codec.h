#pragma once
#include <rtp++/media/IVideoCodecTransform.h>
#include <x264.h>

class X264Codec : public rtp_plus_plus::media::IVideoCodecTransform
{
public:

  X264Codec();
  ~X264Codec();

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
  virtual boost::system::error_code transform(const std::vector<rtp_plus_plus::media::MediaSample>& in, std::vector<rtp_plus_plus::media::MediaSample>& out, uint32_t& uiSize);
  /**
   * @brief @ICooperativeCodec
   */
  virtual boost::system::error_code setBitrate(uint32_t uiTargetBitrate);

private:
  void configureParams();

  rtp_plus_plus::media::MediaTypeDescriptor m_in;
  rtp_plus_plus::media::MediaTypeDescriptor m_out;
  
  x264_picture_t pic_in;
  x264_picture_t pic_out;
  x264_param_t params;
  x264_nal_t* nals;
  x264_t* encoder;
  int num_nals;

  uint32_t m_uiTargetBitrate;

  uint32_t m_uiEncodingBufferSize;
  Buffer m_encodingBuffer;

  uint32_t m_uiMode;
  double m_dCbrFactor;
  uint32_t m_uiMaxSliceSize;
};

