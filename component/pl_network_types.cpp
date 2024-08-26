#include "pl_network_types.h"
#include "lwip/sockets.h"

//==============================================================================

namespace PL {

//==============================================================================

IpV4Address::IpV4Address() : u32(0) {}

//==============================================================================

IpV4Address::IpV4Address(uint8_t u8_0, uint8_t u8_1, uint8_t u8_2, uint8_t u8_3) {
  u8[0] = u8_0;
  u8[1] = u8_1;
  u8[2] = u8_2;
  u8[3] = u8_3;
}

//==============================================================================

IpV4Address::IpV4Address(uint16_t u16_0, uint16_t u16_1) {
  u16[0] = u16_0;
  u16[1] = u16_1;
}

//==============================================================================

IpV4Address::IpV4Address(uint32_t u32) : u32(u32) {}

//==============================================================================

IpV4Address::IpV4Address(const std::string& address) {
  u32 = 0;
  inet_pton(AF_INET, address.c_str(), &u32);
}

//==============================================================================


std::string IpV4Address::ToString() const {
  char addressString[16];
  sprintf(addressString, "%d.%d.%d.%d", u8[0], u8[1], u8[2], u8[3]);
  return addressString;
}

//==============================================================================

IpV6Address::IpV6Address() {
  u32[0] = u32[1] = u32[2] = u32[3] = zoneId = 0;
}

//==============================================================================

IpV6Address::IpV6Address(uint8_t u8_0, uint8_t u8_1, uint8_t u8_2, uint8_t u8_3, uint8_t u8_4, uint8_t u8_5, uint8_t u8_6, uint8_t u8_7,
               uint8_t u8_8, uint8_t u8_9, uint8_t u8_10, uint8_t u8_11, uint8_t u8_12, uint8_t u8_13, uint8_t u8_14, uint8_t u8_15, uint8_t zoneId) {
  u8[0] = u8_0;
  u8[1] = u8_1;
  u8[2] = u8_2;
  u8[3] = u8_3;
  u8[4] = u8_4;
  u8[5] = u8_5;
  u8[6] = u8_6;
  u8[7] = u8_7;
  u8[8] = u8_8;
  u8[9] = u8_9;
  u8[10] = u8_10;
  u8[11] = u8_11;
  u8[12] = u8_12;
  u8[13] = u8_13;
  u8[14] = u8_14;
  u8[15] = u8_15;
  this->zoneId = zoneId;
}

//==============================================================================

IpV6Address::IpV6Address(uint16_t u16_0, uint16_t u16_1, uint16_t u16_2, uint16_t u16_3, uint16_t u16_4, uint16_t u16_5, uint16_t u16_6, uint16_t u16_7, uint8_t zoneId) {
  u16[0] = u16_0;
  u16[1] = u16_1;
  u16[2] = u16_2;
  u16[3] = u16_3;
  u16[4] = u16_4;
  u16[5] = u16_5;
  u16[6] = u16_6;
  u16[7] = u16_7;  
  this->zoneId = zoneId;
}

//==============================================================================

IpV6Address::IpV6Address(uint32_t u32_0, uint32_t u32_1, uint32_t u32_2, uint32_t u32_3, uint8_t zoneId) {
  u32[0] = u32_0;
  u32[1] = u32_1;
  u32[2] = u32_2;
  u32[3] = u32_3;
  this->zoneId = zoneId;
}

//==============================================================================

IpV6Address::IpV6Address(const std::string& address) {
  u32[0] = u32[1] = u32[2] = u32[3] = zoneId = 0;
  inet_pton(AF_INET6, address.c_str(), &u32);
}

//==============================================================================

std::string IpV6Address::ToString() const {
  char addressString[44];
  sprintf(addressString, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x%%%d",
    __builtin_bswap16(u16[0]), __builtin_bswap16(u16[1]), __builtin_bswap16(u16[2]), __builtin_bswap16(u16[3]),
    __builtin_bswap16(u16[4]), __builtin_bswap16(u16[5]), __builtin_bswap16(u16[6]), __builtin_bswap16(u16[7]), zoneId);
  return addressString;
}

//==============================================================================

NetworkAddress::NetworkAddress() : family(NetworkAddressFamily::unknown) {}

//==============================================================================

NetworkAddress::NetworkAddress(IpV4Address address) : family(NetworkAddressFamily::ipV4), ipV4(address) {}

//==============================================================================

NetworkAddress::NetworkAddress(IpV6Address address) : family(NetworkAddressFamily::ipV6), ipV6(address) {}

//==============================================================================

std::string NetworkAddress::ToString() const {
  if (family == PL::NetworkAddressFamily::ipV4)
    return ipV4.ToString();
  if (family == PL::NetworkAddressFamily::ipV6)
    return ipV6.ToString();
  return "";
}

//==============================================================================

NetworkEndpoint::NetworkEndpoint() : address(), port(0) {}

//==============================================================================

NetworkEndpoint::NetworkEndpoint(IpV4Address address, uint16_t port) : address(address), port(port) {}

//==============================================================================

NetworkEndpoint::NetworkEndpoint(IpV6Address address, uint16_t port) : address(address), port(port) {}

//==============================================================================

}
