#pragma once
#include <vector>
#include <boost/system/error_code.hpp>
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus {
namespace media {

class MediaTypeDescriptor
{
public:
  enum MediaType_t
  {
    MT_NOT_SET,
    MT_VIDEO,
    MT_AUDIO,
    MT_TEXT,
    MT_MUXED,
    MT_STREAM
  };
  enum MediaSubtype_t
  {
    MST_NOT_SET,
    MST_YUV_420P,
    MST_H264,
    MST_H264_SVC,
    MST_H265,
    MST_AMR
  };

  MediaTypeDescriptor(uint32_t uiWidth = 0, uint32_t uiHeight = 0, double dFps = 0.0)
    :m_eType(MT_NOT_SET),
      m_eSubtype(MST_NOT_SET),
      m_uiWidth(uiWidth),
      m_uiHeight(uiHeight),
      m_dFps(dFps)
  {

  }

  MediaTypeDescriptor(MediaType_t eType, MediaSubtype_t eSubType, uint32_t uiWidth = 0, uint32_t uiHeight = 0, double dFps = 0.0)
    :m_eType(eType),
      m_eSubtype(eSubType),
      m_uiWidth(uiWidth),
      m_uiHeight(uiHeight),
      m_dFps(dFps)
  {

  }

  uint32_t getWidth() const { return m_uiWidth; }
  uint32_t getHeight() const { return m_uiHeight; }
  double getFps() const { return m_dFps; }

  MediaType_t m_eType;
  MediaSubtype_t m_eSubtype;

  // if video
  uint32_t m_uiWidth;
  uint32_t m_uiHeight;
  double m_dFps;
  // TODO: audio parameters
};

class ITransform
{
public:
  virtual ~ITransform()
  {

  }
  /**
   * @brief sets input type of transform
   */
  virtual boost::system::error_code setInputType(const MediaTypeDescriptor& in) = 0;
  /**
   * @brief configures transform: should be called before initialise()
   */
  virtual boost::system::error_code configure(const std::string& sName, const std::string& sValue) = 0;
  /**
   * @brief configures transform: should be called after setInputType() and configure()
   */
  virtual boost::system::error_code initialise() = 0;
  /**
   * @brief gets output media type: should be called after initialise()
   */
  virtual boost::system::error_code getOutputType(MediaTypeDescriptor& out) = 0;
  /**
   * @brief executes transform
   */
  virtual boost::system::error_code transform(const std::vector<MediaSample>& in, std::vector<MediaSample>& out, uint32_t& uiSize) = 0;

protected:

private:
  MediaTypeDescriptor m_in;
  MediaTypeDescriptor m_out;
};

} // media
} // rtp_plus_plus
