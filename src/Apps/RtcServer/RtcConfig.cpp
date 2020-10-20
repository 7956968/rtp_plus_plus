#include "CorePch.h"
#include "RtcConfig.h"
#include <rtp++/application/ApplicationParameters.h>
#include <rtp++/rfc3550/Rfc3550.h>

namespace po = boost::program_options;

namespace rtp_plus_plus {

using namespace app;

RtcConfigOptions::RtcConfigOptions()
  :m_options("Options")
{
  m_options.add_options()
      (ApplicationParameters::help.c_str(), "produce help message")
      (ApplicationParameters::logfile.c_str(), po::value<std::string>(&RtcParameters.logfile), "Logfile")
      (ApplicationParameters::dur.c_str(), po::value<uint32_t>(&RtcParameters.Duration)->default_value(0), "Session duration (milliseconds )(0 = no limit)")
      (ApplicationParameters::exit_on_bye.c_str(), po::bool_switch(&RtcParameters.ExitOnByeReceived)->default_value(false), "Exit on RTCP BYE received")

      (ApplicationParameters::no_audio.c_str(), po::bool_switch(&RtcParameters.NoAudio)->default_value(false), "No audio")
      (ApplicationParameters::audio_media_type.c_str(), po::value<std::string>(&RtcParameters.AudioMediaType), "Audio media type")
      (ApplicationParameters::audio_device.c_str(), po::value<std::string>(&RtcParameters.AudioDevice)->default_value("gen"), "media source = [gen, stdin, <<device_name>>, <<file_name>>]")
      (ApplicationParameters::a_r.c_str(), po::value<uint32_t>(&RtcParameters.RateMs)->default_value(20), "audio rate (ms)")
      (ApplicationParameters::a_ps.c_str(), po::value<std::string>(&RtcParameters.AudioPayloadSizeString)->default_value("160"), "audio payload size = [a|b|c]")

      (ApplicationParameters::no_video.c_str(), po::bool_switch(&RtcParameters.NoVideo)->default_value(false), "No video")
      (ApplicationParameters::video_media_type.c_str(), po::value<std::string>(&RtcParameters.VideoMediaType), "Video media type")
      (ApplicationParameters::video_device.c_str(), po::value<std::string>(&RtcParameters.VideoDevice)->default_value("gen"), "media source = [<<device_name>>, <<file_name>>, model:<<model>>]")
      (ApplicationParameters::video_codec.c_str(), po::value<std::string>(&RtcParameters.VideoCodec), "H264,H265")
      (ApplicationParameters::video_codec_impl.c_str(), po::value<std::string>(&RtcParameters.VideoCodecImpl), "vpp,x264,openh264")
      (ApplicationParameters::video_codec_param.c_str(), po::value<std::vector<std::string> >(&RtcParameters.VideoCodecParams), "Video codec parameters")
      (ApplicationParameters::video_fps.c_str(), po::value<uint32_t>(&RtcParameters.Fps)->default_value(25), "Video fps")
      (ApplicationParameters::video_kbps.c_str(), po::value<std::string>(&RtcParameters.VideoKbps)->default_value("100"), "Video kbps")
      (ApplicationParameters::video_min_kbps.c_str(), po::value<uint32_t>(&RtcParameters.VideoMinKbps)->default_value(50), "Video min encoder bitrate kbps")
      (ApplicationParameters::video_max_kbps.c_str(), po::value<uint32_t>(&RtcParameters.VideoMaxKbps)->default_value(5000), "Video max encoder bitrate kbps")
      (ApplicationParameters::video_width.c_str(), po::value<uint32_t>(&RtcParameters.Width)->default_value(0), "Video width (only applies to live enc)")
      (ApplicationParameters::video_height.c_str(), po::value<uint32_t>(&RtcParameters.Height)->default_value(0), "Video height (only applies to live enc)")
      (ApplicationParameters::v_ps.c_str(), po::value<std::string>(&RtcParameters.VideoPayloadSizeString)->default_value("1400"), "video payload size = [a|b|c]")
      (ApplicationParameters::v_loop_count.c_str(), po::value<uint32_t>(&RtcParameters.LoopCount)->default_value(0), "Loop count. 0 = infinite. Only applies if source=file.264")

      (ApplicationParameters::local_interfaces.c_str(), po::value<std::vector<std::string> >(&RtcParameters.LocalInterfaces), "Local network interfaces")
      (ApplicationParameters::protocol.c_str(), po::value<std::string>(&RtcParameters.Protocol)->default_value("udp"), "Protocol [udp|tcp|sctp|dccp]")
      (ApplicationParameters::rtp_profile.c_str(), po::value<std::string>(&RtcParameters.RtpProfile)->default_value(rfc3550::AVP), "RTP profile [AVP|AVPF]")
      (ApplicationParameters::rtp_port.c_str(), po::value<uint16_t>(&RtcParameters.RtpPort)->default_value(49170), "Preferred RTP port")
      (ApplicationParameters::rtcp_fb.c_str(), po::value<std::vector<std::string> >(&RtcParameters.RtcpFb), "RTCP FB: [nack|ack]")
      (ApplicationParameters::rtcp_xr.c_str(), po::value<std::vector<std::string> >(&RtcParameters.RtcpXr), "RTCP XR: [rcvr-rtt]")
      (ApplicationParameters::rtcp_rs.c_str(), po::bool_switch(&RtcParameters.RtcpRs)->default_value(false), "RTCP reduced size")
      (ApplicationParameters::rtcp_rtp_header_ext.c_str(), po::bool_switch(&RtcParameters.RtcpRtpHeaderExt)->default_value(false), "RTCP RTP Header Ext")
      (ApplicationParameters::rtx_time.c_str(), po::value<uint32_t>(&RtcParameters.RtxTime)->default_value(0), "rtx-time (ms)")
      (ApplicationParameters::enable_mprtp.c_str(), po::bool_switch(&RtcParameters.EnableMpRtp)->default_value(false), "Enable MPRTP support")
      (ApplicationParameters::rtcp_mux.c_str(), po::bool_switch(&RtcParameters.RtcpMux)->default_value(false), "Enable RTCP-mux support")
      (ApplicationParameters::rtp_ssrc.c_str(), po::value<uint32_t>(&RtcParameters.RtpSsrc), "RTP SSRC")
      (ApplicationParameters::rtp_sn.c_str(), po::value<uint16_t>(&RtcParameters.RtpSn)->default_value(1), "RTP sequence number")
      (ApplicationParameters::rtp_ts.c_str(), po::value<uint32_t>(&RtcParameters.RtpTs)->default_value(0), "RTP timestamp base")
      (ApplicationParameters::rtp_summarise_stats.c_str(), po::bool_switch(&RtcParameters.SummariseStats)->default_value(false), "Summarise RTP session stats")
      (ApplicationParameters::rapid_sync_mode.c_str(), po::value<uint32_t>(&RtcParameters.RapidSyncMode)->default_value(0), "Rapid Sync Mode (0 = none, 1 = all")
      (ApplicationParameters::extract_ntp_ts.c_str(), po::bool_switch(&RtcParameters.ExtractNtp)->default_value(false), "Extract NTP TS from RTP Payload")

      (ApplicationParameters::analyse.c_str(), po::bool_switch(&RtcParameters.Analyse)->default_value(false), "Analyse RTP stats such as loss")
      (ApplicationParameters::buf_lat.c_str(), po::value<uint32_t>(&RtcParameters.BufferLatency)->default_value(100), "RTP playout buffer latency")
      (ApplicationParameters::pred.c_str(), po::value<std::string>(&RtcParameters.Predictor), "Predictor to be used in RTO [mavg|ar2]")
      (ApplicationParameters::mp_pred.c_str(), po::value<std::string>(&RtcParameters.MpPredictor), "Predictor to be used in MPRTP [mp-single|mp-cross|mp-comp]")
      (ApplicationParameters::pto.c_str(), po::value<double>(&RtcParameters.PrematureTimeoutProbability)->default_value(0.05), "Premature timeout probability: (0.0, 1)")
      (ApplicationParameters::mavg_hist.c_str(), po::value<uint32_t>(&RtcParameters.MavgHistSize)->default_value(10), "Moving average history")

      (ApplicationParameters::scheduler.c_str(), po::value<uint32_t>(&RtcParameters.Scheduler)->default_value(UINT32_MAX), "Scheduler")
      (ApplicationParameters::scheduler_param.c_str(), po::value<std::string>(&RtcParameters.SchedulerParam)->default_value(""), "Scheduler param")
      (ApplicationParameters::sctp_policies.c_str(), po::value<std::vector<std::string> >(&RtcParameters.SctpPolicies), "SCTP retransmission policies")
      (ApplicationParameters::sctp_prio.c_str(), po::value<std::string>(&RtcParameters.SctpPrio), "SCTP prioritisation")
      (ApplicationParameters::mprtp_sa.c_str(), po::value<std::string>(&RtcParameters.MpSchedulingAlgorithm)->default_value("rr"), "MPRTP scheduling: algorithm = [ rr | rnd | \"x-y-z...\" ]")
      (ApplicationParameters::media_configuration_file.c_str(), po::value<std::string>(&RtcParameters.MediaConfigurationFile), "Media configuration file")
      (ApplicationParameters::public_ip.c_str(), po::value<std::string>(&RtcParameters.PublicIp), "Public IP (NAT)")
      (ApplicationParameters::rpc_port.c_str(), po::value<uint16_t>(&RtcParameters.RpcPort)->default_value(50051), "RPC port")
      ;
}

boost::program_options::options_description RtcConfigOptions::generateOptions()
{
  po::options_description cmdline_options;
  cmdline_options.add(m_options);
  return cmdline_options;
}

} // rtp_plus_plus
