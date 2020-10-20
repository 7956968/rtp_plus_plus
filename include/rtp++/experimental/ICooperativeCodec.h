#pragma once
#include <boost/system/error_code.hpp>

namespace rtp_plus_plus {

/**
 * @brief The ICooperativeCodec class allows the network and scheduling layer to communicate with the codec layer
 */
class ICooperativeCodec
{
public:

  virtual ~ICooperativeCodec()
  {

  }

  enum BitrateTargetPreciseness
  {
    BT_PRECISE_FRAME,  // target bitrate is precise per generated frame
    BT_PRECISE_AVERAGE // target bitrate is averaged over a period
  };

  virtual boost::system::error_code setBitrate(uint32_t uiTargetBitrate) = 0;
#if 0
  virtual boost::system::error_code switchBitrate(uint32_t uiSwitchType) = 0;
  virtual boost::system::error_code generateIdr() = 0;
  virtual boost::system::error_code setMaxFrameSize(uint32_t uiMaxSizeBytesBytes) = 0;
  virtual boost::system::error_code setFramerate(uint32_t uiFramerate) = 0;
  virtual boost::system::error_code changeFramerate(uint32_t uiFramerate) = 0;
  virtual boost::system::error_code changeGopStructure() = 0;
  virtual boost::system::error_code updateReferencePicture() = 0;
#endif

private:

};

} // rtp_plus_plus
