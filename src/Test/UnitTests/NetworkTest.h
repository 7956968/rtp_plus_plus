#pragma once
#include <boost/test/unit_test.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <rtp++/network/AddressDescriptor.h>
#include <rtp++/network/AddressDescriptorParser.h>
#include <rtp++/network/PortAllocationManager.h>

namespace rtp_plus_plus
{
namespace test
{

BOOST_AUTO_TEST_SUITE(NetworkTest)
BOOST_AUTO_TEST_CASE(test_parseAddressDescriptors_single_line)
{
  std::string sDescription =  "{ ::10.0.0.4:49170, }";
  auto vAddresses = AddressDescriptorParser::parse(sDescription);
  BOOST_CHECK_EQUAL(1, vAddresses->size());
  BOOST_CHECK_EQUAL(1, (*vAddresses)[0].size());
  AddressDescriptor a1 = std::get<0>((*vAddresses)[0].at(0));
  BOOST_CHECK_EQUAL("10.0.0.4", a1.getAddress());
  BOOST_CHECK_EQUAL(49170, a1.getPort());
}

BOOST_AUTO_TEST_CASE(test_parseAddressDescriptors_single_line1)
{
  std::string sDescription =  "{ ::146.64.28.171:5072, ::146.64.28.171:5073; ::130.149.228.93:5004, ::130.149.228.93:5005}";
  auto vAddresses = AddressDescriptorParser::parse(sDescription);
  BOOST_CHECK_EQUAL(1, vAddresses->size());
  BOOST_CHECK_EQUAL(2, (*vAddresses)[0].size());
  AddressDescriptor a1 = std::get<0>((*vAddresses)[0].at(0));
  AddressDescriptor a2 = std::get<1>((*vAddresses)[0].at(0));
  BOOST_CHECK_EQUAL("146.64.28.171", a1.getAddress());
  BOOST_CHECK_EQUAL(5072, a1.getPort());
  BOOST_CHECK_EQUAL("146.64.28.171", a2.getAddress());
  BOOST_CHECK_EQUAL(5073, a2.getPort());
  AddressDescriptor a3 = std::get<0>((*vAddresses)[0].at(1));
  AddressDescriptor a4 = std::get<1>((*vAddresses)[0].at(1));
  BOOST_CHECK_EQUAL("130.149.228.93", a3.getAddress());
  BOOST_CHECK_EQUAL(5004, a3.getPort());
  BOOST_CHECK_EQUAL("130.149.228.93", a4.getAddress());
  BOOST_CHECK_EQUAL(5005, a4.getPort());
}

BOOST_AUTO_TEST_CASE(test_parseAddressDescriptors_single_line2)
{
  std::string sDescription =  "{ ::146.64.28.171:5072, ; ::130.149.228.93:5004, }";
  auto vAddresses = AddressDescriptorParser::parse(sDescription);
  BOOST_CHECK_EQUAL(1, vAddresses->size());
  BOOST_CHECK_EQUAL(2, (*vAddresses)[0].size());
  AddressDescriptor a1 = std::get<0>((*vAddresses)[0].at(0));
  AddressDescriptor a2 = std::get<1>((*vAddresses)[0].at(0));
  BOOST_CHECK_EQUAL("146.64.28.171", a1.getAddress());
  BOOST_CHECK_EQUAL(5072, a1.getPort());
  BOOST_CHECK_EQUAL("146.64.28.171", a2.getAddress());
  BOOST_CHECK_EQUAL(5073, a2.getPort());
  AddressDescriptor a3 = std::get<0>((*vAddresses)[0].at(1));
  AddressDescriptor a4 = std::get<1>((*vAddresses)[0].at(1));
  BOOST_CHECK_EQUAL("130.149.228.93", a3.getAddress());
  BOOST_CHECK_EQUAL(5004, a3.getPort());
  BOOST_CHECK_EQUAL("130.149.228.93", a4.getAddress());
  BOOST_CHECK_EQUAL(5005, a4.getPort());
}

BOOST_AUTO_TEST_CASE(test_parseAddressDescriptors_single_line3)
{
  std::string sDescription =  "{ ::146.64.28.171:5072, ::146.64.28.171:5073; ::146.64.28.171:5074, ::146.64.28.171:5075},{::130.149.228.93:5004, ::130.149.228.93:5005; ::130.149.228.93:5006, ::130.149.228.93:5007}";
  auto vAddresses = AddressDescriptorParser::parse(sDescription);
  BOOST_CHECK_EQUAL(2, vAddresses->size());
  BOOST_CHECK_EQUAL(2, (*vAddresses)[0].size());
  AddressDescriptor a1 = std::get<0>((*vAddresses)[0].at(0));
  AddressDescriptor a2 = std::get<1>((*vAddresses)[0].at(0));
  BOOST_CHECK_EQUAL("146.64.28.171", a1.getAddress());
  BOOST_CHECK_EQUAL(5072, a1.getPort());
  BOOST_CHECK_EQUAL("146.64.28.171", a2.getAddress());
  BOOST_CHECK_EQUAL(5073, a2.getPort());
  AddressDescriptor a3 = std::get<0>((*vAddresses)[0].at(1));
  AddressDescriptor a4 = std::get<1>((*vAddresses)[0].at(1));
  BOOST_CHECK_EQUAL("146.64.28.171", a3.getAddress());
  BOOST_CHECK_EQUAL(5074, a3.getPort());
  BOOST_CHECK_EQUAL("146.64.28.171", a4.getAddress());
  BOOST_CHECK_EQUAL(5075, a4.getPort());

  a1 = std::get<0>((*vAddresses)[1].at(0));
  a2 = std::get<1>((*vAddresses)[1].at(0));
  BOOST_CHECK_EQUAL("130.149.228.93", a1.getAddress());
  BOOST_CHECK_EQUAL(5004, a1.getPort());
  BOOST_CHECK_EQUAL("130.149.228.93", a2.getAddress());
  BOOST_CHECK_EQUAL(5005, a2.getPort());
  a3 = std::get<0>((*vAddresses)[1].at(1));
  a4 = std::get<1>((*vAddresses)[1].at(1));
  BOOST_CHECK_EQUAL("130.149.228.93", a3.getAddress());
  BOOST_CHECK_EQUAL(5006, a3.getPort());
  BOOST_CHECK_EQUAL("130.149.228.93", a4.getAddress());
  BOOST_CHECK_EQUAL(5007, a4.getPort());
}

BOOST_AUTO_TEST_CASE(test_parseAddressDescriptors)
{
  std::string sDescription =  "{"\
      "127.0.0.1:54:146.64.28.171:5072,::130.149.228.93:5004"\
      "}";

  auto vAddresses = AddressDescriptorParser::parse(sDescription);

  BOOST_CHECK_EQUAL(1, vAddresses->size());
}

BOOST_AUTO_TEST_CASE(test_parseAddressDescriptorsMultipath)
{
  std::string sDescription =  "{\n"\
      "::146.64.28.171:5072, ::146.64.28.171:5073\n"\
      "::130.149.228.93:5004, ::130.149.228.93:5005\n"\
      "}\n";

  auto vAddresses = AddressDescriptorParser::parse(sDescription);

  BOOST_CHECK_EQUAL(1, vAddresses->size());
}

BOOST_AUTO_TEST_CASE(test_parseAddressDescriptorsMultisession)
{
  std::string sDescription =  "{\n"\
      "::146.64.28.171:5072, ::146.64.28.171:5073\n"\
      "::130.149.228.93:5004, ::130.149.228.93:5005\n"\
      "}\n{"\
      "::146.64.28.171:5074, ::146.64.28.171:5075\n"\
      "::130.149.228.93:5006, ::130.149.228.93:5007\n"\
      "}\n";

  auto vAddresses = AddressDescriptorParser::parse(sDescription);

  BOOST_CHECK_EQUAL(2, vAddresses->size());
}

/**
* @brief Method to test port allocation.
*
* This method might fail if the used ports are already bound!
*/
BOOST_AUTO_TEST_CASE(test_portAllocationManager)
{
  boost::asio::io_service ioService;
  PortAllocationManager portAllocManager(ioService);

  const std::string sAddress("0.0.0.0");
  uint16_t uiPort = 5004;
  bool bMandatory = true;

  boost::system::error_code ec = portAllocManager.allocateUdpPort(sAddress, uiPort, bMandatory);
  BOOST_CHECK_EQUAL(ec == boost::system::error_code(), true);

  std::unique_ptr<boost::asio::ip::udp::socket> pSocket = portAllocManager.getBoundUdpSocket(sAddress, uiPort);
  BOOST_CHECK_EQUAL(pSocket != 0, true);

  // try to get the same port
  ec = portAllocManager.allocateUdpPort(sAddress, uiPort, bMandatory);
  BOOST_CHECK_EQUAL(ec == boost::asio::error::address_in_use, true);

  // change mandatory requirements - allocation should now succeed
  bMandatory = false;
  ec = portAllocManager.allocateUdpPort(sAddress, uiPort, bMandatory);
  BOOST_CHECK_EQUAL(ec == boost::system::error_code(), true);
  BOOST_CHECK_EQUAL(uiPort, 5005);

  // RTP and RTCP tests
  bMandatory = true;
  uiPort = 5004;
  // should fail since RTP and RTCP port is already allocated
  ec = portAllocManager.allocateUdpPortsForRtpRtcp(sAddress, uiPort, bMandatory);
  BOOST_CHECK_EQUAL(ec == boost::asio::error::address_in_use, true);

  // test failure in case where RTCP port is already allocated
  uint16_t uiRtpPort = 5008;
  uint16_t uiRtcpPort = 5009;
  ec = portAllocManager.allocateUdpPort(sAddress, uiRtcpPort, bMandatory);
  if (!ec)
  {
    // This should fail since the RTCP port is already allocated
    ec = portAllocManager.allocateUdpPortsForRtpRtcp(sAddress, uiRtpPort, bMandatory);
    BOOST_CHECK_EQUAL(ec == boost::asio::error::address_in_use, true);
    // now change mandatory-ness
    bool bMandatory = false;
    ec = portAllocManager.allocateUdpPortsForRtpRtcp(sAddress, uiRtpPort, bMandatory);
    // now 5009 and 5011 should be allocated
    BOOST_CHECK_EQUAL(ec == boost::system::error_code(), true);
    VLOG(2) << "Allocated " << uiRtpPort;
  }
  else
  {
    LOG(WARNING) << "Failed to allocate RTCP port: skipping test";
  }

  uint16_t uiOddRtpPort = 5007;
  ec = portAllocManager.allocateUdpPortsForRtpRtcp(sAddress, uiOddRtpPort, bMandatory);
  BOOST_CHECK_EQUAL(ec == boost::asio::error::invalid_argument, true);

  // test standard alloc
  uiRtpPort = 5006;
  ec = portAllocManager.allocateUdpPortsForRtpRtcp(sAddress, uiRtpPort, bMandatory);
  BOOST_CHECK_EQUAL(ec == boost::system::error_code(), true);
  BOOST_CHECK_EQUAL(uiRtpPort, 5006);

}

BOOST_AUTO_TEST_SUITE_END()

} // test
} // rtp_plus_plus
