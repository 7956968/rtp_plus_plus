#pragma once

#ifdef ENABLE_SCTP_USERLAND
#include <sys/types.h>
#if defined(__Userspace_os_Windows)
#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <crtdbg.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#endif

#include <boost/function.hpp>
#include <usrsctplib/usrsctp.h>
#include <rtp++/network/EndPoint.h>
#include <rtp++/network/NetworkPacket.h>
#include <rtp++/network/Sctp.h>

#define NUMBER_OF_CHANNELS (100)
#define NUMBER_OF_STREAMS (100)

namespace rtp_plus_plus
{
namespace sctp
{

#if !defined(__Userspace_os_Windows)
#define SCTP_PACKED __attribute__((packed))
#else
#pragma pack (push, 1)
#define SCTP_PACKED
#endif

struct rtcweb_datachannel_open_request {
  uint8_t msg_type; /* DATA_CHANNEL_OPEN_REQUEST */
  uint8_t channel_type;
  uint16_t flags;
  uint16_t reliability_params;
  int16_t priority;
  char label[];
} SCTP_PACKED;


struct rtcweb_datachannel_open_response {
  uint8_t  msg_type; /* DATA_CHANNEL_OPEN_RESPONSE */
  uint8_t  error;
  uint16_t flags;
  uint16_t reverse_stream;
} SCTP_PACKED;

struct rtcweb_datachannel_ack {
  uint8_t  msg_type; /* DATA_CHANNEL_ACK */
} SCTP_PACKED;

#if defined(__Userspace_os_Windows)
#pragma pack()
#endif

#undef SCTP_PACKED

struct channel {
  uint32_t id;
  uint32_t pr_value;
  PrPolicy pr_policy;
  uint16_t i_stream;
  uint16_t o_stream;
  bool unordered;
  uint8_t state;
};

/**
  * callback to receive SCTP messages
  */
static int receive_cb(struct socket *sock, union sctp_sockstore addr, void *data,
           size_t datalen, struct sctp_rcvinfo rcv, int flags, void *ulp_info);

class peer_connection
{
public:
  typedef boost::function<void (uint32_t, const NetworkPacket&)> SctpRecvCb_t;
  typedef boost::function<void (uint8_t* pData, uint32_t)> SctpSendFailedCb_t;

  peer_connection(uint16_t uiSctpPort, uint16_t uiLocalUdpEncapsPort, uint16_t uiRemoteUdpEncapsPort, bool bActivateSctp);

  void setRecvCallback(SctpRecvCb_t onSctpRecv){ m_onSctpRecv = onSctpRecv; }
  void setSendFailedCallback(SctpSendFailedCb_t onSendFailed){ m_onSendFailed = onSendFailed; }
  /**
   * Connects the SCTP socket.
   * returns true if the connection succeeds
   */
  bool connect(EndPoint& endPoint);

  bool isConnected() const { return m_bIsConnected; }

  bool bind();
  bool listen();

  bool shutdown();

  channel* find_channel_by_i_stream(uint16_t i_stream);
  channel* find_channel_by_o_stream(uint16_t o_stream);
  channel* find_free_channel();
  uint16_t find_free_o_stream();

  void request_more_o_streams();

  channel* open_channel(bool bUnordered, PrPolicy ePrPolicy, uint32_t pr_value);
  void close_channel(uint32_t id);

  bool setChannelPriority(uint16_t uiChannel, uint16_t uiPriority);

  int send_user_message(size_t id, char *message, size_t length);
  int send_open_request_message(struct socket *sock, uint16_t o_stream, bool bUnordered, PrPolicy ePrPolicy, uint32_t pr_value);
  int send_open_response_message(struct socket *sock, uint16_t o_stream, uint16_t i_stream);
  int send_open_ack_message(struct socket *sock, uint16_t o_stream);

  void reset_outgoing_stream(uint16_t o_stream);

  void send_outgoing_stream_reset();


  void handle_open_request_message(rtcweb_datachannel_open_request *req,
                              size_t length,
                              uint16_t i_stream);

  void handle_open_response_message(rtcweb_datachannel_open_response *rsp,
                               size_t length, uint16_t i_stream);

  void handle_open_ack_message(rtcweb_datachannel_ack *ack,
                          size_t length, uint16_t i_stream);

  void handle_unknown_message(char *msg, size_t length, uint16_t i_stream);

  void handle_data_message(char *buffer, size_t length, uint16_t i_stream);

  void handle_message(char *buffer, size_t length, uint32_t ppid, uint16_t i_stream);

  void handle_association_change_event(struct socket *sock, struct sctp_assoc_change *sac);

  void handle_peer_address_change_event(struct sctp_paddr_change *spc);

  void handle_adaptation_indication(struct sctp_adaptation_event *sai);

  void handle_shutdown_event(struct sctp_shutdown_event *sse);

  void handle_stream_reset_event(struct sctp_stream_reset_event *strrst);

  void handle_stream_change_event(struct sctp_stream_change_event *strchg);

  void handle_remote_error_event(struct sctp_remote_error *sre);

  void handle_send_failed_event(struct sctp_send_failed_event *ssfe);

  void handle_notification(struct socket *sock, union sctp_notification *notif, size_t n);

  void print_status();

public:
  void lock_peer_connection();
  void unlock_peer_connection();

private:
  // rtp++ variables
  bool m_bIsConnected;

  uint16_t m_uiSctpPort;

  // TODO: add method configuring local and remote UDP encapsulation ports
#if 0
  uint16_t m_uiLocalUdpEncapsPort;
  uint16_t m_uiRemoteUdpEncapsPort;
#endif

private:
  // libusrsctp variables
  channel m_channels[NUMBER_OF_CHANNELS];
  channel* m_i_stream_channel[NUMBER_OF_STREAMS];
  channel* m_o_stream_channel[NUMBER_OF_STREAMS];
  uint16_t m_o_stream_buffer[NUMBER_OF_STREAMS];
  uint32_t m_o_stream_buffer_counter;
#if defined(__Userspace_os_Windows)
  CRITICAL_SECTION m_mutex;
#else
  pthread_mutex_t m_mutex;
#endif
  struct socket* m_sock;
  struct sockaddr_in m_addr;

  SctpRecvCb_t m_onSctpRecv;
  SctpSendFailedCb_t m_onSendFailed;
};

}
}

#endif

