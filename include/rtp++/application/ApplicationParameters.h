#pragma once
#include <cstdint>
#include <string>

namespace rtp_plus_plus 
{
namespace app
{

/**
  * @brief Common application parameters for RTP applications.
  *
  * The parameter names map onto their text version by replacing underscores "_" with dashes "-".
  */
class ApplicationParameters
{
public:

  /**
   * help prints the available command line arguments.
   * Shortcut: -h
   */
  static const std::string help;
  /**
   * logging verbosity.
   * Shortcut: -v
   */
  static const std::string vvv;
  static const std::string logfile;
  static const std::string mtu;
  static const std::string jitter_buffer_type;
  static const std::string buf_lat;
  static const std::string extract_ntp_ts;
  static const std::string disable_rtcp;
  static const std::string exit_on_bye;
  static const std::string analyse;
  // if the application is starting out as a sender
  static const std::string is_sender;
  static const std::string use_media_sample_time;
  static const std::string force_rtp_sn;
  static const std::string rtp_sn;
  static const std::string rtp_summarise_stats;
  static const std::string force_rtp_ts;
  static const std::string rtp_ts;
  static const std::string force_rtp_ssrc;
  static const std::string rtp_ssrc;
  static const std::string sctp_policies;
  static const std::string sctp_dont_connect;
  static const std::string sctp_prio;
  static const std::string sdp;
  static const std::string nc;
  static const std::string aout;
  static const std::string vout;
  static const std::string source;
//  static const std::string fps;
  static const std::string limit_rate;
  static const std::string v_p;
  static const std::string v_tot;
  static const std::string v_pc;
  static const std::string v_ps;
  static const std::string v_loop;
  static const std::string v_loop_count;
  static const std::string a_r;
  static const std::string a_p;
  static const std::string a_pc;
  static const std::string a_ps;
  static const std::string a_tot;
  static const std::string r_nc;
  static const std::string l_nc;
  static const std::string s_nc;
  static const std::string interface_conf;
  static const std::string fwd_conf;
  static const std::string strict;
  static const std::string loss;
  static const std::string owd_ms;
  static const std::string jitter_ms;
  static const std::string channel_config;
  static const std::string t_rr_interval;
  static const std::string disable_stap;
  static const std::string rapid_sync_mode;
  static const std::string scheduler;
  static const std::string scheduler_param;

  static const std::string video_device;
  static const std::string audio_device;
  static const std::string stream_name;

  static const std::string rtsp_uri;
  static const std::string use_rtp_rtsp;
  static const std::string rtsp_port;
  static const std::string rtp_session_timeout;
  static const std::string rtsp_rtp_port;
  /**
   * e.g. 127.0.0.1
   */
  static const std::string local_interfaces;
  static const std::string client_rtp_port;
  static const std::string options_only;
  static const std::string describe_only;
  static const std::string audio_only;
  static const std::string video_only;
  static const std::string external_rtp;
  /**
   * e.g. dest_mprtp_addr="1 192.0.2.1 49170; 2 198.51.100.1 51372";
   */
  static const std::string dest_mprtp_addr;
  /**
   * e.g. force_dest_mprtp_addr="1 192.0.2.1 49170; 2 198.51.100.1 51372";
   */
  static const std::string force_dest_mprtp_addr;
  /// enable mprtp support
  static const std::string enable_mprtp;
  /// enable rtcp-mux
  static const std::string rtcp_mux;
  /// enable rtcp-rtp-header-ext
  static const std::string rtcp_rtp_header_ext;
  /// SIP port
  static const std::string sip_port;
  /// SIP proxy URI
  static const std::string sip_proxy;
  /// Force SIP proxy URI
  static const std::string use_sip_proxy;
  /// Register with proxy
  static const std::string register_with_proxy;
  /// SIP callee
  static const std::string sip_callee;
  /// SIP local domain
  static const std::string domain;
  /// Public IP used in offer/answer through NAT
  static const std::string public_ip;
  /// SIP user
  static const std::string sip_user;
  /// SIP FQDN
  static const std::string fqdn;
  /// Media configuration file
  static const std::string media_configuration_file;
  /// MPRTP scheduling algorithm
  static const std::string mprtp_sa;
  /// MPRTP pathwise split of media frame allowed
  static const std::string mprtp_psa;
  /// predictor command line names
  static const std::string pred;
  static const std::string mp_pred;
  /// application duration
  static const std::string dur;
  /// moving average history size
  static const std::string mavg_hist;
  /// premature timeout probability
  static const std::string pto;
  /// MTU
  static const uint32_t defaultMtu;
  /// predictor values
  static const std::string mavg;
  static const std::string ar2;
  static const std::string simple;
  /// mp predictor values
  static const std::string mp_comp;
  static const std::string mp_single;
  static const std::string mp_cross;

  static const std::string sctp_prio_base_layer;
  static const std::string sctp_prio_scalability;

  static const std::string protocol;
  static const std::string rtp_profile;
  static const std::string rtp_port;
  static const std::string remote_host;
  static const std::string rtcp_fb;
  static const std::string rtcp_xr;
  static const std::string rtcp_rs;
  static const std::string rtx_time;

  // RTC server options
  static const std::string no_audio;
  static const std::string audio_media_type;

  static const std::string no_video;
  static const std::string video_media_type;
  static const std::string video_codec;
  static const std::string video_kbps;
  static const std::string video_fps;
  static const std::string video_min_kbps;
  static const std::string video_max_kbps;
  static const std::string video_width;
  static const std::string video_height;

  static const std::string video_codec_impl;
  static const std::string video_codec_param;

  // RTC client options
  static const std::string rpc_port;
  static const std::string rtc_server;
  static const std::string rtc_offerer;
  static const std::string rtc_answerer;
  static const std::string rtc_config;
  static const std::string rtc_config_string;
  static const std::string rtc_kill_session;
};

} // app
} // rtp_plus_plus
