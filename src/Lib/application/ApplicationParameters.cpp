#include "CorePch.h"
#include <rtp++/application/ApplicationParameters.h>

namespace rtp_plus_plus
{
namespace app
{

// parameter names
const std::string ApplicationParameters::help = "help,h";
const std::string ApplicationParameters::vvv = "vvv,v";
const std::string ApplicationParameters::logfile = "logfile,l";
const std::string ApplicationParameters::sdp = "sdp";
const std::string ApplicationParameters::nc = "nc";
const std::string ApplicationParameters::aout = "aout";
const std::string ApplicationParameters::vout = "vout";
const std::string ApplicationParameters::mtu = "mtu";
const std::string ApplicationParameters::jitter_buffer_type = "jitter_buffer_type,j";
const std::string ApplicationParameters::buf_lat = "buf-lat,b";
const std::string ApplicationParameters::exit_on_bye = "exit-on-bye";
const std::string ApplicationParameters::analyse = "analyse";
const std::string ApplicationParameters::extract_ntp_ts = "extract-ntp,x";
const std::string ApplicationParameters::source = "source";
// const std::string ApplicationParameters::fps = "fps";
const std::string ApplicationParameters::limit_rate = "limit-rate";
const std::string ApplicationParameters::v_p = "v-p";
const std::string ApplicationParameters::v_tot = "v-tot";
const std::string ApplicationParameters::v_pc = "v-pc";
const std::string ApplicationParameters::v_ps = "v-ps";
const std::string ApplicationParameters::v_loop = "v-loop";
const std::string ApplicationParameters::v_loop_count = "v-loop-count";
const std::string ApplicationParameters::a_r = "a-r";
const std::string ApplicationParameters::a_p = "a-p";
const std::string ApplicationParameters::a_pc = "a-pc";
const std::string ApplicationParameters::a_ps = "a-ps";
const std::string ApplicationParameters::a_tot = "a-tot";
const std::string ApplicationParameters::r_nc = "r-nc";
const std::string ApplicationParameters::l_nc = "l-nc";
const std::string ApplicationParameters::s_nc = "s-nc";
const std::string ApplicationParameters::interface_conf = "interface_conf,i";
const std::string ApplicationParameters::fwd_conf = "fwd_conf,f";
const std::string ApplicationParameters::strict = "strict";
const std::string ApplicationParameters::loss = "loss";
const std::string ApplicationParameters::owd_ms = "owd-ms";
const std::string ApplicationParameters::jitter_ms = "jitter-ms";
const std::string ApplicationParameters::channel_config = "channel-config";
const std::string ApplicationParameters::disable_rtcp = "disable-rtcp";
const std::string ApplicationParameters::is_sender = "is-sender";
const std::string ApplicationParameters::use_media_sample_time = "use-media-sample-time";
const std::string ApplicationParameters::force_rtp_sn = "force-rtp-sn";
const std::string ApplicationParameters::rtp_sn = "rtp-sn";
const std::string ApplicationParameters::rtp_summarise_stats = "rtp-summarise-stats";
const std::string ApplicationParameters::force_rtp_ts = "force-rtp-ts";
const std::string ApplicationParameters::rtp_ts = "rtp-ts";
const std::string ApplicationParameters::force_rtp_ssrc = "force-rtp-ssrc";
const std::string ApplicationParameters::rtp_ssrc = "rtp-ssrc";
const std::string ApplicationParameters::sctp_policies = "sctp-policy";
const std::string ApplicationParameters::sctp_dont_connect = "sctp-dont-connect";
const std::string ApplicationParameters::sctp_prio = "sctp-prio";
const std::string ApplicationParameters::mprtp_sa = "mprtp-sa";
const std::string ApplicationParameters::mprtp_psa = "mprtp-psa";
const std::string ApplicationParameters::pred = "pred";
const std::string ApplicationParameters::mp_pred = "mp-pred";
const std::string ApplicationParameters::dur = "dur,d";
const std::string ApplicationParameters::mavg_hist = "mavg-hist";
const std::string ApplicationParameters::pto = "pto";
const std::string ApplicationParameters::t_rr_interval = "T_rr_interval";
const std::string ApplicationParameters::disable_stap = "disable-stap";
const std::string ApplicationParameters::rapid_sync_mode = "rapid-sync-mode";
const std::string ApplicationParameters::scheduler = "scheduler";
const std::string ApplicationParameters::scheduler_param = "scheduler-param";
const std::string ApplicationParameters::stream_name = "stream-name";
const std::string ApplicationParameters::video_device = "video-device";
const std::string ApplicationParameters::audio_device = "audio-device";
const std::string ApplicationParameters::rtsp_uri = "rtsp-uri";
const std::string ApplicationParameters::rtsp_port = "rtsp-port,p";
const std::string ApplicationParameters::rtsp_rtp_port = "rtsp-rtp-port";
const std::string ApplicationParameters::use_rtp_rtsp = "use-rtp-rtsp,t";
const std::string ApplicationParameters::rtp_session_timeout = "rtp-session-timeout";
const std::string ApplicationParameters::local_interfaces = "local-interfaces";
const std::string ApplicationParameters::client_rtp_port = "client-rtp-port,r";
const std::string ApplicationParameters::options_only = "options-only,O";
const std::string ApplicationParameters::describe_only = "describe-only,D";
const std::string ApplicationParameters::audio_only = "audio-only,A";
const std::string ApplicationParameters::video_only = "video-only,V";
const std::string ApplicationParameters::external_rtp = "external-rtp,x";
const std::string ApplicationParameters::dest_mprtp_addr = "dest-mprtp-addr";
const std::string ApplicationParameters::force_dest_mprtp_addr = "force-dest-mprtp-addr";
const std::string ApplicationParameters::enable_mprtp = "enable-mprtp";
const std::string ApplicationParameters::rtcp_mux = "rtcp-mux";
const std::string ApplicationParameters::rtcp_rtp_header_ext = "rtcp-rtp-header-ext";
const std::string ApplicationParameters::sip_user = "sip-user";
const std::string ApplicationParameters::fqdn = "fqdn";
const std::string ApplicationParameters::media_configuration_file = "media-configuration-file";
const std::string ApplicationParameters::sip_port = "sip-port,p";
const std::string ApplicationParameters::sip_proxy = "sip-proxy";
const std::string ApplicationParameters::use_sip_proxy = "use-sip-proxy";
const std::string ApplicationParameters::register_with_proxy = "register-with-proxy";
const std::string ApplicationParameters::sip_callee = "sip-callee";
const std::string ApplicationParameters::domain = "domain";
const std::string ApplicationParameters::public_ip = "public-ip";
// parameter values
const uint32_t ApplicationParameters::defaultMtu = 1500;
const std::string ApplicationParameters::mavg = "mavg";
const std::string ApplicationParameters::ar2 = "ar2";
const std::string ApplicationParameters::simple = "simple";

const std::string ApplicationParameters::mp_single = "mp-single";
const std::string ApplicationParameters::mp_cross = "mp-cross";
const std::string ApplicationParameters::mp_comp = "mp-comp";

const std::string ApplicationParameters::sctp_prio_base_layer = "prio-bl";
const std::string ApplicationParameters::sctp_prio_scalability = "prio-sc";

const std::string ApplicationParameters::protocol = "protocol";
const std::string ApplicationParameters::rtp_profile = "rtp-profile";
const std::string ApplicationParameters::rtp_port = "rtp-port";
const std::string ApplicationParameters::remote_host = "remote-host";
const std::string ApplicationParameters::rtcp_fb = "rtcp-fb";
const std::string ApplicationParameters::rtcp_xr = "rtcp-xr";
const std::string ApplicationParameters::rtcp_rs = "rtcp-rs";
const std::string ApplicationParameters::rtx_time = "rtx-time";

const std::string ApplicationParameters::no_audio = "no-audio";
const std::string ApplicationParameters::audio_media_type = "audio-media-type";

const std::string ApplicationParameters::no_video = "no-video";
const std::string ApplicationParameters::video_media_type = "video-media-type";
const std::string ApplicationParameters::video_codec = "video-codec";
const std::string ApplicationParameters::video_fps = "video-fps";
const std::string ApplicationParameters::video_kbps = "video-kbps";
const std::string ApplicationParameters::video_min_kbps = "video-min-kbps";
const std::string ApplicationParameters::video_max_kbps = "video-max-kbps";
const std::string ApplicationParameters::video_width = "width";
const std::string ApplicationParameters::video_height = "height";
const std::string ApplicationParameters::video_codec_impl = "vc-impl";
const std::string ApplicationParameters::video_codec_param = "vc-param";

const std::string ApplicationParameters::rtc_server = "rtc-server";
const std::string ApplicationParameters::rpc_port = "rpc-port";
const std::string ApplicationParameters::rtc_offerer =  "rtc-offerer";
const std::string ApplicationParameters::rtc_answerer = "rtc-answerer";
const std::string ApplicationParameters::rtc_config = "rtc-config";
const std::string ApplicationParameters::rtc_config_string = "rtc-config-string";
const std::string ApplicationParameters::rtc_kill_session = "rtc-kill-session";

} // app
} // rtp_plus_plus
