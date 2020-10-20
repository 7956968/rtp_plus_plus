#pragma once
#include <rtp++/media/IVideoCodecTransform.h>

#ifdef _WIN32
#ifdef X265Codec_EXPORTS
#define X265_API __declspec(dllexport)
#else
#define X265_API __declspec(dllimport)
#endif
#else
#define X265_API 
#endif

struct x265_picture;
struct x265_param;
struct x265_nal;
struct x265_encoder;

class X265_API X265Codec : public rtp_plus_plus::media::IVideoCodecTransform
{
public:

  X265Codec();
  ~X265Codec();
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

  rtp_plus_plus::media::MediaTypeDescriptor m_in;
  rtp_plus_plus::media::MediaTypeDescriptor m_out;
  
  uint8_t* pBufferIn;
  x265_picture* pic_in;
  x265_picture* pic_out;
  x265_param* params;
  x265_nal* nals;
  x265_encoder* encoder;
  int num_nals;  
  uint32_t m_uiIFramePeriod;
  uint32_t m_uiCurrentFrame;
  uint32_t m_uiFrameBitLimit;
  bool m_bNotifyOnIFrame;

  uint32_t m_uiEncodingBufferSize;
  Buffer m_encodingBuffer;

  uint32_t m_uiTargetBitrate;
  std::string m_sTune;
  std::string m_sPreset;
};
