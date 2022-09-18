#pragma once
#include "pl_common.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief Network server class
class NetworkServer : public Server {
public:
  /// @brief Create a network server
  NetworkServer() {}

  /// @brief Get listening port
  /// @return port
  virtual uint16_t GetPort() = 0;

  /// @brief Set listening port
  /// @param port port
  /// @return error code
  virtual esp_err_t SetPort (uint16_t port) = 0;

  /// @brief Get the maximum number of server clients
  /// @return number of clients
  virtual size_t GetMaxNumberOfClients() = 0;

  /// @brief Set the maximum number of server clients
  /// @param maxNumberOfClients number of clients
  /// @return error code
  virtual esp_err_t SetMaxNumberOfClients (size_t maxNumberOfClients) = 0;
};
  
//==============================================================================

}