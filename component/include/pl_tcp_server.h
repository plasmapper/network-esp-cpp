#pragma once
#include "pl_network_stream.h"
#include "pl_network_server.h"
#include <atomic>

//==============================================================================

namespace PL {

//==============================================================================

/// @brief TCP server class
/// @note Listens on a single IPv6 socket and relies on lwIP dual-stack support
/// (IPV6_V6ONLY disabled, the ESP-IDF default) to accept IPv4 clients as well,
/// via IPv4-mapped IPv6 addresses. If dual-stack support is disabled, only
/// IPv6 clients will be able to connect.
class TcpServer : public NetworkServer {
public:
  /// @brief Default server task parameters
  static const TaskParameters defaultTaskParameters;
  /// @brief Default maximum number of server clients
  static constexpr size_t defaultMaxNumberOfClients = 1;
  /// @brief Default idle time before the keep-alive packets are sent in seconds
  static constexpr int defaultKeepAliveIdleTime = 7200;
  /// @brief Default keep-alive packet interval in seconds
  static constexpr int defaultKeepAliveInterval = 75;
  /// @brief Default number of the keep-alive packets
  static constexpr int defaultKeepAliveCount = 9;

  /// @brief Client connected event
  Event<TcpServer, NetworkStream&> clientConnectedEvent;
  /// @brief Client disconnected event
  Event<TcpServer, NetworkStream&> clientDisconnectedEvent;

  /// @brief Creates a TCP server
  /// @param port port
  TcpServer(uint16_t port);
  ~TcpServer();
  TcpServer(const TcpServer&) = delete;
  TcpServer& operator=(const TcpServer&) = delete;

  esp_err_t Lock(TickType_t timeout = portMAX_DELAY) override;
  esp_err_t Unlock() override;

  esp_err_t Enable() override;
  esp_err_t Disable() override;

  /// @brief Enables the Nagle's algorithm
  /// @return error code
  esp_err_t EnableNagleAlgorithm();
  
  /// @brief Disables the Nagle's algorithm
  /// @return error code
  esp_err_t DisableNagleAlgorithm();

  /// @brief Enables the keep-alive packets
  /// @return error code
  esp_err_t EnableKeepAlive();
  
  /// @brief Disables the keep-alive packets
  /// @return error code
  esp_err_t DisableKeepAlive();

  bool IsEnabled() override;

  uint16_t GetPort() override;
  esp_err_t SetPort(uint16_t port) override;

  size_t GetMaxNumberOfClients() override;
  esp_err_t SetMaxNumberOfClients(size_t maxNumberOfClients) override;

  /// @brief Gets the connected client streams
  /// @return client streams
  std::vector<std::shared_ptr<NetworkStream>> GetClientStreams();

  /// @brief Sets the server task parameters
  /// @param taskParameters task parameters
  /// @return error code
  esp_err_t SetTaskParameters(const TaskParameters& taskParameters);

  /// @brief Sets the idle time before the keep-alive packets are sent
  /// @param seconds time in seconds
  /// @return error code
  esp_err_t SetKeepAliveIdleTime(int seconds);

  /// @brief Sets the keep-alive packet interval
  /// @param seconds interval in seconds
  /// @return error code
  esp_err_t SetKeepAliveInterval(int seconds);

  /// @brief Sets the number of the keep-alive packets
  /// @param count number of packets
  /// @return error code
  esp_err_t SetKeepAliveCount(int count);

protected:
  /// @brief Handles the TCP client request
  /// @param clientStream client stream
  /// @return error code
  /// @note Called from a single internal task that also accepts new clients
  /// and services every other connected client in turn, and holds the
  /// server's own lock while doing so. A slow or blocking implementation
  /// delays new connections, all other clients, and any other thread's calls
  /// into this server's public API until it returns.
  virtual esp_err_t HandleRequest(NetworkStream& clientStream) = 0;

private:
  Mutex mutex;
  uint16_t port = 0;
  size_t maxNumberOfClients = defaultMaxNumberOfClients;
  std::vector<std::shared_ptr<NetworkStream>> clientStreams;
  TaskParameters taskParameters = defaultTaskParameters;
  bool nagleAlgorithmEnabled = true;
  bool keepAliveEnabled = false;
  int keepAliveIdleTime = defaultKeepAliveIdleTime;
  int keepAliveInterval = defaultKeepAliveInterval;
  int keepAliveCount = defaultKeepAliveCount;
  std::atomic<TaskHandle_t> taskHandle = NULL;
  std::atomic<bool> disable = false;
  bool disableFromRequest = false;
  bool enableFromRequest = false;

  esp_err_t SetStreamSocketOptions();
  static void TaskCode(void* parameters);

  int Listen();
  esp_err_t RestartIfEnabled(); 
};

//==============================================================================

}