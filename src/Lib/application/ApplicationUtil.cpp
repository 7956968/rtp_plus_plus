#include "CorePch.h"
#ifndef WIN32
#include <signal.h>
#endif

#include <sstream>
#include <rtp++/application/ApplicationUtil.h>
#include <rtp++/Version.h>

namespace rtp_plus_plus
{
namespace app
{

using media::MediaSample;

void ApplicationUtil::printApplicationInfo(int argc, char** argv)
{
  std::ostringstream ostr;
  ostr << "(rtp++ v" << RTP_PLUS_PLUS_VERSION_STRING << ") ";
  for (int i = 0; i < argc; ++i)
    ostr << argv[i] << " ";

  LOG(INFO) << ostr.str();
}

std::vector<InterfaceDescriptions_t> ApplicationUtil::readNetworkConfigurationFiles(const std::vector<std::string>& configFiles, bool& bError, bool bRtcpMux)
{
  std::vector<InterfaceDescriptions_t> vDescriptions;
  for (const std::string& sFile : configFiles)
  {
    boost::optional<InterfaceDescriptions_t> description = ApplicationUtil::readNetworkConfigurationFile(sFile, bRtcpMux);
    if (description)
      vDescriptions.push_back(*description);
    else
      bError = true;
  }
  return vDescriptions;
}

boost::optional<InterfaceDescriptions_t> ApplicationUtil::readNetworkConfigurationFile(const std::string& sConfigFile, bool bRtcpMux)
{
  // read address descriptor of local configuration
  std::string sContent = FileUtil::readFile(sConfigFile, false);
  VLOG(2) << "Receiver address: " << sContent;
  // we need to parse this address descriptor
  return ApplicationUtil::parseNetworkConfiguration(sContent, bRtcpMux);
}

boost::optional<InterfaceDescriptions_t> ApplicationUtil::parseNetworkConfiguration(const std::string& sConfig, bool bRtcpMux)
{
  // we need to parse this address descriptor
  boost::optional<InterfaceDescriptions_t> remoteConf = AddressDescriptorParser::parse(sConfig, bRtcpMux);
  if (!remoteConf)
  {
    LOG(ERROR) << "Failed to parse network configuration";
    return boost::optional<InterfaceDescriptions_t>();
  }
  else
  {
    for (size_t i = 0; i != remoteConf->size(); ++i)
    {
      MediaInterfaceDescriptor_t mediaDescriptor = (*remoteConf)[i];
      for (size_t j = 0; j < mediaDescriptor.size(); ++j)
      {
        AddressDescriptor address = std::get<0>(mediaDescriptor[j]);
        VLOG(2) << "Interface address: " << address.getAddress() << " " << address.getPort();
      }
    }
    return remoteConf;
  }
}

boost::optional<rfc4566::SessionDescription> ApplicationUtil::readSdp(const std::string& sSdpFile, bool bVerbose)
{
  // read local SDP
  std::string sSdpContent = FileUtil::readFile(sSdpFile, false);
  VLOG(2) << "SDP: " << sSdpContent;
  boost::optional<rfc4566::SessionDescription> sdp = rfc4566::SdpParser::parse(sSdpContent, bVerbose);
  if (!sdp)
  {
    LOG(ERROR) << "Failed to read SDP: " << sSdpFile;
    return boost::optional<rfc4566::SessionDescription>();
  }
  else
  {
    VLOG(2) << "SDP Media source: " << (*sdp).getConnection().ConnectionAddress;
    return sdp;
  }
}

std::vector<rfc4566::SessionDescription> ApplicationUtil::readSdps(const std::vector<std::string>& vSdpFiles, bool bVerbose, bool& bError)
{
  std::vector<rfc4566::SessionDescription> vSdps;
  for (const std::string& sSdp : vSdpFiles)
  {
    boost::optional<rfc4566::SessionDescription> sdp = ApplicationUtil::readSdp(sSdp, bVerbose);
    if (sdp)
      vSdps.push_back(*sdp);
    else
      bError = true;
  }
  return vSdps;
}

bool ApplicationUtil::checkAndInitialiseSource(const std::string& sSource, bool& bGen, bool& bStdin, std::ifstream& in1)
{
  bGen = false;
  bStdin = false;
  if (sSource == "stdin")
  {
    bStdin = true;
  }
  else if (sSource == "gen")
  {
    bGen = true;
  }
  else if (!ApplicationUtil::initialiseFileMediaSource(in1, sSource))
  {
    return false;
  }
  return true;
}

void ApplicationUtil::handleIncomingH264MediaSample(const MediaSample& mediaSample, std::ostream& out)
{
  media::h264::H264AnnexBStreamWriter::writeMediaSampleNaluToStream(mediaSample, out, true);
}

void ApplicationUtil::handleIncomingH265MediaSample(const MediaSample& mediaSample, std::ostream& out)
{
  media::h265::H265AnnexBStreamWriter::writeMediaSampleNaluToStream(mediaSample, out, true);
}

void ApplicationUtil::handleIncomingH264AccessUnit(const std::vector<MediaSample>& mediaSamples, std::ostream& out)
{
  for (const MediaSample& mediaSample: mediaSamples)
  {
    media::h264::H264AnnexBStreamWriter::writeMediaSampleNaluToStream(mediaSample, out, true);
  }
}

void ApplicationUtil::handleIncomingH265AccessUnit(const std::vector<MediaSample>& mediaSamples, std::ostream& out)
{
  for (const MediaSample& mediaSample: mediaSamples)
  {
    media::h265::H265AnnexBStreamWriter::writeMediaSampleNaluToStream(mediaSample, out, true);
  }
}

void ApplicationUtil::handleIncomingMediaSample(const MediaSample& mediaSample, std::ostream& out)
{
  out.write((const char*) mediaSample.getDataBuffer().data(), mediaSample.getDataBuffer().getSize());
}

boost::system::error_code ApplicationUtil::handleVideoSample(const MediaSample& mediaSample, boost::shared_ptr<SimpleMediaSession> pMediaSession)
{
#if 0
  DLOG(INFO) << "Video received TS: " << mediaSample.getStartTime() << " Size: " << mediaSample.getPayloadSize();
#endif
  // now use brtp to send sample
  pMediaSession->sendVideo(mediaSample);
  return boost::system::error_code();
}

boost::system::error_code ApplicationUtil::handleVideoAccessUnit(const std::vector<MediaSample>& mediaSamples, boost::shared_ptr<SimpleMediaSession> pMediaSession)
{
#if 0
  DLOG(INFO) << "Video AU received TS: " << mediaSamples[0].getStartTime() << " Size: " << mediaSamples.size();
#endif
  // now use brtp to send sample
  pMediaSession->sendVideo(mediaSamples);
  return boost::system::error_code();
}

boost::system::error_code ApplicationUtil::handleAudioSample(const MediaSample& mediaSample, boost::shared_ptr<SimpleMediaSession> pMediaSession)
{
#if 0
  DLOG(INFO) << "Audio received TS: " << mediaSample.getStartTime() << " Size: " << mediaSample.getPayloadSize();
#endif
  // now use brtp to send sample
  pMediaSession->sendAudio(mediaSample);
  return boost::system::error_code();
}

void ApplicationUtil::handleMediaSourceEof(const boost::system::error_code& ec, boost::shared_ptr<SimpleMediaSession> pMediaSession)
{
#ifdef _WIN32
  LOG(INFO) << "Media Source EOF";
  pMediaSession->stop();
#else
  // try determine which state we were called in
  if (pMediaSession->isRunning())
  {
    // the media session thread is still blocked
    // simulate Ctrl-C
    LOG(INFO) << "Media source EOF: ending media session";
    kill(getpid(), SIGINT);
  }
  else
  {
    // Ctrl-C was already pushed, the video source has been ended now
    LOG(INFO) << "Media source EOF";
  }

  //  raise(SIGINT);
  //  pMediaSession->stop();
  //  kill(getpid(), SIGINT);
#endif
}

void ApplicationUtil::handleApplicationExit(const boost::system::error_code& ec, boost::shared_ptr<SimpleMediaSession> pMediaSession)
{
#ifdef _WIN32
  // TODO:
  LOG(INFO) << "TODO: Application Exit Request";
//  pMediaSession->stop();
#else
  kill(getpid(), SIGINT);
  // try determine which state we were called in
//  if (pMediaSession->isRunning())
//  {
//    // the media session thread is still blocked
//    // simulate Ctrl-C
//    LOG(INFO) << "Media source EOF: ending media session";
//    kill(getpid(), SIGINT);
//  }
//  else
//  {
//    // Ctrl-C was already pushed, the video source has been ended now
//    LOG(INFO) << "Media source EOF";
//  }
#endif
}

std::vector<uint32_t> ApplicationUtil::parseMediaSampleSizeString(const std::string& sSizes)
{
  return StringTokenizer::tokenizeV2<uint32_t>(sSizes, "|", true);
}

bool ApplicationUtil::initialiseFileMediaSource(std::ifstream& in1, const std::string& sSource)
{
  if (FileUtil::fileExists(sSource))
  {
    VLOG(5) << "Using file source: " << sSource;
    in1.open(sSource.c_str(), std::ifstream::in | std::ifstream::binary);
    return true;
  }
  return false;
}

/// V2
boost::system::error_code ApplicationUtil::handleVideoSampleV2(const MediaSample& mediaSample,
                                                               boost::shared_ptr<SimpleMediaSessionV2> pMediaSession)
{
#if 0
  DLOG(INFO) << "Video received TS: " << mediaSample.getStartTime() << " Size: " << mediaSample.getPayloadSize();
#endif
  // now use brtp to send sample
  pMediaSession->sendVideo(mediaSample);
  return boost::system::error_code();
}

boost::system::error_code ApplicationUtil::handleVideoAccessUnitV2(const std::vector<MediaSample>& mediaSamples,
                                                                   boost::shared_ptr<SimpleMediaSessionV2> pMediaSession)
{
#if 0
  DLOG(INFO) << "Video AU received TS: " << mediaSamples[0].getStartTime() << " Size: " << mediaSamples.size();
#endif
  // now use brtp to send sample
  pMediaSession->sendVideo(mediaSamples);
  return boost::system::error_code();
}

boost::system::error_code ApplicationUtil::handleAudioSampleV2(const MediaSample& mediaSample,
                                                             boost::shared_ptr<SimpleMediaSessionV2> pMediaSession)
{
#if 0
  DLOG(INFO) << "Audio received TS: " << mediaSample.getStartTime() << " Size: " << mediaSample.getPayloadSize();
#endif
  // now use brtp to send sample
  pMediaSession->sendAudio(mediaSample);
  return boost::system::error_code();
}

void ApplicationUtil::handleMediaSourceEofV2(const boost::system::error_code& ec,
                                             boost::shared_ptr<SimpleMediaSessionV2> pMediaSession)
{
#ifdef _WIN32
  LOG(INFO) << "Media Source EOF";
  pMediaSession->stop();
#else
  // try determine which state we were called in
  if (pMediaSession->isRunning())
  {
    // the media session thread is still blocked
    // simulate Ctrl-C
    LOG(INFO) << "Media source EOF: ending media session";
    kill(getpid(), SIGINT);
  }
  else
  {
    // Ctrl-C was already pushed, the video source has been ended now
    LOG(INFO) << "Media source EOF";
  }

  //  raise(SIGINT);
  //  pMediaSession->stop();
  //  kill(getpid(), SIGINT);
#endif
}

void ApplicationUtil::handleApplicationExitV2(const boost::system::error_code& ec,
                                              boost::shared_ptr<SimpleMediaSessionV2> pMediaSession)
{
#ifdef _WIN32
  // TODO:
  LOG(INFO) << "TODO: Application Exit Request";
//  pMediaSession->stop();
#else
  LOG(INFO) << "TODO: Application Exit Request";
  kill(getpid(), SIGINT);
  // try determine which state we were called in
//  if (pMediaSession->isRunning())
//  {
//    // the media session thread is still blocked
//    // simulate Ctrl-C
//    LOG(INFO) << "Media source EOF: ending media session";
//    kill(getpid(), SIGINT);
//  }
//  else
//  {
//    // Ctrl-C was already pushed, the video source has been ended now
//    LOG(INFO) << "Media source EOF";
//  }
#endif
}

}
}
