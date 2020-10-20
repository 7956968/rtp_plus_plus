#include "CorePch.h"
#include <rtp++/network/NetworkInterfaceUtil.h>
#include <boost/algorithm/string.hpp>

// for interface and mask discovery
#ifdef _WIN32
#include <winsock2.h>
// #include <Ipifcons.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#elif __APPLE__
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#endif

// #define DEBUG_NETWORK_INTERFACES

namespace rtp_plus_plus
{

std::vector<NetworkInterface> NetworkInterfaceUtil::getLocalInterfaces(bool bRemoveZeroAddresses, bool bFilterOutLinkLocal)
{
  std::vector<NetworkInterface> vLocalInterfaces;
#ifdef _WIN32
  IP_ADAPTER_INFO  *pAdapterInfo;
  ULONG            ulOutBufLen;
  DWORD            dwRetVal;

  pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
  ulOutBufLen = sizeof(IP_ADAPTER_INFO);

  if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS) {
    free(pAdapterInfo);
    pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
  }

  if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) != ERROR_SUCCESS) {
    printf("GetAdaptersInfo call failed with %d\n", dwRetVal);
  }

  PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
  while (pAdapter) {
#ifdef DEBUG_NETWORK_INTERFACES
    printf("Adapter Name: %s\n", pAdapter->AdapterName);
    printf("Adapter Desc: %s\n", pAdapter->Description);
    printf("\tAdapter Addr: \t");
    for (UINT i = 0; i < pAdapter->AddressLength; i++) {
      if (i == (pAdapter->AddressLength - 1))
        printf("%.2X\n", (int)pAdapter->Address[i]);
      else
        printf("%.2X-", (int)pAdapter->Address[i]);
    }
#endif

    NetworkInterface localInterface;
    localInterface.Address = std::string(pAdapter->IpAddressList.IpAddress.String);
    localInterface.Mask = std::string(pAdapter->IpAddressList.IpMask.String);

    if (bRemoveZeroAddresses && localInterface.Address == "0.0.0.0")
    {
      pAdapter = pAdapter->Next;
      continue;
    }

    if (bFilterOutLinkLocal && boost::algorithm::starts_with(localInterface.Address, "169.254"))
    {
      pAdapter = pAdapter->Next;
      continue;
    }

    vLocalInterfaces.push_back(localInterface);
    
#ifdef DEBUG_NETWORK_INTERFACES
    printf("IP Address: %s\n", pAdapter->IpAddressList.IpAddress.String);
    printf("IP Mask: %s\n", pAdapter->IpAddressList.IpMask.String);
    printf("\tGateway: \t%s\n", pAdapter->GatewayList.IpAddress.String);
    printf("\t***\n");
    if (pAdapter->DhcpEnabled) {
      printf("\tDHCP Enabled: Yes\n");
      printf("\t\tDHCP Server: \t%s\n", pAdapter->DhcpServer.IpAddress.String);
    }
    else
      printf("\tDHCP Enabled: No\n");
#endif
    pAdapter = pAdapter->Next;
  }
#elif __APPLE__

  NetworkInterface localInterface;
  localInterface.Address = "10.0.0.7";
  localInterface.Mask = "255.255.255.0";
  vLocalInterfaces.push_back(localInterface); 

