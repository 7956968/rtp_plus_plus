#include "RtpWrapper.h"

// To prevent double inclusion of winsock on windows
#ifdef _WIN32
// To be able to use std::max
#define NOMINMAX
#include <WinSock2.h>
#endif

#ifdef _WIN32
// To get around compile error on windows: ERROR macro is defined
#define GLOG_NO_ABBREVIATED_SEVERITIES
#endif
#include <glog/logging.h>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/ref.hpp>

#include <cpputil/FileUtil.h>

#include <rtp++/media/h264/H264AnnexBStreamParser.h>
#include <rtp++/mediasession/MediaSessionDescription.h>
// #include <rtp++/mediasession/MediaSessionFactory.h>
// #include <rtp++/mediasession/MediaSessionManager.h>
#include <rtp++/mediasession/SimpleMediaSession.h>
#include <rtp++/mediasession/SimpleMediaSessionFactory.h>
#include <rtp++/network/AddressDescriptorParser.h>
#include <rtp++/rfc3550/Rfc3550.h>
#include <rtp++/rfc4566/SdpParser.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/application/ApplicationParameters.h>

#define DEFAULT_H264_BUFFER_SIZE 10000

using namespace rtp_plus_plus;
using media::MediaSample;

RtpWrapper::RtpWrapper()
  :m_buffer(new uint8_t[DEFAULT_H264_BUFFER_SIZE], DEFAULT_H264_BUFFER_SIZE),
    m_uiCurrentPos(0)
{
  // for now we are only handling H.264
  m_pMediaStreamParser = std::unique_ptr<media::h264::H264AnnexBStreamParser>(new media::h264::H264AnnexBStreamParser());
}

RtpWrapper::~RtpWrapper()
{

}

int RtpWrapper::start()
{
  // TODO: the VLC module should generate the SDP
  std::string sSdp("/home/rglobisch/vlc.sdp");
  std::string sRemoteConf("/home/rglobisch/vlc.cfg");

  // read address descriptor of local configuration
  std::string sRemoteCfg = FileUtil::readFile(sRemoteConf, false);
  LOG(INFO) << "Receiver address: " << sRemoteCfg;
  // we need to parse this address descriptor
  boost::optional<InterfaceDescriptions_t> remoteConf = AddressDescriptorParser::parse(sRemoteCfg);
  if (!remoteConf)
  {
    LOG(ERROR) << "Failed to read remote configuration";
    return RTPW_EC_GENERIC;
  }
  else
  {
    for (size_t i = 0; i != remoteConf->size(); ++i)
    {
      MediaInterfaceDescriptor_t mediaDescriptor = (*remoteConf)[i];
      for (size_t j = 0; j < mediaDescriptor.size(); ++j)
      {
        AddressDescriptor address = std::get<0>(mediaDescriptor[j]);
        LOG(INFO) << "Remote interface address: " << address.getAddress() << " " << address.getPort();
      }
    }
  }

  // read local SDP
  std::string sSdpContent = FileUtil::readFile(sSdp, false);
  LOG(INFO) << "SDP: " << sSdpContent;
  boost::optional<rfc4566::SessionDescription> localSdp = rfc4566::SdpParser::parse(sSdpContent);
  if (!localSdp)
  {
    LOG(ERROR) << "Failed to read local SDP";
    return RTPW_EC_GENERIC;
  }
  else
  {
    LOG(INFO) << "Media source: " << (*localSdp).getConnection().ConnectionAddress;
  }

  GenericParameters parameters;
  parameters.setUintParameter(app::ApplicationParameters::mtu, 1460);

  MediaSessionDescription mediaSessionDescription = MediaSessionDescription(rfc3550::getLocalSdesInformation(), *localSdp, *remoteConf, true);
  SimpleMediaSessionFactory factory;
  m_pMediaSession = factory.create(mediaSessionDescription, parameters);

  if (m_pMediaSession)
  {
    LOG(INFO) << "Starting media session manager thread";
    // start thread
    m_pThread = std::unique_ptr<boost::thread>(new boost::thread(boost::bind(&SimpleMediaSession::start, m_pMediaSession)));
    return RTPW_EC_NO_ERROR;
  }
  else
  {
    LOG(ERROR) << "Failed to add media session";
    return RTPW_EC_GENERIC;
  }
}

int RtpWrapper::stop()
{
  LOG(INFO) << "Stopping media session manager thread";
  m_pMediaSession->stop();
  LOG(INFO) << "Joining thread";
  m_pThread->join();
  LOG(INFO) << "Eventloop complete";
  return RTPW_EC_NO_ERROR;
}


