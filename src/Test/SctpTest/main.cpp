#include "SctpTestPch.h"

#include <fstream>
#include <sstream>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <rtp++/application/ApplicationUtil.h>

#define SCTP_TEST
// libusrsctp
#ifdef SCTP_TEST
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__Userspace_os_Windows)
#include <unistd.h>
#endif
#include <sys/types.h>
#if !defined(__Userspace_os_Windows)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <usrsctplib/usrsctp.h>
#endif

using namespace std;
using namespace rtp_plus_plus;

namespace po = boost::program_options;

int done = 0;

#define USE_KERNEL_VERSION

void
debug_printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}

static int
receive_cb(struct socket *sock, union sctp_sockstore addr, void *data,
           size_t datalen, struct sctp_rcvinfo rcv, int flags, void *ulp_info)
{
    if (data == NULL) {
        done = 1;
        usrsctp_close(sock);
    } else {
        write(fileno(stdout), data, datalen);
        free(data);
    }
    return 1;
}

int main(int argc, char** argv)
{
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);
  boost::filesystem::path app = boost::filesystem::system_complete(argv[0]);
  std::string logfile = app.filename().string();

#ifdef SCTP_TEST
  int local_encaps_port = 5005;
  int remote_encaps_port = 5004;
  int remote_sctp_port = 5000;
  const char* remote_address = "130.149.228.80";

#define USE_KERNEL_SCTP_VERSION
#ifdef USE_KERNEL_SCTP_VERSION
  usrsctp_init(local_encaps_port, NULL, debug_printf);
#else
  usrsctp_init(local_encaps_port);
#endif
  struct socket *sock;
  struct sockaddr_in addr4;
  struct sockaddr_in6 addr6;
  struct sctp_udpencaps encaps;
  //char buffer[80];

  usrsctp_sysctl_set_sctp_debug_on(0);

#if 1

  usrsctp_sysctl_set_sctp_blackhole(2);

  if ((sock = usrsctp_socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP, receive_cb, NULL, 0, NULL)) == NULL) {
      perror("userspace_socket ipv6");
  }
//  if ((sock = usrsctp_socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP, receive_cb, NULL, 0, &peer_connection)) == NULL) {
//      perror("socket");
//  }
  memset(&encaps, 0, sizeof(struct sctp_udpencaps));
  encaps.sue_address.ss_family = AF_INET6;
  encaps.sue_port = htons(remote_encaps_port);
  if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_REMOTE_UDP_ENCAPS_PORT, (const void*)&encaps, (socklen_t)sizeof(struct sctp_udpencaps)) < 0) {
      perror("setsockopt");
  }
  memset((void *)&addr4, 0, sizeof(struct sockaddr_in));
  memset((void *)&addr6, 0, sizeof(struct sockaddr_in6));
#if !defined(__Userspace_os_Linux) && !defined(__Userspace_os_Windows)
  addr4.sin_len = sizeof(struct sockaddr_in);
  addr6.sin6_len = sizeof(struct sockaddr_in6);
#endif
  addr4.sin_family = AF_INET;
  addr6.sin6_family = AF_INET6;
  addr4.sin_port = htons(remote_sctp_port);
  addr6.sin6_port = htons(remote_sctp_port);

  if (inet_pton(AF_INET6, remote_address, &addr6.sin6_addr) == 1) {
      if (usrsctp_connect(sock, (struct sockaddr *)&addr6, sizeof(struct sockaddr_in6)) < 0) {
          perror("userspace_connect");
      }
  } else if (inet_pton(AF_INET, remote_address, &addr4.sin_addr) == 1) {
      if (usrsctp_connect(sock, (struct sockaddr *)&addr4, sizeof(struct sockaddr_in)) < 0) {
          perror("userspace_connect");
      }
  } else {
      printf("Illegal destination address.\n");
  }

  char buffer[256];
  memset(buffer, 'A', 255);
  buffer[255] = '\0';
//  while ((fgets(buffer, sizeof(buffer), stdin) != NULL) && !done) {
  for (int i =0; i < 5; ++i){
      usrsctp_sendv(sock, buffer, strlen(buffer), NULL, 0,
                                NULL, 0, SCTP_SENDV_NOINFO, 0);
#if defined (__Userspace_os_Windows)
      Sleep(1*1000);
#else
      sleep(1);
#endif
  }
  if (!done) {
      usrsctp_shutdown(sock, SHUT_WR);
  }
  while (!done) {
#if defined (__Userspace_os_Windows)
      Sleep(1*1000);
#else
      sleep(1);
#endif
  }
  while (usrsctp_finish() != 0) {
#if defined (__Userspace_os_Windows)
      Sleep(1000);
#else
      sleep(1);
#endif
  }
#endif
#endif

  return 0;

  std::ostringstream ostr;
  for (int i = 0; i < argc; ++i)
      ostr << argv[i] << " ";

  LOG(INFO) << ostr.str();

  try
  {
    // SPD for remote media session
    std::string sSdp;

    // check program configuration  
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
      ("help", "produce help message")
      ("sdp", po::value<std::string>(&sSdp), "SDP file")
      ;

    po::positional_options_description p;
    p.add("sdp", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
        options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) 
    {
      ostringstream ostr;
      ostr << desc;
      LOG(WARNING) << ostr.str();
      return 1;
    }

    boost::optional<rfc4566::SessionDescription> sdp = app::ApplicationUtil::readSdp(sSdp);
    return (sdp ? 0 : 1);
  }
  catch (boost::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << boost::diagnostic_information(e);
  }
  catch (std::exception& e)
  {
    LOG(ERROR) << "Exception: %1%" << e.what();
  }
  catch (...)
  {
    LOG(ERROR) << "Unknown exception!!!";
  }
  return 0;
}

