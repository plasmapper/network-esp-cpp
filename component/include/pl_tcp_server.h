#pragma once
#include "pl_network_stream.h"
#include "pl_network_server.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief TCP server class
class TcpServer : public NetworkServer {
public:
  /// @brief Default server task parameters
  static const TaskParameters defaultTaskParameters;
  /// @brief Default maximum number of server clients
  static const int defaultMaxNumberOfClients = 1;
  /// @brief Default idle time before the keep-alive packets are sent in seconds
  static const int defaultKeepAliveIdleTime = 7200;
  /// @brief Default keep-alive packet interval in seconds
  static const int defaultKeepAliveInterval = 75;
  /// @brief Default number of the keep-alive packets
  static const int defaultKeepAliveCount = 9;

  /// @brief Client connected event
  Event<TcpServer, NetworkStream&> clientConnectedEvent;
  /// @brief Client disconnected event
  Event<TcpServer, NetworkStream&> clientDisconnectedEvent;

  /// @brief Create a TCP server
  /// @param port port
  TcpServer (uint16_t port);
  ~TcpServer();
  TcpServer (const TcpServer&) = delete;
  TcpServer& operator= (const TcpServer&) = delete;

  esp_err_t Lock (TickType_t timeout = portMAX_DELAY) override;
  esp_err_t Unlock() override;

  esp_err_t Enable() override;
  esp_err_t Disable() override;

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

  bool IsEnabled() override;

  uint16_t GetPort() override;
  esp_err_t SetPort (uint16_t port) override;

  size_t GetMaxNumberOfClients() override;
  esp_err_t SetMaxNumberOfClients (size_t maxNumberOfClients) override;

  /// @brief Get the connected client streams
  /// @return client streams
  std::vector<std::shared_ptr<NetworkStream>> GetClientStreams();

  /// @brief Set the server task parameters
  /// @param taskParameters task parameters
  /// @return error code
  esp_err_t SetTaskParameters (const TaskParameters& taskParameters);

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

protected:
  /// @brief Handle the TCP client request
  /// @param clientStream client stream
  /// @return error code
  virtual esp_err_t HandleRequest (NetworkStream& clientStream) = 0;

private:
  Mutex mutex;
  enum class Status {stopped, starting, started, stopping} status = Status::stopped;
  uint16_t port = 0;
  int maxNumberOfClients = defaultMaxNumberOfClients;
  std::vector<std::shared_ptr<NetworkStream>> clientStreams;
  TaskParameters taskParameters = defaultTaskParameters;
  bool nagleAlgorithmEnabled = true;
  bool keepAliveEnabled = false;
  int keepAliveIdleTime = defaultKeepAliveIdleTime;
  int keepAliveInterval = defaultKeepAliveInterval;
  int keepAliveCount = defaultKeepAliveCount;

  esp_err_t SetStreamSocketOptions();
  static void TaskCode (void* parameters);
};

//==============================================================================

}