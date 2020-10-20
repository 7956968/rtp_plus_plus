#pragma once
#include <rtp++/rfc2326/RtspServer.h>
#include <rtp++/media/IVideoDevice.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

class LiveRtspServer : public RtspServer
{
  static const uint16_t DEFAULT_SERVER_MEDIA_PORT;
public:

  LiveRtspServer(const std::string& sLiveStreamName, std::shared_ptr<media::IVideoDevice> pVideoDevice,
                 boost::asio::io_service& ioService, const GenericParameters& applicationParameters,
                 boost::system::error_code& ec, unsigned short uiPort = 554, 
                 unsigned short uiRtpPort = DEFAULT_SERVER_MEDIA_PORT, 
                 unsigned uiSessionTimeout = DEFAULT_SESSION_TIMEOUT);

  uint16_t getNextServerRtpPort() const;

protected:
  virtual ResponseCode handleOptions(std::set<RtspMethod>& methods, const std::vector<std::string>& vSupported) const;
  virtual ResponseCode handleSetup(const RtspMessage& setup, std::string& sSession,
                                   Transport& transport, const std::vector<std::string>& vSupported);
  virtual ResponseCode handleTeardown(const RtspMessage& teardown, const std::vector<std::string>& vSupported);

  virtual std::string getServerInfo() const { return std::string("rtp++ Live RTSP Server ") + RTP_PLUS_PLUS_VERSION_STRING; }
  virtual bool checkResource(const std::string& sResource);
  virtual bool isContentTypeAccepted(std::vector<std::string>& vAcceptedTypes);
  virtual std::vector<std::string> getUnsupported(const RtspMessage& rtspRequest);
  virtual bool getResourceDescription(const std::string& sResource, std::string& sSessionDescription, std::string& sContentType, const std::vector<std::string>& vSupported, const Transport& transport = Transport());
  virtual boost::optional<Transport> selectTransport(const RtspUri& rtspUri, const std::vector<std::string>& vTransports);
  virtual std::unique_ptr<ServerMediaSession> createServerMediaSession(const std::string& sSession, const std::string& sSessionDescription, const std::string& sContentType);
  virtual boost::system::error_code getServerTransportInfo(const RtspUri& rtspUri, const std::string& sSession, Transport& transport);
  virtual void onShutdown();

protected:
  boost::system::error_code accessUnitCB(const std::vector<media::MediaSample>& mediaSamples);

private:
  std::string m_sLiveStreamName;
  std::shared_ptr<media::IVideoDevice> m_pVideoDevice;

  mutable uint16_t m_uiNextServerTransportPort;
};

} // rfc2326
} // rtp_plus_plus
