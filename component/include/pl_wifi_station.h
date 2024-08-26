#pragma once
#include "pl_network_interface.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief Wi-Fi station class
class WiFiStation : public virtual NetworkInterface {
public:
  /// @brief Creates a Wi-Fi station
  WiFiStation() {}

  /// @brief Gets station SSID
  /// @return station SSID
  virtual std::string GetSsid() = 0;

  /// @brief Sets station SSID
  /// @param ssid station SSID
  /// @return error code
  virtual esp_err_t SetSsid(const std::string& ssid) = 0;

  /// @brief Gets station password
  /// @return station password
  virtual std::string GetPassword() = 0;

  /// @brief Sets station password
  /// @param password station password
  /// @return error code
  virtual esp_err_t SetPassword(const std::string& password) = 0;
};

//==============================================================================

}