int RtpWrapper::sendRtpPacket(uint8_t uiPayloadType, uint8_t* pBuffer, uint32_t uiBufferSize, uint64_t uiPts, uint64_t uiDts)
{
  LOG(INFO) << "PayloadType: " << (int)uiPayloadType << " Size: " << uiBufferSize << " PTS: " << uiPts << " DTS: " << uiDts;

#if 0
  static int iFile = 0;
  std::ostringstream ostr;
  ostr << "File_" << iFile++ << ".dat";
  FileUtil::writeFile(ostr.str(), (const char*)pBuffer, uiBufferSize, true);
#endif

  // calculate PTS
  lldiv_t res;
  res = lldiv (uiPts,1000000LL);

  double dPts = res.quot + res.rem/1000000.0;
  std::vector<MediaSample> vSamples = m_pMediaStreamParser->extractAll(pBuffer, uiBufferSize, dPts);
  m_pMediaSession->sendVideo(vSamples);

//  for (size_t i = 0; i < vSamples.size(); ++i)
//  {
//#if 0
//    const uint8_t* pData = vSamples[i]->getDataBuffer().data();
//    uint8_t uiNut = pData[0] & 0x1F;
//    rfc6184::NalUnitType eType = static_cast<rfc6184::NalUnitType>(uiNut);
//    VLOG(5) << "Sending NAL type: " << rfc6184::toString(eType) << " NUT: " << (uint32_t)uiNut;
//    LOG(INFO) << "Sending sample of size: " << vSamples[i]->getDataBuffer().getSize() << " Size: " << dPts;
//#endif
//    m_pMediaSession->sendVideo(m_uiId, uiPayloadType, vSamples[i]);
//  }

#if 0
  // check if there's enough space in the buffer for the read, else resize it
  if (m_buffer.getSize() - m_uiCurrentPos < uiBufferSize )
  {
    LOG(INFO) << "Resizing read buffer - left " << m_buffer.getSize() - m_uiCurrentPos;
    uint32_t uiBufferSize = m_buffer.getSize() * 2;
    uint8_t* pNewBuffer = new uint8_t[uiBufferSize];
    memcpy((char*)pNewBuffer, (char*)m_buffer.data(), m_uiCurrentPos);
    m_buffer.setData(pNewBuffer, uiBufferSize);
  }

  // copy new data
  memcpy((char*) m_buffer.data() + m_uiCurrentPos, pBuffer, uiBufferSize);
  m_uiCurrentPos += uiBufferSize;

  int32_t iSize = 0;
  // NOTE: iSize determines whether any bytes were consumed and should be moved up
  MediaSample::ptr pMediaSample = m_pMediaStreamParser->extract(m_buffer.data(), m_uiCurrentPos, iSize);
  while (iSize > 0)
  {
    if (pMediaSample == NULL)
    {
      LOG(WARNING) << "Media sample NULL - iSize: " << iSize;
      break;
    }
    LOG(INFO) << "Extracted sample from stream - size: " << iSize;
    m_pManager->sendMediaSample(m_uiId, uiPayloadType, pMediaSample);

    // copy remaining data to front of buffer
    memmove(&m_buffer[0], &m_buffer[iSize], m_uiCurrentPos - iSize);
    // update pointer for new reads
    m_uiCurrentPos = m_uiCurrentPos - iSize;

    pMediaSample = m_pMediaStreamParser->extract(m_buffer.data(), m_uiCurrentPos, iSize);
  }
#endif

  /*
  if (iSize == -1)
  {
      LOG(WARNING) << "EOS returned from media stream parser";
      m_bStopping = true;
      break;
  }
*/

  /*
  // make a copy of the media since out shared MediaSample pointer will automatically release the memory
  uint8_t* pBufferCopy = new uint8_t[uiBufferSize - uiStartCodeLen];
  memcpy(pBufferCopy, pBuffer + uiStartCodeLen, uiBufferSize - uiStartCodeLen);

  pMediaSample->setData(pBufferCopy, uiBufferSize);
  // TODO: convert PTS to RTP presentation time
  double dStartTime = 0.0;
  pMediaSample->setStartTime(dStartTime);
  // TODO: set marker bit?
  m_pManager->sendMediaSample(m_uiId, uiPayloadType, pMediaSample);
*/
  return RTPW_EC_NO_ERROR;
}