#else
#if 1
  // TODO: add strings to vector
  struct ifaddrs *interfaceArray = NULL, *tempIfAddr = NULL;
  void *tempAddrPtr = NULL;
  int rc = 0;
  char addressOutputBuffer[INET6_ADDRSTRLEN];

  rc = getifaddrs(&interfaceArray);  /* retrieve the current interfaces */
  if (rc == 0)
  {
    for (tempIfAddr = interfaceArray; tempIfAddr != NULL; tempIfAddr = tempIfAddr->ifa_next)
    {
      if (tempIfAddr->ifa_addr->sa_family == AF_INET)
        tempAddrPtr = &((struct sockaddr_in *)tempIfAddr->ifa_addr)->sin_addr;
      else
        tempAddrPtr = &((struct sockaddr_in6 *)tempIfAddr->ifa_addr)->sin6_addr;

      const char* szAddress = inet_ntop(tempIfAddr->ifa_addr->sa_family,
        tempAddrPtr,
        addressOutputBuffer,
        sizeof(addressOutputBuffer));

      // ignore null addresses
      if (szAddress == NULL)
        continue;

      // ignore loopback, non-running
      if ((strcmp("lo", tempIfAddr->ifa_name) == 0) ||
        !(tempIfAddr->ifa_flags & (IFF_RUNNING)))
        continue;

      // ignore non-IPv4
      if (tempIfAddr->ifa_addr->sa_family != AF_INET)
        continue;

      NetworkInterface localInterface;
      localInterface.Address = std::string(szAddress);
      localInterface.Mask = std::string(inet_ntop(tempIfAddr->ifa_netmask->sa_family,
        tempAddrPtr,
        addressOutputBuffer,
        sizeof(addressOutputBuffer)));

      if (bFilterOutLinkLocal)
      {
        if (!boost::algorithm::starts_with(localInterface.Address, "169.254"))
          vLocalInterfaces.push_back(localInterface);
      }
      else
      {
        vLocalInterfaces.push_back(localInterface);
      }

#ifdef DEBUG_NETWORK_INTERFACES
      printf("Internet Address:  %s \n",
        inet_ntop(tempIfAddr->ifa_addr->sa_family,
        tempAddrPtr,
        addressOutputBuffer,
        sizeof(addressOutputBuffer)));
#endif
        inet_ntop(tempIfAddr->ifa_addr->sa_family,
        tempAddrPtr,
        addressOutputBuffer,
        sizeof(addressOutputBuffer));

#ifdef DEBUG_NETWORK_INTERFACES
      printf("LineDescription :  %s \n", tempIfAddr->ifa_name);
#endif
      if (tempIfAddr->ifa_netmask != NULL)
      {
        if (tempIfAddr->ifa_netmask->sa_family == AF_INET)
          tempAddrPtr = &((struct sockaddr_in *)tempIfAddr->ifa_netmask)->sin_addr;
        else
          tempAddrPtr = &((struct sockaddr_in6 *)tempIfAddr->ifa_netmask)->sin6_addr;

#ifdef DEBUG_NETWORK_INTERFACES
        printf("Netmask         :  %s \n",
          inet_ntop(tempIfAddr->ifa_netmask->sa_family,
          tempAddrPtr,
          addressOutputBuffer,
          sizeof(addressOutputBuffer)));
#endif
          inet_ntop(tempIfAddr->ifa_netmask->sa_family,
          tempAddrPtr,
          addressOutputBuffer,
          sizeof(addressOutputBuffer));
      }
      if (tempIfAddr->ifa_ifu.ifu_broadaddr != NULL)
      {
        /* If the ifa_flags field indicates that this is a P2P interface */
        if (tempIfAddr->ifa_flags & IFF_POINTOPOINT)
        {
#ifdef DEBUG_NETWORK_INTERFACES
          printf("Destination Addr:  ");
#endif
          if (tempIfAddr->ifa_ifu.ifu_dstaddr->sa_family == AF_INET)
            tempAddrPtr = &((struct sockaddr_in *)tempIfAddr->ifa_ifu.ifu_dstaddr)->sin_addr;
          else
            tempAddrPtr = &((struct sockaddr_in6 *)tempIfAddr->ifa_ifu.ifu_dstaddr)->sin6_addr;
        }
        else
        {
#ifdef DEBUG_NETWORK_INTERFACES
          printf("Broadcast Addr  :  ");
#endif
          if (tempIfAddr->ifa_ifu.ifu_broadaddr->sa_family == AF_INET)
            tempAddrPtr = &((struct sockaddr_in *)tempIfAddr->ifa_ifu.ifu_broadaddr)->sin_addr;
          else
            tempAddrPtr = &((struct sockaddr_in6 *)tempIfAddr->ifa_ifu.ifu_broadaddr)->sin6_addr;
        }

#ifdef DEBUG_NETWORK_INTERFACES
        printf("%s \n",
          inet_ntop(tempIfAddr->ifa_ifu.ifu_broadaddr->sa_family,
          tempAddrPtr,
          addressOutputBuffer,
          sizeof(addressOutputBuffer)));
#endif
      }
#ifdef DEBUG_NETWORK_INTERFACES
      printf("\n");
#endif
    }

    freeifaddrs(interfaceArray);             /* free the dynamic memory */
    interfaceArray = NULL;                   /* prevent use after free  */
  }
#else

  NetworkInterface localInterface;
  localInterface.Address = std::string("146.64.19.0");
  localInterface.Mask = std::string("255.255.255.0");
  vLocalInterfaces.push_back(localInterface);
#endif
#endif

  return vLocalInterfaces;
}

} // rtp_plus_plus
