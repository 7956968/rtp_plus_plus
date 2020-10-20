#pragma once
#include <fstream>
#include <vector>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <rtp++/experimental/ICooperativeCodec.h>
#include <rtp++/media/IVideoDevice.h>
#include <rtp++/media/MediaSource.h>
#include <rtp++/media/GeneratedMediaSource.h>
#include <rtp++/media/ITransform.h>

namespace rtp_plus_plus
{
namespace media
{

// fwd
class MediaSink;

/**
 * @brief This virtual video device is used to get data into an rtp++ application.
 * Initially this only supported read NALUs from a raw annex B H.264 file.
 * It now supports .265 files as well as raw YUV files, and is able to apply tranforms
 * to the raw data.
 */
class VirtualVideoDeviceV2 : public IVideoDevice,
                             public ICooperativeCodec
{
  enum TimingMode
  {
    TM_LIVE,
    TM_FRAME_RATE
  };

public:
  /** 
   * @brief Named constructor for file-based sources
   */
  static std::shared_ptr<VirtualVideoDeviceV2> create(boost::asio::io_service& ioService,
                                                      const std::string& sDeviceName, const std::string &sMediaType,
                                                      uint32_t uiFps, bool bLimitRate = true, bool bLoop = true, uint32_t uiLoopCount = 0,
                                                      uint32_t uiWidth = 0, uint32_t uiHeight = 0,
                                                      const std::string& sVideoCodecImpl = "",
                                                      const std::vector<std::string>& videoCodecParameters = std::vector<std::string>());
  /** 
   * @brief Named constructor for generated sources.
   */
  static std::shared_ptr<VirtualVideoDeviceV2> create(boost::asio::io_service& ioService,
                                                      const std::string& sDeviceName, const std::string &sMediaType,
                                                      uint32_t uiFps, 
                                                      PayloadType ePayloadType, const std::string& sPacketSizes,
                                                      uint8_t uiCharPayload, uint32_t uiTotalVideoSamples);
  /**
   * @brief Constructor
   */
  explicit VirtualVideoDeviceV2(boost::asio::io_service& ioService,
                                const std::string& sDeviceName, const std::string &sMediaType,
                                uint32_t uiFps, bool bLimitRate = true, bool bLoop = true,
                                uint32_t uiLoopCount = 0, uint32_t uiWidth = 0, uint32_t uiHeight = 0,
                                const std::string& sVideoCodecImpl = "",
                                const std::vector<std::string>& videoCodecParameters = std::vector<std::string>());
  /**
   * @brief Constructor
   */
  explicit VirtualVideoDeviceV2(boost::asio::io_service& ioService,
    const std::string& sDeviceName, const std::string &sMediaType,
    uint32_t uiFps,
    PayloadType ePayloadType, const std::string& sPacketSizes,
    uint8_t uiCharPayload, uint32_t uiTotalVideoSamples);

  ~VirtualVideoDeviceV2();
  /**
   * @brief This method determines how samples are timestamped: either not at all, according to framerate or according to the system time.
   * @param [in] eMode Can be TM_NONE, TM_LIVE or TM_FRAME_RATE
   */
  void setMode(TimingMode eMode);

  virtual bool isRunning() const;
  virtual boost::system::error_code start();
  virtual boost::system::error_code stop();
  virtual boost::system::error_code getParameter(const std::string& sParamName, std::string& sParamValue);
  virtual boost::system::error_code setParameter(const std::string& sParamName, const std::string& sParamValue);
  /**
   * @brief @ICooperativeCodec
   */
  virtual boost::system::error_code setBitrate(uint32_t uiTargetBitrateKbps);

protected:
  void handleTimeout(const boost::system::error_code& ec);
  /**
   * @brief outputNal
   * @return
   */
  bool outputNal();
  /**
   * @brief transforms media samples using transforms
   */
  boost::system::error_code transform(const std::vector<MediaSample>& vMediaSamples, std::vector<MediaSample>& out, uint32_t& uiSize);

private:

  boost::asio::io_service& m_rIoService;
  boost::asio::deadline_timer m_timer;

  std::ifstream m_in1;
  std::unique_ptr<MediaSource> m_pSource;

  /**
   * @brief This enum configures how media samples are timestamped
   */
  TimingMode m_eMode;
  uint32_t m_uiFrameDurationUs;
  // TS of current AU
  double m_dCurrentAccessUnitPts;
  double m_dFrameDuration;
  bool m_bShuttingDown;

  // outgoing AU counter
  uint32_t m_uiAuCount;
  boost::posix_time::ptime m_tStart;

  // H264 parameters
  std::string m_sSps;
  std::string m_sPps;
  std::string m_sProfileLevelId;
  std::string m_sSPropParameterSets;

  std::vector<std::unique_ptr<ITransform>> m_transforms;

  media::MediaSink* m_pMediaSink;

  std::vector<uint32_t> m_vTestBitrates;
  int m_iSwitchFrame;
};

} // media
} // rtp_plus_plus

