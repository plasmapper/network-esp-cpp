#pragma once
#include "pl_common.h"
#include "pl_network_types.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief Network interface class
class NetworkInterface : public HardwareInterface {
public:
  /// @brief Network interface connected event
  Event<NetworkInterface> connectedEvent;
  /// @brief Network interface disconnected event
  Event<NetworkInterface> disconnectedEvent;
  /// @brief Network interface got IPv4 event
  Event<NetworkInterface> gotIpV4AddressEvent;
  /// @brief Network interface lost IPv4 event
  Event<NetworkInterface> lostIpV4AddressEvent;
  /// @brief Network interface got IPv6 event
  Event<NetworkInterface> gotIpV6AddressEvent;
  /// @brief Network interface lost IPv4 event
  Event<NetworkInterface> lostIpV6AddressEvent;

  /// @brief Creates a network interface
  NetworkInterface();

  /// @brief Enables IPv4 DHCP client
  /// @return error code
  virtual esp_err_t EnableIpV4DhcpClient() = 0;

  /// @brief Disables IPv4 DHCP client
  /// @return error code
  virtual esp_err_t DisableIpV4DhcpClient() = 0;

  /// @brief Enables IPv6 DHCP client
  /// @return error code
  virtual esp_err_t EnableIpV6DhcpClient() = 0;

  /// @brief Disables IPv6 DHCP client
  /// @return error code
  virtual esp_err_t DisableIpV6DhcpClient() = 0;

  /// @brief Checks if the network interface is connected
  /// @return true if the network interface is connected
  virtual bool IsConnected() = 0;

  /// @brief Checks if IPv4 DHCP client is enabled
  /// @return true if IPv4 DHCP client is enabled
  virtual bool IsIpV4DhcpClientEnabled() = 0;

  /// @brief Checks if IPv6 DHCP client is enabled
  /// @return true if IPv6 DHCP client is enabled
  virtual bool IsIpV6DhcpClientEnabled() = 0;

  /// @brief Gets IPv4 address
  /// @return IPv4 address
  virtual IpV4Address GetIpV4Address() = 0;

  /// @brief Sets IPv4 address
  /// @param address IPv4 address
  /// @return error code
  virtual esp_err_t SetIpV4Address(IpV4Address address) = 0;

  /// @brief Gets IPv4 netmask
  /// @return IPv4 netmask
  virtual IpV4Address GetIpV4Netmask() = 0;

  /// @brief Sets IPv4 netmask
  /// @param netmask IPv4 netmask
  /// @return error code
  virtual esp_err_t SetIpV4Netmask(IpV4Address netmask) = 0;

  /// @brief Gets IPv4 gateway
  /// @return IPv4 gateway
  virtual IpV4Address GetIpV4Gateway() = 0;

  /// @brief Sets IPv4 gateway
  /// @param gateway IPv4 gateway
  /// @return error code
  virtual esp_err_t SetIpV4Gateway(IpV4Address gateway) = 0;

  /// @brief Gets IPv6 link local address
  /// @return IPv6 link local address
  virtual IpV6Address GetIpV6LinkLocalAddress() = 0;

  /// @brief Gets IPv6 global address
  /// @return IPv6 global address
  virtual IpV6Address GetIpV6GlobalAddress() = 0;

  /// @brief Sets IPv6 global address
  /// @param address IPv6 global address
  /// @return error code
  virtual esp_err_t SetIpV6GlobalAddress(IpV6Address address) = 0;
};
  
//==============================================================================

}