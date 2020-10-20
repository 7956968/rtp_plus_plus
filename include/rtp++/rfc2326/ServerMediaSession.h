#pragma once
#include <string>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <rtp++/media/MediaSample.h>
#include <rtp++/rfc2326/IRtspAgent.h>
#include <rtp++/rfc2326/ResponseCodes.h>
#include <rtp++/rfc2326/RtspMessage.h>
#include <rtp++/rfc2326/RtspUri.h>

namespace rtp_plus_plus
{
namespace rfc2326
{

// fwd
class RtspServer;

class ServerMediaSession : public IRtspAgent
{
  friend class RtspServer;
public:

  /**
   * @brief Constructor
   */
  ServerMediaSession(RtspServer& rtspServer, const std::string& sSession, 
    const std::string& sSessionDescription, const std::string& sContentType);
  /**
   * @brief Destructor
   */
  virtual ~ServerMediaSession();

  std::string getSession() { return m_sSession; }

  bool isPlaying() const { return m_eState == ST_PLAYING; }
  bool isExpired() const;  
  bool livenessCheckFailed() const;

  void deliver(const uint8_t uiPayloadType, const media::MediaSample& mediaSample);
  void deliver(const uint8_t uiPayloadType, const std::vector<media::MediaSample>& mediaSamples);
  void shutdown();

protected:

  virtual ResponseCode handleOptions(std::set<RtspMethod>& methods, const std::vector<std::string>& vSupported) const;
  virtual ResponseCode handleDescribe(const RtspMessage& describe, std::string& sSessionDescription,
                                      std::string& sContentType, const std::vector<std::string>& vSupported);
  virtual ResponseCode handleSetup(const RtspMessage& setup, std::string& sSession,
    Transport& transport, const std::vector<std::string>& vSupported);
  /// overriding for state management
  virtual ResponseCode handlePlay(const RtspMessage& play, RtpInfo& rtpInfo, const std::vector<std::string>& vSupported);
  virtual ResponseCode doHandlePlay(const RtspMessage& play, RtpInfo& rtpInfo, const std::vector<std::string>& vSupported){ return NOT_IMPLEMENTED; }

  // TODO: handle SETUP still has to be implemented. At what point should the media session object be created?!?

  virtual bool checkResource(const std::string& sResource);
  virtual boost::optional<Transport> selectTransport(const RtspUri& rtspUri, const std::vector<std::string>& vTransports);
  virtual void doDeliver(const uint8_t uiPayloadType, const media::MediaSample& mediaSample){}
  virtual void doDeliver(const uint8_t uiPayloadType, const std::vector<media::MediaSample>& mediaSamples){}
  virtual void doShutdown(){}
  /**
   * allow subclasses to timeout sessions based on RTCP, RTSP, etc
   */
  virtual bool isSessionLive() const{ return true; }

protected:

  RtspServer& m_rtspServer;
  std::string m_sSession;
  std::string m_sSessionDescription;
  std::string m_sContentType;
  
  // TOREMOVE: we need to be able to handle the Transport of multiple m-lines
  // Transport m_transport;

  enum State
  {
    ST_SETUP,
    ST_PLAYING,
    ST_TORNDOWN,
    ST_ERROR
  };
  State m_eState;

  boost::posix_time::ptime m_tTorndown;
};

} // rfc2326
} // rtp_plus_plus
