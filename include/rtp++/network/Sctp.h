#pragma once

namespace rtp_plus_plus
{
namespace sctp
{

#define LINE_LENGTH (1024)
#define BUFFER_SIZE (1<<16)

#define DATA_CHANNEL_PPID_CONTROL   50
#define DATA_CHANNEL_PPID_DOMSTRING 51
#define DATA_CHANNEL_PPID_BINARY    52

#define DATA_CHANNEL_CLOSED     0
#define DATA_CHANNEL_CONNECTING 1
#define DATA_CHANNEL_OPEN       2
#define DATA_CHANNEL_CLOSING    3

enum PrPolicy
{
  SCTP_PR_POLICY_SCTP_NONE = 0x0000, /* Reliable transfer */
  SCTP_PR_POLICY_SCTP_TTL  = 0x0001, /* Time based PR-SCTP */
  SCTP_PR_POLICY_SCTP_BUF  = 0x0002, /* Buffer based PR-SCTP */
  SCTP_PR_POLICY_SCTP_RTX  = 0x0003  /* Number of retransmissions based PR-SCTP */
};


/*

struct channel {
  uint32_t id;
  uint32_t pr_value;
  uint16_t pr_policy;
  uint16_t i_stream;
  uint16_t o_stream;
  uint8_t unordered;
  uint8_t state;
};

struct peer_connection {
  struct channel channels[NUMBER_OF_CHANNELS];
  struct channel *i_stream_channel[NUMBER_OF_STREAMS];
  struct channel *o_stream_channel[NUMBER_OF_STREAMS];
  uint16_t o_stream_buffer[NUMBER_OF_STREAMS];
  uint32_t o_stream_buffer_counter;
#if defined(__Userspace_os_Windows)
  CRITICAL_SECTION mutex;
#else
  pthread_mutex_t mutex;
#endif
  struct socket *sock;
} peer_connection;
*/



#define DATA_CHANNEL_OPEN_REQUEST  0
#define DATA_CHANNEL_OPEN_RESPONSE 1
#define DATA_CHANNEL_ACK           2

#define DATA_CHANNEL_RELIABLE                0
#define DATA_CHANNEL_RELIABLE_STREAM         1
#define DATA_CHANNEL_UNRELIABLE              2
#define DATA_CHANNEL_PARTIAL_RELIABLE_REXMIT 3
#define DATA_CHANNEL_PARTIAL_RELIABLE_TIMED  4

#define DATA_CHANNEL_FLAG_OUT_OF_ORDER_ALLOWED 0x0001

}
}

