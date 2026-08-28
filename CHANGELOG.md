# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Fixed
- EspWiFiStation::defaultName and EspEthernet::defaultName static initialization order dependency.
- NetworkStream::SetReadTimeout and SetWriteTimeout with a zero timeout blocking forever instead of not blocking.
- NetworkStream::SockAddrToEndpoint IPv4-mapped-IPv6 detection depending on host endianness and reading the port through the wrong sockaddr type.
- EspNetworkInterface unregistering the IP event handler in the base destructor, after the derived class's own members were already destroyed.

## [1.4.3] - 2026-08-27
### Fixed
- EspWiFiStation::SetSsid and SetPassword restarting Wi-Fi even when the value did not change.
- EspNetworkInterface::SetIpV4Address, SetIpV4Netmask and SetIpV4Gateway calling esp_netif_set_ip_info even when the value did not change.

## [1.4.2] - 2026-08-27
### Fixed
- TcpServer::SetPort, SetMaxNumberOfClients and SetTaskParameters restarting the server even when the value did not change.

## [1.4.1] - 2026-08-25
### Fixed
- TcpServer destruction sequence.
- EspWiFiStation and EspEthernet destructors not disabling before releasing driver resources, so disabledEvent never fired on destruction.

## [1.4.0] - 2026-08-24
### Added
- TcpClient connect timeout.

### Fixed
- TcpServer task not being stopped before the derived object is destroyed.
- TcpServer accept loop spinning forever on a failed accept.
- TcpServer not generating disabledEvent when Listen fails.
- NetworkStream::Read and NetworkStream::Write closing the socket on a timeout based on stale errno.
- EspEthernet and EspWiFiStation destructors freeing driver resources before the driver was safely stopped.
- Missing socket ownership documentation for the NetworkStream constructor.
- EspEthernet::Initialize and EspWiFiStation::Initialize not checking driver and netif creation results before use.
- NetworkAddress default constructor leaving the address union uninitialized.
- EspNetworkInterface logging an error for permanently unsupported IPv6 operations.

## [1.3.3] - 2026-08-19
### Fixed
- EspNetworkInterface IPv4 events not filtered by netif and Ethernet IP-lost event ignored.
- Unsynchronized TcpServer task state flags.
- EspEthernet leaking PHY and MAC driver objects.
- tcp_ota_server example not calling esp_ota_abort if error occurs.

## [1.3.2] - 2026-08-17
### Fixed
- Read and Write not implementing overall timeout.

## [1.3.1] - 2026-08-14
### Fixed
- Discard read performance when discarding data one byte at a time.

## [1.3.0] - 2026-08-14
### Added
- Write operation timeout to NetworkStream and TcpClient.

### Fixed
- Event handler register/unregister.
- Read misreporting a closed connection as a timeout.
- IpV4Address and IpV6Address comparison operators not being const.
- sprintf used instead of snprintf for address formatting.
- Documentation of sin6_scope_id to IpV6Address::zoneId narrowing in SockAddrToEndpoint.

## [1.2.2] - 2026-08-11
### Fixed
- Invalid iterator use in TcpServer::TaskCode.

## [1.2.1] - 2026-08-10
### Changed
- Example and test to use esp_eth_phy_new_generic.

## [1.2.0] - 2026-08-10
### Added
- ESP-IDF v6.0 support.

## [1.1.3] - 2026-08-06
### Changed
- Lock timeout handling.
- Static const members to constexpr.
- Documented TcpServer dual-stack dependency and HandleRequest threading model.

### Fixed
- Wrong parameter setting procedure in TcpServer::SetKeepAliveIdleTime.
- WiFi station and Ethernet initialization rollback.
- NetworkStream::GetReadableSize select() error handling.
- EspWiFiStation and EspEthernet event handler thread safety.
- EspNetworkInterface::IsIpV4DhcpClientEnabled missing lock.
- Ignored Disable/Enable errors in EspWiFiStation::SetSsid and SetPassword.
- Ignored DHCP client start/stop errors in EspNetworkInterface.
- Unsafe reinterpret-cast in EspNetworkInterface IPv6 address getters.
- Uninitialized sockaddr_in6 in TcpClient::Connect.
- Misleading error message on TcpClient::Connect connect failure.
- TcpServer::maxNumberOfClients type mismatch with NetworkServer interface.
- Incorrect string formatting in EspWiFiStation::Enable SSID/password handling.
- NetworkStream::GetLocalEndpoint and GetRemoteEndpoint missing lock.
- EspNetworkInterface::EventHandler missing lock.

## [1.1.2] - 2024-09-26
### Added
- IpV4Address and IpV6Address == and != operators.

## [1.1.1] - 2024-09-11
### Fixed
- EspNetworkInterface::Initialize conflict with NetworkInterface::Initialize.

## [1.1.0] - 2024-08-26
### Changed
- ESP-IDF dependency to 5.3.

## [1.0.2] - 2024-07-16
### Fixed
- Compiling ESP Ethernet code only if it is used.

## [1.0.1] - 2024-06-12
### Added
- Copying examples to component folder on upload.

## [1.0.0] - 2024-06-12
Initial release.