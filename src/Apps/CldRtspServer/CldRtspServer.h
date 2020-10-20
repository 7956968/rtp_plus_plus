#pragma once
#include <tuple>
#include <unordered_map>
#ifdef _WIN32
#pragma warning(push)     // disable for this header only
#pragma warning(disable:4503) 
#endif
#include <boost/bimap.hpp>
#ifdef _WIN32
#pragma warning(pop)      // restore original warning level
#endif
#include <rtp++/rfc2326/ResponseCodes.h>
#include <rtp++/rfc2326/RtspServer.h>
#include <rtp++/network/AddressDescriptor.h>
#include <rtp++/rfc4566/SessionDescription.h>
#include <rtp++/mediasession/SimpleMediaSessionV2.h>

namespace rtp_plus_plus
{

class CldRtspServer : public rfc2326::RtspServer
{
public:
  static const uint16_t DEFAULT_SERVER_MEDIA_PORT;

  CldRtspServer(boost::asio::io_service& ioService, const GenericParameters& applicationParameters,
    boost::system::error_code& ec, unsigned short uiPort = 554, unsigned uiSessionTimeout = DEFAULT_SESSION_TIMEOUT);

  boost::system::error_code addLiveRtspSource(const std::string& sRtspUri, const std::string& sLiveStreamName);
  boost::system::error_code addLiveSource(const rfc4566::SessionDescription& sdp, const InterfaceDescriptions_t& networkConfig);

protected:
  virtual rfc2326::ResponseCode handleOptions(std::set<rfc2326::RtspMethod>& methods, const std::vector<std::string>& vSupported) const;
  virtual rfc2326::ResponseCode handleSetup(const rfc2326::RtspMessage& setup, std::string& sSession,
                                            rfc2326::Transport& transport, const std::vector<std::string>& vSupported);
  virtual rfc2326::ResponseCode handleTeardown(const rfc2326::RtspMessage& teardown, const std::vector<std::string>& vSupported);

  virtual std::string getServerInfo() const { return std::string("rtp++ CLD RTSP Server ") + RTP_PLUS_PLUS_VERSION_STRING; }
  virtual bool checkResource(const std::string& sResource);
  virtual std::vector<std::string> getUnsupported(const rfc2326::RtspMessage& rtspRequest);
  virtual bool isContentTypeAccepted(std::vector<std::string>& vAcceptedTypes);
  virtual bool getResourceDescription(const std::string& sResource, std::string& sSessionDescription, std::string& sContentType, const std::vector<std::string>& vSupported, const rfc2326::Transport& transport = rfc2326::Transport());
  virtual boost::optional<rfc2326::Transport> selectTransport(const rfc2326::RtspUri& rtspUri, const std::vector<std::string>& vTransports);
  virtual std::unique_ptr<rfc2326::ServerMediaSession> createServerMediaSession(const std::string& sSession, const std::string& sSessionDescription, const std::string& ContentType);
  virtual boost::system::error_code getServerTransportInfo(const rfc2326::RtspUri& rtspUri, const std::string& sSession, rfc2326::Transport& transport);
  virtual void onShutdown();

protected:
  void accessUnitCB(const std::vector<media::MediaSample>& mediaSamples, uint32_t uiSourceId);

private:

  mutable uint16_t m_uiNextServerTransportPort;

  static uint32_t SourceId;

  typedef std::tuple<rfc4566::SessionDescription, InterfaceDescriptions_t, boost::shared_ptr<SimpleMediaSessionV2> > SourceInfo_t;
  std::unordered_map<uint32_t, SourceInfo_t> m_mSources;

  typedef boost::bimap<uint32_t, std::string> bm_type;
  bm_type m_mSourceIdSessionNameMap;
  // map to know which sample should be delivered to which RTSP sessions
  std::multimap<uint32_t, std::string> m_mSourceRtspSessionMap;
};

}
