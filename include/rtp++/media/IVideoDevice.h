#pragma once
#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <rtp++/media/MediaSample.h>

namespace rtp_plus_plus
{
namespace media 
{

// Callback for access units
typedef boost::function<boost::system::error_code (const std::vector<MediaSample>&)> ReceiveAccessUnitCb_t;
typedef boost::function<void (const boost::system::error_code&)> CompletionHandler_t;

// TODO: rename to IDevice as this can be used for audio and video
class IVideoDevice : public boost::noncopyable
{
public:

  virtual ~IVideoDevice()
  {
    VLOG(10) << "Destroying video device " << m_sName;
  }

  std::string getDeviceName() const { return m_sName; }

  void setReceiveAccessUnitCB(ReceiveAccessUnitCb_t accessUnitCB){ m_accessUnitCB = accessUnitCB; }
  void setCompletionHandler(CompletionHandler_t onComplete) { m_onComplete = onComplete; }

  virtual boost::system::error_code getParameter(const std::string& sParamName, std::string& sParamValue)
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::get_generic_category());
  }

  virtual boost::system::error_code setParameter(const std::string& sParamName, const std::string& sParamValue)
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::get_generic_category());
  }

  virtual bool isRunning() const = 0;

  virtual boost::system::error_code start() = 0;
  virtual boost::system::error_code stop() = 0;

protected:
  IVideoDevice(const std::string& sName)
    :m_sName(sName)
  {
    VLOG(10) << "Creating video device " << m_sName;
  }

  std::string m_sName;
  ReceiveAccessUnitCb_t m_accessUnitCB;
  // on end of file/stream
  CompletionHandler_t m_onComplete;
};

} // media
} // rtp_plus_plus

