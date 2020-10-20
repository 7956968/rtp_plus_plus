#pragma once
#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <cpputil/GenericParameters.h>

namespace rtp_plus_plus 
{

struct RtcConfig
{
  /// app: name of log file
  std::string logfile;
  /// session: duration of streaming session in seconds
  uint32_t Duration;
  /// session: exit application on BYE
  bool ExitOnByeReceived;

  /// audio: no audio
  bool NoAudio;
  /// audio: media type
  std::string AudioMediaType;

  /// video: no video
  bool NoVideo;
  std::string VideoDevice;
  std::string VideoPayloadSizeString;
  uint32_t LoopCount;
  /// video: media type
  std::string VideoMediaType;
  /// video kbps
  std::string VideoKbps;
  /// min encoder bitate for live encoding
  uint32_t VideoMinKbps;
  /// max encoder bitate for live encoding
  uint32_t VideoMaxKbps;
  // Video coding implementation
  std::string VideoCodecImpl;
  // Codec parameters
  std::vector<std::string> VideoCodecParams;
  // video codec
  std::string VideoCodec;
  uint32_t Fps;
  uint32_t Width;
  uint32_t Height;

  /// RPC port
  uint16_t RpcPort;
  ///local interfaces
  std::vector<std::string> LocalInterfaces;
  /// transport
  std::string Protocol;
  /// RTP profile
  std::string RtpProfile;
  /// RPC port
  uint16_t RtpPort;
  /// rtcp-mux
  bool RtcpMux;
  /// mprtp
  bool EnableMpRtp;
  /// rtcp-fb
  std::vector<std::string> RtcpFb;
  /// rtcp-xr
  std::vector<std::string> RtcpXr;
  /// rtcp-rs
  bool RtcpRs;
  /// rtcp header ext
  bool RtcpRtpHeaderExt;
  /// rtx-time
  uint32_t RtxTime;

  uint16_t RtpSn;
  uint32_t RtpTs;
  uint32_t RtpSsrc;
  bool SummariseStats;

  uint32_t Scheduler;
  std::string SchedulerParam;
  std::vector<std::string> SctpPolicies;
  std::string SctpPrio;
  uint32_t BufferLatency;
  // std predictor
  std::string Predictor;
  // MPRTP predictor
  std::string MpPredictor;
  // premature timeout probability
  double PrematureTimeoutProbability;
  uint32_t MavgHistSize;
  // Analyse stats
  bool Analyse;
  uint32_t RapidSyncMode;
  bool ExtractNtp;

  // Should be name of mic or path to file in the case of virtual mic
  std::string AudioDevice;
  uint32_t RateMs;
  std::string AudioPayloadSizeString;
  // hack to set local FQDN
  std::string FQDN;
  // hack to set local domain name
  std::string Domain;
  std::string MpSchedulingAlgorithm;
  std::string SipUser;
  std::string MediaConfigurationFile;
  std::string PublicIp;
};

/**
 * @brief The RtcConfig class stores common application parameters.
 * Note: parameter names map onto their text version by replacing underscores "_" with dashes "-"
 */
class RtcConfigOptions
{
public:

  RtcConfigOptions();
  boost::program_options::options_description generateOptions();

public:
  RtcConfig RtcParameters;

private:
  boost::program_options::options_description m_options;
};

} // rtp_plus_plus
