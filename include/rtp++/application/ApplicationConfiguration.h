#pragma once
#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <cpputil/GenericParameters.h>
#include <rtp++/application/ApplicationParameters.h>

namespace rtp_plus_plus 
{
namespace app
{

/**
  * Common application parameters for rtp++ applications
  * Note: parameter names map onto their text version by replacing underscores "_" with dashes "-"
  */
class ApplicationConfiguration
{
public:
  enum Option
  {
    GENERIC,
    SESSION,
    SENDER,
    RECEIVER,
    VIDEO,
    AUDIO,
    RTP_RTCP,
    NETWORK,
    MULTIPATH,
    RTPPROXY,
    SIMULATION,
    RTSP_SERVER,
    RTSP_CLIENT,
    SIP_CLIENT,
    SIP_PROXY
  };

  ApplicationConfiguration();

  ApplicationConfiguration& addOptions(Option option);
  boost::program_options::options_description generateOptions();
  GenericParameters getGenericParameters();

public:

  struct AppParameters
  {
    AppParameters()
      :Verbose(false)
    {

    }

    bool Verbose;
    std::string logfile;
  };
  AppParameters Application;

  // session
  struct SessionParameters
  {
    SessionParameters()
      :Duration(0),
        Mtu(1460)//,
//        DisableRtcp(false),
//        ExitOnByeReceived(true),
//        T_rr_interval(5.0),
//        EnableMpRtp(false),
//        RtcpMux(false),
//        RtcpRs(false),
//        ForceRtpSsrc(false),
//        RtpSsrc(0)
    {

    }

    std::vector<std::string> sessionDescriptions;
    std::vector<std::string> NetworkConf;
    // application duration in seconds
    uint32_t Duration;
    // network MTU
    uint32_t Mtu;
//    bool DisableRtcp;
//    bool ExitOnByeReceived;
//    double T_rr_interval;
//    bool EnableMpRtp;
//    bool RtcpMux;
//    bool RtcpRs;
//    bool ForceRtpSsrc;
//    // TODO: this should be a vector for multiple media lines?
//    uint32_t RtpSsrc;
  };
  SessionParameters Session;

  // sender
  struct SenderParameters
  {
    SenderParameters()
      :UseMediaSampleTime(false)//,
//        ForceRtpSn(false),
//        ForceRtpTs(false),
//        RtpSn(0),
//        RtpTs(0),
//        DisableStap(false),
//        RapidSyncMode(false)
    {

    }

    bool UseMediaSampleTime;
    uint32_t Scheduler;
    std::string SchedulerParam;
//    bool ForceRtpSn;
//    bool ForceRtpTs;
//    uint16_t RtpSn;
//    uint32_t RtpTs;
    std::vector<std::string> SctpPolicies;
    std::string SctpPrio;
//    bool DisableStap;
//    uint32_t RapidSyncMode;
  };
  SenderParameters Sender;

  struct ReceiverParameters
  {
    ReceiverParameters()
      :JitterBufferType(2),
        BufferLatency(150),
        PrematureTimeoutProbability(0.05),
        MavgHistSize(10),
        Analyse(false)
    {

    }

    std::string AudioFilename;
    // output file (H.264 Debugging)
    std::string VideoFilename;
    // Types of jitter buffers for testing
    uint32_t JitterBufferType;
    // RTP playout buffer latency: affects the number of packets that are considered late
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
  };
  ReceiverParameters Receiver;

  // video
  struct VideoParameters
  {
    VideoParameters()
      :NoVideo(false),
        Fps(25),
        LimitRate(true),
        Loop(false),
        LoopCount(0),
        TotalSamples(0)
    {

    }

    /// video: no video
    bool NoVideo;
    /// video: media type
    std::string VideoMediaType;
    /// video kbps
    std::string VideoKbps;
    /// min encoder bitate for live encoding
    uint32_t VideoMinKbps;
    /// max encoder bitate for live encoding
    uint32_t VideoMaxKbps;
    // Should be name of camera or path to file in the case of virtual camera
    std::string VideoDevice;
    // Video coding implementation
    std::string VideoCodecImpl;
    // Codec parameters
    std::vector<std::string> VideoCodecParams;
    // video codec
    std::string VideoCodec;
    uint32_t Fps;
    uint32_t Width;
    uint32_t Height;
    bool LimitRate;
    std::string PayloadType;
    uint8_t PayloadChar;
    std::string PayloadSizeString;
    bool Loop;
    uint32_t LoopCount;
    uint32_t TotalSamples;
  };
  VideoParameters Video;

  //audio
  struct AudioParameters
  {
    AudioParameters()
      :NoAudio(false),
        TotalSamples(0)
    {

    }

    /// audio: no audio
    bool NoAudio;
    /// audio: media type
    std::string AudioMediaType;
    // Should be name of mic or path to file in the case of virtual mic
    std::string AudioDevice;
    uint32_t RateMs;
    std::string PayloadType;
    uint8_t PayloadChar;
    std::string PayloadSizeString;
    uint32_t TotalSamples;
  };
  AudioParameters Audio;

  // RTP RTCP
  struct RtpRtcpParameters
  {
    RtpRtcpParameters()
      :RtcpMux(false),
        EnableMpRtp(false),
        RtcpRtpHeaderExt(false),
        RtcpRs(false),
        RtxTime(0),
        RapidSyncMode(0),
        ExtractNtp(0),
        DisableRtcp(false),
        ExitOnByeReceived(true),
        T_rr_interval(5.0),
        ForceRtpSsrc(false),
        RtpSsrc(0),
        ForceRtpSn(false),
        ForceRtpTs(false),
        RtpSn(0),
        RtpTs(0),
        DisableStap(false),
        SummariseStats(false)
    {

    }

