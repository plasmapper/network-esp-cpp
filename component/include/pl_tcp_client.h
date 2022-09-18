#pragma once
#include "pl_network_stream.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief TCP client class
class TcpClient : public Lockable {
public:
  /// @brief Create an IPv4 TCP client
  /// @param address IPv4 addres
  /// @param port port
  TcpClient (IpV4Address address, uint16_t port);

  /// @brief Create an IPv6 TCP client
  /// @param address IPv6 addres
  /// @param port port
  TcpClient (IpV6Address address, uint16_t port);
  ~TcpClient ();
  TcpClient (const TcpClient&) = delete;
  TcpClient& operator= (const TcpClient&) = delete;

  esp_err_t Lock (TickType_t timeout = portMAX_DELAY) override;
  esp_err_t Unlock() override;

  /// @brief Connect to the server if not already connected
  /// @return error code
  esp_err_t Connect();

  /// @brief Disconnect from the server
  /// @return error code
  esp_err_t Disconnect();

  /// @brief Enable the Nagle algorithm
  /// @return error code
  esp_err_t EnableNagleAlgorithm();

  /// @brief Disable the Nagle algorithm
  /// @return error code
  esp_err_t DisableNagleAlgorithm();

  /// @brief Check if the client is connected
  /// @return true if the client is connected
  bool IsConnected();

  /// @brief Get the read operation timeout 
  /// @return timeout in FreeRTOS ticks
  TickType_t GetReadTimeout();

  /// @brief Set the read operation timeout 
  /// @param timeout timeout in FreeRTOS ticks
  /// @return error code
  esp_err_t SetReadTimeout (TickType_t timeout);

  /// @brief Get the local endpoint of the client
  /// @return local endpoint
  NetworkEndpoint GetLocalEndpoint();

  /// @brief Get the remote endpoint of the client
  /// @return remote endpoint
  NetworkEndpoint GetRemoteEndpoint();

  /// @brief Set the IPv4 remote endpoint of the client
  /// @param address IPv4 address
  /// @param port port
  /// @return error code
  esp_err_t SetRemoteEndpoint (IpV4Address address, uint16_t port);

  /// @brief Set the IPv6 remote endpoint of the client
  /// @param address IPv6 address
  /// @param port port
  /// @return error code
  esp_err_t SetRemoteEndpoint (IpV6Address address, uint16_t port);

  /// @brief Get the client stream
  /// @return stream
  std::shared_ptr<NetworkStream> GetStream();

private:
  Mutex mutex;
  NetworkEndpoint remoteEndpoint;
  std::shared_ptr<NetworkStream> stream;
  TickType_t readTimeout = NetworkStream::defaultReadTimeout;
  bool nagleAlgorithmEnabled = true;
};

//==============================================================================

}