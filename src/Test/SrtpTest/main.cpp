#include "SrtpTestPch.h"
#include <srtp.h>
#include <RtpPacket.h>
#include <RtpPacketiser.h>

using namespace rtp_plus_plus;

int main(int argc, char** argv)
{
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);

  uint32_t uiSsrc = 12345;
  ssrc_t ssrc;
  ssrc.type = ssrc_specific;
  ssrc.value = uiSsrc;

  srtp_t session_sender;
  srtp_t session_receiver;
  srtp_policy_t policy;
  uint8_t key[30];

  // initialize libSRTP
  err_status_t err = srtp_init();
  if (err)
  {
    LOG(WARNING) << "Error in init SRTP: " << err;
    return 0;
  }

  // set policy to describe a policy for an SRTP stream
  crypto_policy_set_rtp_default(&policy.rtp);
  crypto_policy_set_rtcp_default(&policy.rtcp);
  policy.ssrc = ssrc;
  policy.key  = key;
  policy.next = NULL;
  //policy.ssrc.type  = ssrc_specific;
  //policy.ssrc.value = ssrc;
  policy.key  = (uint8_t *) key;
  policy.ekt  = NULL;
  policy.next = NULL;
  policy.window_size = 128;
  policy.allow_repeat_tx = 0;
  policy.rtp.sec_serv = sec_serv_conf_and_auth;
  //policy.rtcp.sec_serv = sec_serv_conf_and_auth;  /* we don't do RTCP anyway */
  policy.rtcp.sec_serv = sec_serv_none;  /* we don't do RTCP anyway */

  // set key to random value
  err = crypto_get_random(key, 30);
  if (err)
  {
    LOG(WARNING) << "Error in crypto get random key: " << err;
    return 0;
  }

  // allocate and initialize the SRTP session
  err = srtp_create(&session_sender, &policy);
  if (err)
  {
    LOG(WARNING) << "Error in srtp sender session create: " << err;
    return 0;
  }

  // allocate and initialize the SRTP session
  err = srtp_create(&session_receiver, &policy);
  if (err)
  {
    LOG(WARNING) << "Error in srtp receiver session create: " << err;
    return 0;
  }

  rfc3550::RtpHeader rtpHeader;
  rtpHeader.setPayloadType(96);
  rtpHeader.setRtpTimestamp(1);
  rtpHeader.setSequenceNumber(1);
  rtpHeader.setSSRC(uiSsrc);

  RtpPacket rtpPacket;
  rtpPacket.setRtpHeader(rtpHeader);

  Buffer rtp_buffer(new uint8_t[11], 11);

  rtp_buffer[0] = 'H';
  rtp_buffer[1] = 'E';
  rtp_buffer[2] = 'L';
  rtp_buffer[3] = 'L';
  rtp_buffer[4] = 'O';
  rtp_buffer[5] = ' ';
  rtp_buffer[6] = 'S';
  rtp_buffer[7] = 'R';
  rtp_buffer[8] = 'T';
  rtp_buffer[9] = 'P';
  rtp_buffer[10] = '\0';
  rtpPacket.setPayload(rtp_buffer);

  RtpPacketiser packetiser;
  Buffer packetisedRtpPacket = packetiser.packetise(rtpPacket);

  // create new packet with space to copy encrypted data to
  Buffer packetisedRtpPacketCopy(new uint8_t[2048], 2048);
  memcpy((void*)packetisedRtpPacketCopy.data(), (void*)packetisedRtpPacket.data(), packetisedRtpPacket.getSize());

  int len = packetisedRtpPacket.getSize();

  LOG(INFO) << "Before SRTP: " << rtp_buffer.data() << " (" << len << ")";

  // protected packet
  err = srtp_protect(session_sender, (void*)packetisedRtpPacketCopy.data(), &len);
  if (err)
  {
    LOG(WARNING) << "Error encoding SRTP: " << err;
    return 0;
  }
  else
  {
    packetisedRtpPacketCopy[len] = '\0';
    LOG(INFO) << "After SRTP protect: " << &packetisedRtpPacketCopy[12]  << " (" << len << ")";
  }

  err = srtp_unprotect(session_receiver, (void*)packetisedRtpPacketCopy.data(), &len);

  if (err)
  {
    LOG(WARNING) << "Error decoding SRTP: " << err;
  }
  else
  {
    packetisedRtpPacketCopy[len] = '\0';
    LOG(INFO) << "After SRTP unprotect: " << &packetisedRtpPacketCopy[12] << " (" << len << ")";
  }

  srtp_shutdown();
  return 0;
}

