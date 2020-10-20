#pragma once
#include <sstream>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <cpputil/Conversion.h>
#include <cpputil/StringTokenizer.h>
#include <rtp++/network/AddressDescriptor.h>

namespace rtp_plus_plus
{

/**
 * @brief This class is used to parse our custom address descriptor config file
 *
 * Addresses are stored in the following format and are context dependent:
 * bind_ip:bind_port:ip:port
 *
 * TODO: error checking using regular expressions
 *
 * IPv4:
 * ^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])
 *
 * Colon:
 * \:
 *
 * Unsigned short:
 * (0|(\+)?([1-9]{1}[0-9]{0,3})|([1-5]{1}[0-9]{1,4}|[6]{1}([0-4]{1}[0-9]{3}|[5]{1}([0-4]{1}[0-9]{2}|[5]{1}([0-2]{1}[0-9]{1}|[3]{1}[0-5]{1})))))$
 *
 * Hostname:
 * ^(([a-zA-Z]|[a-zA-Z][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z]|[A-Za-z][A-Za-z0-9\-]*[A-Za-z0-9])$
*/
class AddressDescriptorParser
{
public:
  /**
   * @brief parseInterfaceDescription parses interface descriptions passed in on the command line.
   * The description must have the following format 127.0.0.1:5004:5005. Multiple descriptions can
   * be separated by a comma (boost::options fails to pass through when using a semicolon.
   *
   * @param sInterfaceDescription
   * @param vInterfaces
   * @return
   */
  static boost::optional<InterfaceDescriptions_t> parseInterfaceDescriptionShort(const std::string& sInterfaceDescription)
  {
    InterfaceDescriptions_t vInterfaces;
    std::vector<std::string> vTempInterfaces = StringTokenizer::tokenize(sInterfaceDescription, ",", true, true);
    for (const std::string& sInterfaceDesc: vTempInterfaces )
    {
      // the interface is of the form address:rtp_port:rtcp_port
      std::vector<std::string> interfaceDesc = StringTokenizer::tokenize(sInterfaceDesc, ":", true, true);
      if (interfaceDesc.size() != 3)
      {
        return boost::optional<InterfaceDescriptions_t>();
      }
      else
      {
        bool bRes = false;
        std::string sAddress = interfaceDesc[0];
        uint16_t uiRtpPort = convert<uint16_t>(interfaceDesc[1], bRes);
        if (!bRes)
          return boost::optional<InterfaceDescriptions_t>();
        uint16_t uiRtcpPort = convert<uint16_t>(interfaceDesc[2], bRes);
        if (!bRes) boost::optional<InterfaceDescriptions_t>();
        vInterfaces.push_back(MediaInterfaceDescriptor_t(1, std::make_tuple(AddressDescriptor(sAddress, uiRtpPort), AddressDescriptor(sAddress, uiRtcpPort), "")));
      }
    }
    return boost::optional<InterfaceDescriptions_t>(vInterfaces);
  }
  /**
   * @brief parse
   * @param sAddressDecriptor
   * @return
   */
  static boost::optional<InterfaceDescriptions_t> parse(const std::string& sAddressDecriptor, bool bRtcpMux = false)
  {
    if (sAddressDecriptor.empty()) return boost::optional<InterfaceDescriptions_t>();
    // preprocess string and remove lines starting with a comment
    std::istringstream istr(sAddressDecriptor);
    std::ostringstream ostr;

    while (!istr.eof())
    {
      std::string sLine;
      getline(istr, sLine);
      if (sLine[0] != '#')
        ostr << sLine << std::endl;
    }
    std::string sProcessed = ostr.str();

    InterfaceDescriptions_t vInterfaceDescriptions;

    // find first media descriptor
    size_t start = sProcessed.find_first_of("{");
    while (start != std::string::npos)
    {
      size_t end = sProcessed.find_first_of("}", start);
      // check for error
      if (end == std::string::npos)
        return boost::optional<InterfaceDescriptions_t>();

      MediaInterfaceDescriptor_t vMediaDescriptors;
      std::string addresses = sProcessed.substr(start + 1, end - start - 1);

      std::vector<std::string> media_interfaces = StringTokenizer::tokenize(addresses, "\n;", true);

      BOOST_FOREACH(std::string mediaInterface, media_interfaces)
      {
        std::vector<std::string> descriptors = StringTokenizer::tokenize(mediaInterface, ",", true);
        if (descriptors.size() < 2)
          return boost::optional<InterfaceDescriptions_t>();

        AddressDescriptor interface1, interface2;
        if (!parse(descriptors[0], interface1))
          return boost::optional<InterfaceDescriptions_t>();

        if (descriptors[1] != "")
        {
          if (!parse(descriptors[1], interface2))
            return boost::optional<InterfaceDescriptions_t>();
        }
        else
        {
          VLOG(12) << "No RTCP specified, using default";
          // if empty, default RTCP port
          interface2 = interface1;
          uint16_t uiRtcp = interface2.getPort();
          if (!bRtcpMux)
           ++uiRtcp;
          interface2.setPort(uiRtcp);
        }

        std::string sId;
        if (descriptors.size() == 3)
          sId = descriptors[2];
        vMediaDescriptors.push_back(std::make_tuple(interface1, interface2, sId));
      }

      vInterfaceDescriptions.push_back(vMediaDescriptors);

      start = sProcessed.find("{", end);
    }

    return boost::optional<InterfaceDescriptions_t>(vInterfaceDescriptions);
  }
#if 0
    // get line
    std::istringstream buffer(sAddressDecriptor);
    std::string line;
    AddressDescriptorList_t addresses;

    while (std::getline(buffer, line))
    {
      std::vector<std::string> vTokens = StringTokenizer::tokenize(line, ":", true);
      if (vTokens.size() != 4)
        return boost::optional<AddressDescriptorList_t>();

      addresses.push_back(AddressDescriptor(vTokens[0], convert<uint16_t>(vTokens[1]), vTokens[2], convert<uint16_t>(vTokens[3])));
    }

    return boost::optional<AddressDescriptorList_t>(addresses);
  }
#endif

private:

  static bool parse(const std::string& sDescriptor, AddressDescriptor& addressDescriptor )
  {
    // FIXME: this WON'T work for IPv6
    std::vector<std::string> vTokens = StringTokenizer::tokenize(sDescriptor, ":", true);
    // validate syntax
    if (vTokens.size() != 4)
      return false;

    // FIXME: TODO: bug: if the port is an empty string, the conversion function yields the wrong value
    addressDescriptor = AddressDescriptor(vTokens[0], convert<uint16_t>(vTokens[1], 0), vTokens[2], convert<uint16_t>(vTokens[3], 0));
    return true;
  }
};

} // rtp_plus_plus