    /// RTP profile
    std::string RtpProfile;
    /// rtcp-mux
    bool RtcpMux;
    /// mprtp
    bool EnableMpRtp;
    /// rtcp rtp header ext
    bool RtcpRtpHeaderExt;
    /// rtcp-fb
    std::vector<std::string> RtcpFb;
    /// rtcp-xr
    std::vector<std::string> RtcpXr;
    /// rtcp-rs
    bool RtcpRs;
    /// rtx-time
    uint32_t RtxTime;
    uint32_t RapidSyncMode;
    bool ExtractNtp;
    bool DisableRtcp;
    bool ExitOnByeReceived;
    double T_rr_interval;
    bool ForceRtpSn;
    bool ForceRtpTs;
    bool ForceRtpSsrc;
    uint16_t RtpSn;
    uint32_t RtpTs;
    // TODO: this should be a vector for multiple media lines?
    uint32_t RtpSsrc;
    bool DisableStap;
    bool SummariseStats;
  };
  RtpRtcpParameters RtpRtcp;

  // Network parameters
  struct NetworkParameters
  {
    /// transport
    std::string Protocol;
    // preferred RTP starting port
    uint16_t RtpPort;
    std::vector<std::string> LocalInterfaces;
    // local network conf
    std::string LocalNetworkConf;
    // remote network conf
    std::string RemoteNetworkConf;
    /// remote host
    std::string RemoteHost;
    // hack to set local FQDN
    std::string FQDN;
    // hack to set local domain name
    std::string Domain;
  };
  NetworkParameters Network;

  // Multipath
  struct MultipathParameters
  {
    MultipathParameters()
      :PathwiseSplitAllowed(true)
    {

    }

    std::string SchedulingAlgorithm;
    bool PathwiseSplitAllowed;
  };
  MultipathParameters Multipath;

  struct RtpProxyParameters
  {
    RtpProxyParameters()
      :Strict(false)
    {

    }

    // Interface configuration
    std::vector<std::string> InterfaceConf;
    // Fwd configuration
    std::vector<std::string> FwdConf;
    // Store local onfiguration
    std::string LocalConf;
    // Store receiver onfiguration
    std::string ReceiverConf;
    // SDP for remote media session
    std::string SenderConf;
    // loss probability
    std::vector<uint32_t> LossProbability;
    // owd in ms
    std::vector<uint32_t> OwdMs;
    // jitter in ms
    std::vector<uint32_t> JitterMs;
    // strict: forces RTP parsing
    bool Strict;
    // Channel delay config files
    std::string ChannelConfigFile;
  };
  RtpProxyParameters RtpProxy;

  struct SimulationParameters
  {
    std::string ChannelConfigFile;
  };
  SimulationParameters Simulation;

  struct RtspServerParameters
  {
    std::string StreamName;
    // RTSP port
    uint16_t RtspPort;
    // RTP session timeout
    uint32_t RtpSessionTimeout;
    // RTSP RTP port
    uint16_t RtspRtpPort;
    //uint32_t Fps;
  };
  RtspServerParameters Rtsp;

  struct RtspClientParameters
  {
    RtspClientParameters()
      :UseRtpRtsp(false),
        OptionsOnly(false),
        DescribeOnly(false),
        AudioOnly(false),
        VideoOnly(false),
        ExternalRtp(false)
    {

    }

    std::string RtspUri;
    bool UseRtpRtsp;
    uint16_t ClientPort;
    uint32_t Duration;
    bool OptionsOnly;
    bool DescribeOnly;
    bool AudioOnly;
    bool VideoOnly;
    bool ExternalRtp;
    std::vector<std::string> DestMprtpAddr;
    std::vector<std::string> ForceDestMprtpAddr;
  };
  RtspClientParameters RtspClient;

  struct SipClientParameters
  {
    SipClientParameters()
      :UseSipProxy(false),
        RegisterWithProxy(false)
    {

    }

    std::string SipUser;
    // Sip port
    uint16_t SipPort;
    std::string SipProxy;
    bool UseSipProxy;
    bool RegisterWithProxy;
    std::string SipCallee;
    std::string MediaConfigurationFile;
    std::string PublicIp;
//    uint32_t Duration;
  };
  SipClientParameters SipClient;

  struct SipProxyParameters
  {
    // Sip port
    uint16_t SipPort;
  };
  SipProxyParameters SipProxy;

private:
  boost::program_options::options_description m_genericOptions;
  boost::program_options::options_description m_sessionOptions;
  boost::program_options::options_description m_senderOptions;
  boost::program_options::options_description m_receiverOptions;
  boost::program_options::options_description m_videoOptions;
  boost::program_options::options_description m_audioOptions;
  boost::program_options::options_description m_rtp_rtcp_options;
  boost::program_options::options_description m_networkOptions;
  boost::program_options::options_description m_multipathOptions;
  boost::program_options::options_description m_proxyOptions;
  boost::program_options::options_description m_simulationOptions;
  boost::program_options::options_description m_rtspServerOptions;
  boost::program_options::options_description m_rtspClientOptions;
  boost::program_options::options_description m_sipProxyOptions;
  boost::program_options::options_description m_sipClientOptions;

  std::set<Option> m_options;
};

} // app
} // rtp_plus_plus

