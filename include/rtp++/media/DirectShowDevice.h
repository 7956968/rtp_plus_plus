#pragma once
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <rtp++/media/IVideoDevice.h>

namespace rtp_plus_plus
{
namespace media 
{

/**
 * @brief Abstraction for DirectShow device
 */
class DirectShowDevice : public IVideoDevice,
  public std::enable_shared_from_this<DirectShowDevice>
{
public:

  static std::shared_ptr<DirectShowDevice> create(boost::asio::io_service& ioService, const std::string& sDeviceName)
  {
    return std::make_shared<DirectShowDevice>(ioService, sDeviceName);
  }

  DirectShowDevice(boost::asio::io_service& ioService, const std::string& sName)
    :IVideoDevice(sName),
    m_ioService(ioService),
    m_bIsRunning(false)
  {
    VLOG(10) << "Creating DirectShowDevice: " << sName;
  }

  virtual ~DirectShowDevice()
  {
    VLOG(10) << "Destroying DirectShowDevice: " << m_sName;
  }

  virtual boost::system::error_code getParameter(const std::string& sParamName, std::string& sParamValue)
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::get_generic_category());
  }

  virtual boost::system::error_code setParameter(const std::string& sParamName, const std::string& sParamValue)
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::get_generic_category());
  }

  /**
   * @brief Delivers media samples in io_service thread
   */
  void deliver(const std::vector<MediaSample>& mediaSamples)
  {
    if (m_bIsRunning)
      m_ioService.post(boost::bind(&DirectShowDevice::doDeliver, shared_from_this(), mediaSamples));
  }

  virtual bool isRunning() const
  {
    return m_bIsRunning;
  }

  virtual boost::system::error_code start()
  {
    // NOOP
    VLOG(2) << "DirectShowDevice start";
    m_bIsRunning = true;
    return boost::system::error_code();
  }
  virtual boost::system::error_code stop()
  {
    // NOOP
    VLOG(2) << "DirectShowDevice stop";
    m_bIsRunning = false;
    return boost::system::error_code();
  }

private:

private:
  void doDeliver(const std::vector<MediaSample>& mediaSamples)
  {
    if (m_accessUnitCB)
      m_accessUnitCB(mediaSamples);
  }

  boost::asio::io_service& m_ioService;
  bool m_bIsRunning;
};

} // media
} // rtp_plus_plus

