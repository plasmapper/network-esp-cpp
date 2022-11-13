#pragma once
#include "stdint.h"
#include <string>

//==============================================================================

namespace PL {

//==============================================================================

/// @brief Network address family
enum class NetworkAddressFamily {
  /// @brief unknown address family
  unknown,
  /// @brief IPv4 address family
  ipV4,
  /// @brief IPv6 address family
  ipV6
};

//==============================================================================

/// @brief IPv4 address
struct IpV4Address {
  union {
    /// @brief Address as 4 bytes in network byte order
    uint8_t u8[4];
    /// @brief Address as 2 words in network byte order
    uint16_t u16[2];
    /// @brief Address as dword in network byte order
    uint32_t u32;
  };

  /// @brief Create a zero IPv4 address
  IpV4Address();

  /// @brief Create an IPv4 address from bytes in network byte order
  IpV4Address (uint8_t u8_0, uint8_t u8_1, uint8_t u8_2, uint8_t u8_3);

  /// @brief Create an IPv4 address from words in network byte order
  IpV4Address (uint16_t u16_0, uint16_t u16_1);

  /// @brief Create an IPv4 address from dword in network byte order
  IpV4Address (uint32_t u32);

  /// @brief Create an IPv4 address from string
  IpV4Address (const std::string& address);

  /// @brief Convert address to string
  /// @return address as string
  std::string ToString() const;
};

//==============================================================================

/// @brief IPv6 address
struct IpV6Address {
  union {
    /// @brief Address as 16 bytes in network byte order
    uint8_t u8[16];
    /// @brief Address as 8 words in network byte order
    uint16_t u16[8];
    /// @brief Address as 4 dwords in network byte order
    uint32_t u32[4];
  };
  /// @brief IPv6 zone ID
  uint8_t zoneId;

  /// @brief Create a zero IPv6 address
  IpV6Address();

  /// @brief Create an IPv6 address from bytes in network byte order
  IpV6Address (uint8_t u8_0, uint8_t u8_1, uint8_t u8_2, uint8_t u8_3, uint8_t u8_4, uint8_t u8_5, uint8_t u8_6, uint8_t u8_7,
               uint8_t u8_8, uint8_t u8_9, uint8_t u8_10, uint8_t u8_11, uint8_t u8_12, uint8_t u8_13, uint8_t u8_14, uint8_t u8_15, uint8_t zoneId = 0);

  /// @brief Create an IPv6 address from words in network byte order
  IpV6Address (uint16_t u16_0, uint16_t u16_1, uint16_t u16_2, uint16_t u16_3, uint16_t u16_4, uint16_t u16_5, uint16_t u16_6, uint16_t u16_7, uint8_t zoneId = 0);

  /// @brief Create an IPv6 address from dwords in network byte order
  IpV6Address (uint32_t u32_0, uint32_t u32_1, uint32_t u32_2, uint32_t u32_3, uint8_t zoneId = 0);

  /// @brief Create an IPv6 address from string
  IpV6Address (const std::string& address);

  /// @brief Convert address to string
  /// @return address as string
  std::string ToString() const;
};

//==============================================================================

/// @brief Network address (IPv4 or IPv6)
struct NetworkAddress {
  /// @brief Address family
  NetworkAddressFamily family;

  union {
    /// @brief IPv4 address
    IpV4Address ipV4;
    /// @brief IPv6 address
    IpV6Address ipV6;
  };

  /// @brief Create a zero network address
  NetworkAddress();
  /// @brief Create an IPv4 address
  NetworkAddress (IpV4Address address);
  /// @brief Create an IPv6 address
  NetworkAddress (IpV6Address address);
  
  /// @brief Convert address to string
  /// @return address as string
  std::string ToString() const;
};

//==============================================================================

/// @brief Network endpoint (address and port)
struct NetworkEndpoint {
  /// @brief Address
  NetworkAddress address;
  /// @brief Port
  uint16_t port;  

  /// @brief Create zero network endpoint
  NetworkEndpoint();
  /// @brief Create an IPv4 network endpoint
  NetworkEndpoint (IpV4Address address, uint16_t port);
  /// @brief Create an IPv6 network endpoint
  NetworkEndpoint (IpV6Address address, uint16_t port);
};

//==============================================================================

}