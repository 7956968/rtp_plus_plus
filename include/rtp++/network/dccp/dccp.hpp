//
// ip/dccp.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IP_DCCP_HPP
#define BOOST_ASIO_IP_DCCP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/basic_socket_iostream.hpp>
#include <boost/asio/basic_stream_socket.hpp>
// RG
// #include <boost/asio/basic_packetstream_socket.hpp>
#include "packetstream_socket_service.hpp"

#include <boost/asio/detail/socket_option.hpp>
#include <boost/asio/detail/socket_types.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/basic_resolver.hpp>
#include <boost/asio/ip/basic_resolver_iterator.hpp>
#include <boost/asio/ip/basic_resolver_query.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ip {

/// Encapsulates the flags needed for DCCP.
/**
 * The boost::asio::ip::dccp class contains flags necessary for DCCP sockets.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Safe.
 *
 * @par Concepts:
 * Protocol, InternetProtocol.
 */
class dccp
{
public:
  /// The type of a DCCP endpoint.
  typedef basic_endpoint<dccp> endpoint;

  /// Construct to represent the IPv4 DCCP protocol.
  static dccp v4()
  {
    return dccp(PF_INET);
  }

  /// Construct to represent the IPv6 DCCP protocol.
  static dccp v6()
  {
    return dccp(PF_INET6);
  }

  /// Obtain an identifier for the type of the protocol.
  int type() const
  {
    return SOCK_DCCP;
  }

  /// Obtain an identifier for the protocol.
  int protocol() const
  {
    return IPPROTO_DCCP;
  }

  /// Obtain an identifier for the protocol family.
  int family() const
  {
    return family_;
  }

  // RG: TODO?
  /// The DCCP socket type.
  //typedef basic_stream_socket<dccp> socket;

#if 0
  typedef basic_stream_socket<dccp, packetstream_socket_service<dccp> > socket;
#else
  typedef basic_datagram_socket<dccp, packetstream_socket_service<dccp> > socket;
#endif

//typedef basic_packetstream_socket<dccp> socket;

  //typedef basic_seq_packet_socket<dccp> socket;
  /// The UDP socket type.
  //typedef basic_datagram_socket<dccp> socket;

  /// The DCCP acceptor type.
  typedef basic_socket_acceptor<dccp> acceptor;

  /// The DCCP resolver type.
  typedef basic_resolver<dccp> resolver;

#if !defined(BOOST_NO_IOSTREAM)
  /// The DCCP iostream type.
  typedef basic_socket_iostream<dccp> iostream;
#endif // !defined(BOOST_NO_IOSTREAM)

  /// Socket option for disabling the Nagle algorithm.
  /**
   * Implements the IPPROTO_TCP/TCP_NODELAY socket option.
   *
   * @par Examples
   * Setting the option:
   * @code
   * boost::asio::ip::tcp::socket socket(io_service); 
   * ...
   * boost::asio::ip::tcp::no_delay option(true);
   * socket.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * boost::asio::ip::tcp::socket socket(io_service); 
   * ...
   * boost::asio::ip::tcp::no_delay option;
   * socket.get_option(option);
   * bool is_set = option.value();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Boolean_Socket_Option.
   */
/* RG
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined no_delay;
#else
  typedef boost::asio::detail::socket_option::boolean<
    IPPROTO_TCP, TCP_NODELAY> no_delay;
#endif
*/

  /// Compare two protocols for equality.
  friend bool operator==(const dccp& p1, const dccp& p2)
  {
    return p1.family_ == p2.family_;
  }

  /// Compare two protocols for inequality.
  friend bool operator!=(const dccp& p1, const dccp& p2)
  {
    return p1.family_ != p2.family_;
  }

private:
  // Construct with a specific family.
  explicit dccp(int protocol_family)
    : family_(protocol_family)
  {
  }

  int family_;
};

} // namespace ip
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IP_DCCP_HPP
