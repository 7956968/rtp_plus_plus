#include "CorePch.h"
#include <rtp++/application/ApplicationConfiguration.h>
#include <rtp++/rfc3550/Rfc3550.h>

namespace po = boost::program_options;

namespace rtp_plus_plus
{
namespace app
{

ApplicationConfiguration::ApplicationConfiguration()
  :m_genericOptions("Generic options"),
    m_sessionOptions("Session options"),
    m_senderOptions("Sender options"),
    m_receiverOptions("Receiver options"),
    m_videoOptions("Video options"),
    m_audioOptions("Audio options"),
    m_networkOptions("Network options"),
    m_rtp_rtcp_options("RTP/RTCP options"),
    m_multipathOptions("Multipath options"),
    m_proxyOptions("RTP proxy options"),
    m_simulationOptions("Simulation options"),
    m_rtspServerOptions("RTSP server options"),
    m_rtspClientOptions("RTSP client options"),
    m_sipProxyOptions("SIP proxy options"),
    m_sipClientOptions("SIP client options")
{
  m_genericOptions.add_options()
      (ApplicationParameters::help.c_str(), "produce help message")
      (ApplicationParameters::vvv.c_str(), po::bool_switch(&Application.Verbose)->default_value(false), "Verbose [false]")
      (ApplicationParameters::logfile.c_str(), po::value<std::string>(&Application.logfile), "Logfile")
      ;

  m_sessionOptions.add_options()
      (ApplicationParameters::nc.c_str(), po::value<std::vector<std::string> >(&Session.NetworkConf), "network configuration(s) of remote host")
      (ApplicationParameters::sdp.c_str(), po::value<std::vector<std::string> >(&Session.sessionDescriptions), "media session sdp file(s)")
      (ApplicationParameters::mtu.c_str(), po::value<uint32_t>(&Session.Mtu)->default_value(1460), "Max video packet size (match MTU)")
      (ApplicationParameters::dur.c_str(), po::value<uint32_t>(&Session.Duration)->default_value(0), "Session duration (milliseconds )(0 = no limit)")
      ;

  m_senderOptions.add_options()
      (ApplicationParameters::use_media_sample_time.c_str(), po::value<bool>(&Sender.UseMediaSampleTime)->default_value(false), "Use media sample time")
      (ApplicationParameters::scheduler.c_str(), po::value<uint32_t>(&Sender.Scheduler)->default_value(UINT32_MAX), "Scheduler")
      (ApplicationParameters::scheduler_param.c_str(), po::value<std::string>(&Sender.SchedulerParam)->default_value(""), "Scheduler param")
      (ApplicationParameters::sctp_policies.c_str(), po::value<std::vector<std::string> >(&Sender.SctpPolicies), "SCTP retransmission policies")
      (ApplicationParameters::sctp_prio.c_str(), po::value<std::string>(&Sender.SctpPrio), "SCTP prioritisation")
      ;

  m_receiverOptions.add_options()
      (ApplicationParameters::aout.c_str(), po::value<std::string>(&Receiver.AudioFilename)->default_value("audio-"), "Audio Output [file|cout]")
      (ApplicationParameters::vout.c_str(), po::value<std::string>(&Receiver.VideoFilename)->default_value("video-"), "Video Output [file|cout]")
      (ApplicationParameters::jitter_buffer_type.c_str(), po::value<uint32_t>(&Receiver.JitterBufferType)->default_value(2), "RTP jitter buffer type (0=std, 1=pts, 2=v2)")
      (ApplicationParameters::buf_lat.c_str(), po::value<uint32_t>(&Receiver.BufferLatency)->default_value(100), "RTP playout buffer latency")
      (ApplicationParameters::pred.c_str(), po::value<std::string>(&Receiver.Predictor), "Predictor to be used in RTO [mavg|ar2]")
      (ApplicationParameters::mp_pred.c_str(), po::value<std::string>(&Receiver.MpPredictor), "Predictor to be used in MPRTP [mp-single|mp-cross|mp-comp]")
      (ApplicationParameters::pto.c_str(), po::value<double>(&Receiver.PrematureTimeoutProbability)->default_value(0.05), "Premature timeout probability: (0.0, 1)")
      (ApplicationParameters::mavg_hist.c_str(), po::value<uint32_t>(&Receiver.MavgHistSize)->default_value(10), "Moving average history")
      (ApplicationParameters::analyse.c_str(), po::bool_switch(&Receiver.Analyse)->default_value(false), "Analyse RTP stats such as loss")
      ;

  m_videoOptions.add_options()
      (ApplicationParameters::no_video.c_str(), po::bool_switch(&Video.NoVideo)->default_value(false), "No video")
      (ApplicationParameters::video_media_type.c_str(), po::value<std::string>(&Video.VideoMediaType), "Video media type")
      (ApplicationParameters::video_device.c_str(), po::value<std::string>(&Video.VideoDevice)->default_value("gen"), "media source = [gen, stdin, <<device_name>>, <<file_name>>]")
      (ApplicationParameters::video_codec.c_str(), po::value<std::string>(&Video.VideoCodec), "H264,H265")
      (ApplicationParameters::video_codec_impl.c_str(), po::value<std::string>(&Video.VideoCodecImpl), "vpp,x264,openh264")
      (ApplicationParameters::video_codec_param.c_str(), po::value<std::vector<std::string> >(&Video.VideoCodecParams), "Video codec parameters")
      (ApplicationParameters::video_fps.c_str(), po::value<uint32_t>(&Video.Fps)->default_value(25), "Video fps")
      (ApplicationParameters::video_kbps.c_str(), po::value<std::string>(&Video.VideoKbps)->default_value("100"), "Video kbps")
      (ApplicationParameters::video_min_kbps.c_str(), po::value<uint32_t>(&Video.VideoMinKbps)->default_value(50), "Video min encoder bitrate kbps")
      (ApplicationParameters::video_max_kbps.c_str(), po::value<uint32_t>(&Video.VideoMaxKbps)->default_value(5000), "Video max encoder bitrate kbps")
      (ApplicationParameters::video_width.c_str(), po::value<uint32_t>(&Video.Width)->default_value(0), "Video width (only applies to live enc)")
      (ApplicationParameters::video_height.c_str(), po::value<uint32_t>(&Video.Height)->default_value(0), "Video height (only applies to live enc)")
      // (ApplicationParameters::fps.c_str(), po::value<uint32_t>(&Video.Fps)->default_value(25), "frames per second")
      (ApplicationParameters::limit_rate.c_str(), po::value<bool>(&Video.LimitRate)->default_value(true), "Limit frame rate to fps [true]")
      (ApplicationParameters::v_p.c_str(), po::value<std::string>(&Video.PayloadType)->default_value("rand"), "video payload type = [rand,char,ntp]")
      (ApplicationParameters::v_tot.c_str(), po::value<uint32_t>(&Video.TotalSamples)->default_value(0), "Total number of video samples to generate")
      (ApplicationParameters::v_pc.c_str(), po::value<uint8_t>(&Video.PayloadChar)->default_value('0'), "video payload char =[0-255]")
      (ApplicationParameters::v_ps.c_str(), po::value<std::string>(&Video.PayloadSizeString)->default_value("1400"), "video payload size = [a|b|c]")
      (ApplicationParameters::v_loop.c_str(), po::bool_switch(&Video.Loop)->default_value(false), "Try loop video file source: only applies if source=file.264")
      (ApplicationParameters::v_loop_count.c_str(), po::value<uint32_t>(&Video.LoopCount)->default_value(0), "Loop count. 0 = infinite. Only applies if source=file.264")
      ;

  m_audioOptions.add_options()
      (ApplicationParameters::no_audio.c_str(), po::bool_switch(&Audio.NoAudio)->default_value(false), "No audio")
      (ApplicationParameters::audio_media_type.c_str(), po::value<std::string>(&Audio.AudioMediaType), "Audio media type")
      (ApplicationParameters::audio_device.c_str(), po::value<std::string>(&Audio.AudioDevice)->default_value("gen"), "media source = [gen, stdin, <<device_name>>, <<file_name>>]")
      (ApplicationParameters::a_r.c_str(), po::value<uint32_t>(&Audio.RateMs)->default_value(20), "audio rate (ms)")
      (ApplicationParameters::a_p.c_str(), po::value<std::string>(&Audio.PayloadType)->default_value("rand"), "audio payload type =[rand,char='a',ntp]")
      (ApplicationParameters::a_pc.c_str(), po::value<uint8_t>(&Audio.PayloadChar)->default_value('0'), "audio payload char =[0-255]")
      (ApplicationParameters::a_ps.c_str(), po::value<std::string>(&Audio.PayloadSizeString)->default_value("160"), "audio payload size = [a|b|c]")
      (ApplicationParameters::a_tot.c_str(), po::value<uint32_t>(&Audio.TotalSamples)->default_value(0), "Total number of audio samples to generate")
      ;

  m_rtp_rtcp_options.add_options()
      (ApplicationParameters::rtp_profile.c_str(), po::value<std::string>(&RtpRtcp.RtpProfile)->default_value(rfc3550::AVP), "RTP profile [AVP|AVPF]")
      (ApplicationParameters::rtcp_fb.c_str(), po::value<std::vector<std::string> >(&RtpRtcp.RtcpFb), "RTCP FB: [nack|ack]")
      (ApplicationParameters::rtcp_xr.c_str(), po::value<std::vector<std::string> >(&RtpRtcp.RtcpXr), "RTCP XR: [rcvr-rtt]")
      (ApplicationParameters::rtcp_rs.c_str(), po::bool_switch(&RtpRtcp.RtcpRs)->default_value(false), "RTCP reduced size")
      (ApplicationParameters::rtcp_rtp_header_ext.c_str(), po::bool_switch(&RtpRtcp.RtcpRtpHeaderExt)->default_value(false), "RTCP RTP header ext")
      (ApplicationParameters::rtx_time.c_str(), po::value<uint32_t>(&RtpRtcp.RtxTime)->default_value(0), "rtx-time (ms)")
      (ApplicationParameters::enable_mprtp.c_str(), po::bool_switch(&RtpRtcp.EnableMpRtp)->default_value(false), "Enable MPRTP support")
      (ApplicationParameters::rtcp_mux.c_str(), po::bool_switch(&RtpRtcp.RtcpMux)->default_value(false), "Enable RTCP-mux support")
      (ApplicationParameters::rapid_sync_mode.c_str(), po::value<uint32_t>(&RtpRtcp.RapidSyncMode)->default_value(0), "Rapid Sync Mode (0 = none, 1 = all")
      (ApplicationParameters::extract_ntp_ts.c_str(), po::bool_switch(&RtpRtcp.ExtractNtp)->default_value(false), "Extract NTP TS from RTP Payload")
      (ApplicationParameters::disable_rtcp.c_str(), po::value<bool>(&RtpRtcp.DisableRtcp)->default_value(false), "Disable RTCP")
      (ApplicationParameters::exit_on_bye.c_str(), po::bool_switch(&RtpRtcp.ExitOnByeReceived)->default_value(false), "Exit on RTCP BYE received")
      (ApplicationParameters::t_rr_interval.c_str(), po::value<double>(&RtpRtcp.T_rr_interval)->default_value(5.0), "RFC4585 T_rr_interval")
      (ApplicationParameters::force_rtp_sn.c_str(), po::bool_switch(&RtpRtcp.ForceRtpSn)->default_value(false), "Force RTP sequence number")
      (ApplicationParameters::rtp_sn.c_str(), po::value<uint16_t>(&RtpRtcp.RtpSn), "RTP sequence number")
      (ApplicationParameters::force_rtp_ts.c_str(), po::bool_switch(&RtpRtcp.ForceRtpTs)->default_value(false), "Force RTP timestamp base")
      (ApplicationParameters::rtp_ts.c_str(), po::value<uint32_t>(&RtpRtcp.RtpTs), "RTP timestamp base")
      (ApplicationParameters::force_rtp_ssrc.c_str(), po::bool_switch(&RtpRtcp.ForceRtpSsrc)->default_value(false), "Force RTP SSRC")
      (ApplicationParameters::rtp_ssrc.c_str(), po::value<uint32_t>(&RtpRtcp.RtpSsrc), "RTP SSRC")
      (ApplicationParameters::rtp_summarise_stats.c_str(), po::bool_switch(&RtpRtcp.SummariseStats)->default_value(false), "Summarise RTP session stats")
      ;
  const std::string LocalInterfacesDescrip("Local network interfaces to be used by the client. "\
    "If this parameter is set to 0.0.0.0, the client will attempt to auto-detect the IP addresses. "\
    "Multiple interfaces can be comma-separated.");

  m_networkOptions.add_options()
    (ApplicationParameters::protocol.c_str(), po::value<std::string>(&Network.Protocol)->default_value("udp"), "Protocol [udp|tcp|sctp|dccp]")
    (ApplicationParameters::rtp_port.c_str(), po::value<uint16_t>(&Network.RtpPort)->default_value(0), "Preferred RTP starting port")
    (ApplicationParameters::local_interfaces.c_str(), po::value<std::vector<std::string> >(&Network.LocalInterfaces), LocalInterfacesDescrip.c_str())
    (ApplicationParameters::l_nc.c_str(), po::value<std::string>(&Network.LocalNetworkConf)->default_value(""), "Local network configuration")
    (ApplicationParameters::r_nc.c_str(), po::value<std::string>(&Network.RemoteNetworkConf)->default_value(""), "Remote network configuration")
    (ApplicationParameters::remote_host.c_str(), po::value<std::string>(&Network.RemoteHost)->default_value(""), "Remote host")
    (ApplicationParameters::fqdn.c_str(), po::value<std::string>(&Network.FQDN)->default_value(""), "SIP FQDN (optional) used in \"Contact\"")
    (ApplicationParameters::domain.c_str(), po::value<std::string>(&Network.Domain), "SIP Domain (optional) used in generated Call-ID")
    ;

  m_multipathOptions.add_options()
      (ApplicationParameters::mprtp_sa.c_str(), po::value<std::string>(&Multipath.SchedulingAlgorithm)->default_value("rr"), "MPRTP scheduling: algorithm = [ rr | rnd | \"x-y-z...\" ]")
      (ApplicationParameters::mprtp_psa.c_str(), po::value<bool>(&Multipath.PathwiseSplitAllowed)->default_value(false), "MPRTP scheduling: pathwise split allowed")
      ;

  m_proxyOptions.add_options()
      (ApplicationParameters::interface_conf.c_str(), po::value<std::vector<std::string> >(&RtpProxy.InterfaceConf), "Interface network configuration(s)")
      (ApplicationParameters::fwd_conf.c_str(), po::value<std::vector<std::string> >(&RtpProxy.FwdConf), "FWD configuration(s)")
      (ApplicationParameters::r_nc.c_str(), po::value<std::string>(&RtpProxy.ReceiverConf)->default_value(""), "Receiver network configuration")
      (ApplicationParameters::l_nc.c_str(), po::value<std::string>(&RtpProxy.LocalConf)->default_value(""), "Local network configuration")
      (ApplicationParameters::s_nc.c_str(), po::value<std::string>(&RtpProxy.SenderConf)->default_value(""), "Sender network configuration")
      (ApplicationParameters::strict.c_str(), po::bool_switch(&RtpProxy.Strict)->default_value(false), "Strict: forces RTP and RTCP level parsing")
      (ApplicationParameters::loss.c_str(), po::value<std::vector<uint32_t> >(&RtpProxy.LossProbability), "Loss probability(s)")
      (ApplicationParameters::owd_ms.c_str(), po::value<std::vector<uint32_t> >(&RtpProxy.OwdMs), "Simulated OWD in ms(s)")
      (ApplicationParameters::jitter_ms.c_str(), po::value<std::vector<uint32_t> >(&RtpProxy.JitterMs), "Simulated jitter in ms(s)")
      (ApplicationParameters::channel_config.c_str(), po::value<std::string>(&RtpProxy.ChannelConfigFile), "Channel configuration file")
      ;

  m_simulationOptions.add_options()
      (ApplicationParameters::channel_config.c_str(), po::value<std::string>(&Simulation.ChannelConfigFile), "Channel configuration file")
      ;

  m_rtspServerOptions.add_options()
      (ApplicationParameters::stream_name.c_str(), po::value<std::string>(&Rtsp.StreamName), "Name of live stream")
      (ApplicationParameters::rtsp_port.c_str(), po::value<uint16_t>(&Rtsp.RtspPort)->default_value(554), "RTSP port")
      (ApplicationParameters::rtsp_rtp_port.c_str(), po::value<uint16_t>(&Rtsp.RtspRtpPort)->default_value(49170), "RTSP RTP port")
      (ApplicationParameters::rtp_session_timeout.c_str(), po::value<uint32_t>(&Rtsp.RtpSessionTimeout)->default_value(45), "RTSP/RTP session timeout (s)")
      ;

  const std::string DestMprtpAddrDescrip("dest_mprtp_addr field for Transport header in RTSP "\
    "SETUP request. Used to specify which interfaces/ports the client MUST bind to."\
    "E.g. \"1 192.0.2.1 49170; 2 198.51.100.1 51372\".");

  const std::string ForceDestMprtpAddrDescrip("Forces dest_mprtp_addr field for Transport header in "\
    "RTSP SETUP request. Useful for inserting an RTP proxy between the RTSP server and client. "\
    "E.g. \"1 192.0.2.1 49170; 2 198.51.100.1 51372\". WARNING: there is no "\
    "validation of the address or port for the purpose of using RTP proxy addresses.");

  m_rtspClientOptions.add_options()
    (ApplicationParameters::rtsp_uri.c_str(), po::value<std::string>(&RtspClient.RtspUri), "RTSP URI")
    (ApplicationParameters::use_rtp_rtsp.c_str(), po::bool_switch(&RtspClient.UseRtpRtsp), "RTP over RTSP")
    (ApplicationParameters::client_rtp_port.c_str(), po::value<uint16_t>(&RtspClient.ClientPort)->default_value(5004), "Preferred client port")
    (ApplicationParameters::dur.c_str(), po::value<uint32_t>(&RtspClient.Duration)->default_value(0), "Duration (s)")
    (ApplicationParameters::options_only.c_str(), po::bool_switch(&RtspClient.OptionsOnly), "OPTIONS only")
    (ApplicationParameters::describe_only.c_str(), po::bool_switch(&RtspClient.DescribeOnly), "DESCRIBE only")
    (ApplicationParameters::audio_only.c_str(), po::bool_switch(&RtspClient.AudioOnly), "Audio only")
    (ApplicationParameters::video_only.c_str(), po::bool_switch(&RtspClient.VideoOnly), "Video only")
    (ApplicationParameters::external_rtp.c_str(), po::bool_switch(&RtspClient.ExternalRtp), "External RTP handler")
    (ApplicationParameters::dest_mprtp_addr.c_str(), po::value<std::vector<std::string> >(&RtspClient.DestMprtpAddr), DestMprtpAddrDescrip.c_str())
    (ApplicationParameters::force_dest_mprtp_addr.c_str(), po::value<std::vector<std::string> >(&RtspClient.ForceDestMprtpAddr), ForceDestMprtpAddrDescrip.c_str())
    ;

  m_sipClientOptions.add_options()
    (ApplicationParameters::sip_user.c_str(), po::value<std::string>(&SipClient.SipUser)->default_value("Anon"), "SIP user name")
    (ApplicationParameters::sip_port.c_str(), po::value<uint16_t>(&SipClient.SipPort)->default_value(5060), "SIP port")
    (ApplicationParameters::sip_proxy.c_str(), po::value<std::string>(&SipClient.SipProxy), "SIP proxy URI")
    (ApplicationParameters::use_sip_proxy.c_str(), po::bool_switch(&SipClient.UseSipProxy)->default_value(false), "Use SIP proxy for call")
    (ApplicationParameters::register_with_proxy.c_str(), po::bool_switch(&SipClient.RegisterWithProxy)->default_value(false), "Register with SIP proxy")
    (ApplicationParameters::sip_callee.c_str(), po::value<std::string>(&SipClient.SipCallee), "SIP callee URI")
    (ApplicationParameters::media_configuration_file.c_str(), po::value<std::string>(&SipClient.MediaConfigurationFile), "Media configuration file")
    (ApplicationParameters::public_ip.c_str(), po::value<std::string>(&SipClient.PublicIp), "Public IP (NAT)")
    ;

  m_sipProxyOptions.add_options()
    (ApplicationParameters::sip_port.c_str(), po::value<uint16_t>(&SipProxy.SipPort)->default_value(5060), "SIP port")
    ;

}

ApplicationConfiguration& ApplicationConfiguration::addOptions(Option option)
{
  m_options.insert(option);
  return *this;
}

boost::program_options::options_description ApplicationConfiguration::generateOptions()
{
  po::options_description cmdline_options;
  for (Option option: m_options)
  {
    switch (option)
    {
      case GENERIC:
      {
        cmdline_options.add(m_genericOptions);
        break;
      }
      case SESSION:
      {
        cmdline_options.add(m_sessionOptions);
        break;
      }
      case SENDER:
      {
        cmdline_options.add(m_senderOptions);
        break;
      }
      case RECEIVER:
      {
        cmdline_options.add(m_receiverOptions);
        break;
      }
      case VIDEO:
      {
        cmdline_options.add(m_videoOptions);
        break;
      }
      case AUDIO:
      {
        cmdline_options.add(m_audioOptions);
        break;
      }
      case RTP_RTCP:
      {
        cmdline_options.add(m_rtp_rtcp_options);
        break;
      }
      case NETWORK:
      {
        cmdline_options.add(m_networkOptions);
        break;
      }
      case MULTIPATH:
      {
        cmdline_options.add(m_multipathOptions);
        break;
      }
      case RTPPROXY:
      {
        cmdline_options.add(m_proxyOptions);
        break;
      }
      case SIMULATION:
      {
        cmdline_options.add(m_simulationOptions);
        break;
      }
      case RTSP_SERVER:
      {
        cmdline_options.add(m_rtspServerOptions);
        break;
      }
      case RTSP_CLIENT:
      {
        cmdline_options.add(m_rtspClientOptions);
        break;
      }
      case SIP_CLIENT:
      {
        cmdline_options.add(m_sipClientOptions);
        break;
      }
      case SIP_PROXY:
      {
        cmdline_options.add(m_sipProxyOptions);
        break;
      }
    }
  }
  return cmdline_options;
}

GenericParameters ApplicationConfiguration::getGenericParameters()
{
  GenericParameters applicationParameters;
  for (Option option: m_options)
  {
    switch (option)
    {
      case GENERIC:
      {
        applicationParameters.setBoolParameter(ApplicationParameters::vvv, Application.Verbose);
        break;
      }
      case SESSION:
      {
        applicationParameters.setUintParameter(ApplicationParameters::dur, Session.Duration);
        applicationParameters.setUintParameter(ApplicationParameters::mtu, Session.Mtu);
        break;
      }
      case SENDER:
      {
        if (Sender.UseMediaSampleTime)
        {
          applicationParameters.setBoolParameter(ApplicationParameters::use_media_sample_time, true);
        }
        if (Sender.Scheduler != UINT32_MAX)
        {
          applicationParameters.setUintParameter(ApplicationParameters::scheduler, Sender.Scheduler);
        }
        applicationParameters.setStringParameter(ApplicationParameters::scheduler_param, Sender.SchedulerParam);
        break;
      }
      case RECEIVER:
      {
        applicationParameters.setUintParameter(ApplicationParameters::jitter_buffer_type, Receiver.JitterBufferType);
        applicationParameters.setUintParameter(ApplicationParameters::buf_lat, Receiver.BufferLatency);
        applicationParameters.setStringParameter(ApplicationParameters::pred, Receiver.Predictor);
        applicationParameters.setStringParameter(ApplicationParameters::mp_pred, Receiver.MpPredictor);
        applicationParameters.setDoubleParameter(ApplicationParameters::pto, Receiver.PrematureTimeoutProbability);
        applicationParameters.setUintParameter(ApplicationParameters::mavg_hist, Receiver.MavgHistSize);
        if (Receiver.Analyse)
          applicationParameters.setBoolParameter(ApplicationParameters::analyse, Receiver.Analyse);
        break;
      }
      case VIDEO:
      {
        applicationParameters.setStringParameter(ApplicationParameters::video_device, Video.VideoDevice);
        applicationParameters.setUintParameter(ApplicationParameters::video_fps, Video.Fps);
        applicationParameters.setUintParameter(ApplicationParameters::video_min_kbps, Video.VideoMinKbps);
        applicationParameters.setUintParameter(ApplicationParameters::video_max_kbps, Video.VideoMaxKbps);
        break;
      }
      case AUDIO:
      {
        applicationParameters.setStringParameter(ApplicationParameters::audio_device, Audio.AudioDevice);
        break;
      }
      case RTP_RTCP:
      {
        applicationParameters.setBoolParameter(ApplicationParameters::disable_rtcp, RtpRtcp.DisableRtcp);
        applicationParameters.setBoolParameter(ApplicationParameters::exit_on_bye, RtpRtcp.ExitOnByeReceived);
        applicationParameters.setDoubleParameter(ApplicationParameters::t_rr_interval, RtpRtcp.T_rr_interval);
        applicationParameters.setBoolParameter(ApplicationParameters::enable_mprtp, RtpRtcp.EnableMpRtp);
        applicationParameters.setBoolParameter(ApplicationParameters::rtcp_mux, RtpRtcp.RtcpMux);
        applicationParameters.setBoolParameter(ApplicationParameters::rtcp_rs, RtpRtcp.RtcpRs);
        applicationParameters.setBoolParameter(ApplicationParameters::rtcp_rtp_header_ext, RtpRtcp.RtcpRtpHeaderExt);
        if (RtpRtcp.ForceRtpSn)
        {
          applicationParameters.setUintParameter(ApplicationParameters::rtp_sn, RtpRtcp.RtpSn);
        }
        if (RtpRtcp.ForceRtpTs)
        {
          applicationParameters.setUintParameter(ApplicationParameters::rtp_ts, RtpRtcp.RtpTs);
        }
        if (RtpRtcp.ForceRtpSsrc)
        {
          applicationParameters.setUintParameter(ApplicationParameters::rtp_ssrc, RtpRtcp.RtpSsrc);
        }
        if (RtpRtcp.DisableStap)
          applicationParameters.setBoolParameter(ApplicationParameters::disable_stap, RtpRtcp.DisableStap);
        if (RtpRtcp.RapidSyncMode > 0)
          applicationParameters.setUintParameter(ApplicationParameters::rapid_sync_mode, RtpRtcp.RapidSyncMode);
        if (RtpRtcp.ExtractNtp)
          applicationParameters.setBoolParameter(ApplicationParameters::extract_ntp_ts, RtpRtcp.ExtractNtp);
        if (RtpRtcp.SummariseStats)
          applicationParameters.setBoolParameter(ApplicationParameters::rtp_summarise_stats, RtpRtcp.SummariseStats);

        break;
      }
      case NETWORK:
      {
        applicationParameters.setStringsParameter(ApplicationParameters::local_interfaces, Network.LocalInterfaces);
        if (!Network.FQDN.empty())
          applicationParameters.setStringParameter(ApplicationParameters::fqdn, Network.FQDN);
        if (!Network.Domain.empty())
          applicationParameters.setStringParameter(ApplicationParameters::domain, Network.Domain);
        break;
      }
      case MULTIPATH:
      {
        applicationParameters.setStringParameter(ApplicationParameters::mprtp_sa, Multipath.SchedulingAlgorithm);
        applicationParameters.setBoolParameter(ApplicationParameters::mprtp_psa, Multipath.PathwiseSplitAllowed);
        break;
      }
      case RTPPROXY:
      {
        if (!RtpProxy.ChannelConfigFile.empty())
          applicationParameters.setStringParameter(ApplicationParameters::channel_config, RtpProxy.ChannelConfigFile);
        break;
      }
      case SIMULATION:
      {
        break;
      }
      case RTSP_SERVER:
      {
        applicationParameters.setStringParameter(ApplicationParameters::stream_name, Rtsp.StreamName);
        break;
      }
      case RTSP_CLIENT:
      {
        if (!RtspClient.RtspUri.empty())
          applicationParameters.setStringParameter(ApplicationParameters::rtsp_uri, RtspClient.RtspUri);
        applicationParameters.setUintParameter(ApplicationParameters::dur, RtspClient.Duration);
        if (RtspClient.UseRtpRtsp)
          applicationParameters.setBoolParameter(ApplicationParameters::use_rtp_rtsp, RtspClient.UseRtpRtsp);
        applicationParameters.setUintParameter(ApplicationParameters::client_rtp_port, RtspClient.ClientPort);
        if (RtspClient.OptionsOnly)
          applicationParameters.setBoolParameter(ApplicationParameters::options_only, RtspClient.OptionsOnly);
        if (RtspClient.DescribeOnly)
          applicationParameters.setBoolParameter(ApplicationParameters::describe_only, RtspClient.DescribeOnly);
        if (RtspClient.AudioOnly)
          applicationParameters.setBoolParameter(ApplicationParameters::audio_only, RtspClient.AudioOnly);
        if (RtspClient.VideoOnly)
          applicationParameters.setBoolParameter(ApplicationParameters::video_only, RtspClient.VideoOnly);
        if (RtspClient.ExternalRtp)
          applicationParameters.setBoolParameter(ApplicationParameters::external_rtp, RtspClient.ExternalRtp);
        if (!RtspClient.DestMprtpAddr.empty())
          applicationParameters.setStringsParameter(ApplicationParameters::dest_mprtp_addr, RtspClient.DestMprtpAddr);
        if (!RtspClient.ForceDestMprtpAddr.empty())
          applicationParameters.setStringsParameter(ApplicationParameters::force_dest_mprtp_addr, RtspClient.ForceDestMprtpAddr);
        break;
      }
      case SIP_CLIENT:
      {
        applicationParameters.setStringParameter(ApplicationParameters::sip_user, SipClient.SipUser);
        applicationParameters.setUintParameter(ApplicationParameters::sip_port, SipClient.SipPort);
        if (!SipClient.SipProxy.empty())
        {
          applicationParameters.setStringParameter(ApplicationParameters::sip_proxy, SipClient.SipProxy);
          applicationParameters.setBoolParameter(ApplicationParameters::use_sip_proxy, SipClient.UseSipProxy);
          applicationParameters.setBoolParameter(ApplicationParameters::register_with_proxy, SipClient.RegisterWithProxy);
        }
        if (!SipClient.SipCallee.empty())
          applicationParameters.setStringParameter(ApplicationParameters::sip_callee, SipClient.SipCallee);
        if (!SipClient.MediaConfigurationFile.empty())
          applicationParameters.setStringParameter(ApplicationParameters::media_configuration_file, SipClient.MediaConfigurationFile);
        if (!SipClient.PublicIp.empty())
          applicationParameters.setStringParameter(ApplicationParameters::public_ip, SipClient.PublicIp);

        break;
      }
      case SIP_PROXY:
      {
        applicationParameters.setUintParameter(ApplicationParameters::sip_port, SipClient.SipPort);
        break;
      }
    }
  }
  return applicationParameters;
}

} // app
} // rtp_plus_plus
