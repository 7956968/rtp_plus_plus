#include "CorePch.h"
#include <rtp++/media/VirtualVideoDevice.h>
#include <functional>
#include <boost/asio/io_service.hpp>
#include <cpputil/MakeService.h>
#include <rtp++/application/ApplicationUtil.h>
#include <rtp++/media/h264/H264NalUnitTypes.h>
#include <rtp++/rfc6184/Rfc6184.h>
#include <rtp++/util/Base64.h>

namespace rtp_plus_plus
{
namespace media
{

std::shared_ptr<IVideoDevice> VirtualVideoDevice::create(boost::asio::io_service& ioService,
                                                         const std::string& sDeviceName, uint32_t uiFps)
{
  if (!FileUtil::fileExists(sDeviceName))
  {
    LOG(WARNING) << "Failed to initialise virtual video device from file: " << sDeviceName;
    return std::shared_ptr<IVideoDevice>();
  }
  else
  {
    return std::make_shared<VirtualVideoDevice>(ioService, sDeviceName, uiFps);
  }
}

VirtualVideoDevice::VirtualVideoDevice(boost::asio::io_service& ioService,
                                       const std::string& sDeviceName, uint32_t uiFps)
  :IVideoDevice(sDeviceName),
    m_source(ioService, boost::ref(m_in1), rfc6184::H264, 10000, true, 0, uiFps, true)
{
  bool success = app::ApplicationUtil::initialiseFileMediaSource(m_in1, sDeviceName);
  assert(success);

  m_source.setReceiveAccessUnitCB(
        std::bind(&VirtualVideoDevice::handleVideoAccessUnit, this, std::placeholders::_1));

  // there is a better/cleaer way to do this using the AsyncStreamMediaSource class but for now we are just going
  // to do it manually and then reset the input stream
  // This code assumed that the SPS and PPS are at the beginning of the stream
  assert (m_in1.good());

  const int bufferSize = 20000;
  uint8_t buffer[bufferSize];
  m_in1.read((char*)buffer, bufferSize);
  // now search for the SPS and the PPS
  using namespace media::h264;
  NalUnitType ePreviousType = NUT_UNSPECIFIED;
  int32_t iPreviousIndex = -1;

  char startCode[3] = { 0, 0, 1};
  for (size_t i = 0; i < bufferSize - 3; ++i)
  {
    if (memcmp(&buffer[i], startCode, 3) == 0)
    {
      if (ePreviousType == NUT_SEQUENCE_PARAMETER_SET)
      {
        assert (iPreviousIndex != -1);
        m_sSps = std::string((const char*)&(buffer[iPreviousIndex + 3]), (i - iPreviousIndex - 3));      
      }
      else if (ePreviousType == NUT_PICTURE_PARAMETER_SET)
      {
        assert (iPreviousIndex != -1);
        m_sPps = std::string((const char*)(&buffer[iPreviousIndex + 3]), (i - iPreviousIndex - 3));      
      }

      if (!m_sSps.empty() && !m_sPps.empty())
      {
        break;
      }
      ePreviousType = getNalUnitType(buffer[i + 3]);
      iPreviousIndex = i;
    }
  }

  if (m_sSps.empty() || m_sPps.empty() )
  {
    LOG(WARNING) << "Failed to find SPS and PPS in first 256 bytes of H264 file";
  }
  else
  {
    char fConfigStr[7];
    sprintf(fConfigStr, "%02X%02X%02X", m_sSps.at(1), m_sSps.at(2), m_sSps.at(3));
    m_sProfileLevelId = std::string((char*)fConfigStr, 6);
    m_sSPropParameterSets = base64_encode(m_sSps) + "," + base64_encode(m_sPps);
  }

  m_in1.clear();
  m_in1.seekg(0, std::ios_base::beg);
}

VirtualVideoDevice::~VirtualVideoDevice()
{
  if (m_in1.is_open())
    m_in1.close();
}

bool VirtualVideoDevice::isRunning() const
{
  return true;
//  assert(m_pStreamVideoService);
//  return m_pStreamVideoService->isRunning();
}

boost::system::error_code VirtualVideoDevice::start()
{
//  assert(m_pStreamVideoService);
//  m_pStreamVideoService->start();
  m_source.start();
  return boost::system::error_code();
}

boost::system::error_code VirtualVideoDevice::stop()
{
//  assert(m_pStreamVideoService);
//  if (m_pStreamVideoService->isRunning())
//  {
//    m_pStreamVideoService->stop();
//  }
  m_source.stop();
  return boost::system::error_code();
}

boost::system::error_code VirtualVideoDevice::getParameter(const std::string& sParamName, std::string& sParamValue)
{
  if (sParamName == rfc6184::PROFILE_LEVEL_ID)
  {
    if (m_sProfileLevelId.empty())
    {
      // we had failed to extract the SPS/PPS
      return boost::system::error_code(boost::system::errc::bad_file_descriptor, boost::system::get_generic_category());
    }
    else
    {
     sParamValue = m_sProfileLevelId;
     return boost::system::error_code();
    }
  }
  else if (sParamName == rfc6184::SPROP_PARAMETER_SETS)
  {
    if (m_sSPropParameterSets.empty())
    {
      // we had failed to extract the SPS/PPS
      return boost::system::error_code(boost::system::errc::bad_file_descriptor, boost::system::get_generic_category());
    }
    else
    {
      sParamValue = m_sSPropParameterSets;
      return boost::system::error_code();
    }
  }
  else
  {
    return boost::system::error_code(boost::system::errc::not_supported, boost::system::get_generic_category());
  }
}

boost::system::error_code VirtualVideoDevice::setParameter(const std::string& sParamName, const std::string& sParamValue)
{
  return boost::system::error_code(boost::system::errc::not_supported, boost::system::get_generic_category());
}

boost::system::error_code VirtualVideoDevice::handleVideoAccessUnit(const std::vector<MediaSample>& mediaSamples)
{
  VLOG(15) << "Video AU received TS: " << mediaSamples[0].getStartTime() << " Size: " << mediaSamples.size();
  m_accessUnitCB(mediaSamples);
  return boost::system::error_code();
}

void VirtualVideoDevice::handleEos(const boost::system::error_code& ec)
{
  VLOG(2) << "End of stream";
}

}
}
