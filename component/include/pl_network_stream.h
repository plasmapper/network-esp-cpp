#pragma once
#include "pl_common.h"
#include "pl_network_types.h"
#include "lwip/sockets.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief Network stream class
class NetworkStream : public Stream {
public:
  /// @brief Default read operation timeout in FreeRTOS ticks
  static const TickType_t defaultReadTimeout = 300 / portTICK_PERIOD_MS;

  /// @brief Create a closed network stream
  NetworkStream() {}

  /// @brief Create an open network stream
  /// @param sock stream socket
  NetworkStream (int sock);

  esp_err_t Lock (TickType_t timeout = portMAX_DELAY) override;
  esp_err_t Unlock() override;

  using Stream::Read;
  esp_err_t Read (void* dest, size_t size) override;
  using Stream::Write;
  esp_err_t Write (const void* src, size_t size) override;

  /// @brief Close the stream
  /// @return error code
  esp_err_t Close();

  /// @brief Enable the Nagle algorithm
  /// @return error code
  esp_err_t EnableNagleAlgorithm();

  /// @brief Disable the Nagle algorithm
  /// @return error code
  esp_err_t DisableNagleAlgorithm();
  
  /// @brief Enable the keep-alive packets
  /// @return error code
  esp_err_t EnableKeepAlive();
  
  /// @brief Disable the keep-alive packets
  /// @return error code
  esp_err_t DisableKeepAlive();

  /// @brief Check if the stream is open
  /// @return true if the stream is open
  bool IsOpen();

  size_t GetReadableSize() override;

  TickType_t GetReadTimeout() override;
  esp_err_t SetReadTimeout (TickType_t timeout) override;

  /// @brief Get the local endpoint of the stream 
  /// @return local endpoint
  NetworkEndpoint GetLocalEndpoint();

  /// @brief Get the remote endpoint of the stream 
  /// @return remote endpoint
  NetworkEndpoint GetRemoteEndpoint();

  /// @brief Set the idle time before the keep-alive packets are sent
  /// @param seconds time in seconds
  /// @return error code
  esp_err_t SetKeepAliveIdleTime (int seconds);

  /// @brief Set the keep-alive packet interval
  /// @param seconds interval in seconds
  /// @return error code
  esp_err_t SetKeepAliveInterval (int seconds);

  /// @brief Set the number of the keep-alive packets
  /// @param count number of packets
  /// @return error code
  esp_err_t SetKeepAliveCount (int count);

private:
  Mutex mutex;
  int sock = -1;
  TickType_t readTimeout = defaultReadTimeout;

  NetworkEndpoint SockAddrToEndpoint (sockaddr_storage& sockAddr);
  esp_err_t SetSocketOption (int level, int option, int value);
};
  
//==============================================================================

}