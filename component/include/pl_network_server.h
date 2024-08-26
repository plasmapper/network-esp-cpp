#pragma once
#include "pl_common.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief Network server class
class NetworkServer : public Server {
public:
  /// @brief Creates a network server
  NetworkServer() {}

  /// @brief Gets listening port
  /// @return port
  virtual uint16_t GetPort() = 0;

  /// @brief Sets listening port
  /// @param port port
  /// @return error code
  virtual esp_err_t SetPort(uint16_t port) = 0;

  /// @brief Gets the maximum number of server clients
  /// @return number of clients
  virtual size_t GetMaxNumberOfClients() = 0;

  /// @brief Sets the maximum number of server clients
  /// @param maxNumberOfClients number of clients
  /// @return error code
  virtual esp_err_t SetMaxNumberOfClients(size_t maxNumberOfClients) = 0;
};
  
//==============================================================================

}