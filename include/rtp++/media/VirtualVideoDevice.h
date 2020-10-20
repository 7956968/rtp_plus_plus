#pragma once
#include <fstream>
#include <rtp++/media/IVideoDevice.h>
#include <rtp++/media/AsyncStreamMediaSource.h>

namespace rtp_plus_plus
{
namespace media
{

/**
 * DEPRECATED: replaced by VirtualVideoDevice which uses NalUnitMediaSource
 * This virtual video device reads an Annex B H264 file and streams it.
 */

class VirtualVideoDevice : public IVideoDevice
{
public:

  static std::shared_ptr<IVideoDevice> create(boost::asio::io_service& ioService,
                                              const std::string& sDeviceName, uint32_t uiFps);
  VirtualVideoDevice(boost::asio::io_service& ioService,
                     const std::string& sDeviceName, uint32_t uiFps);
  ~VirtualVideoDevice();

  virtual bool isRunning() const;
  virtual boost::system::error_code start();
  virtual boost::system::error_code stop();
  virtual boost::system::error_code getParameter(const std::string& sParamName, std::string& sParamValue);
  virtual boost::system::error_code setParameter(const std::string& sParamName, const std::string& sParamValue);

protected:

  boost::system::error_code handleVideoAccessUnit(const std::vector<MediaSample>& mediaSamples);
  void handleEos(const boost::system::error_code& ec);

private:
  std::ifstream m_in1;
  AsyncStreamMediaSource m_source;

  // H264 parameters
  std::string m_sSps;
  std::string m_sPps;
  std::string m_sProfileLevelId;
  std::string m_sSPropParameterSets;
};

}
}

