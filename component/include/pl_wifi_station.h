#pragma once
#include "pl_network_interface.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief Wi-Fi station class
class WiFiStation : public virtual NetworkInterface {
public:
  /// @brief Create a Wi-Fi station
  WiFiStation() {}

  /// @brief Get station SSID
  /// @return station SSID
  virtual std::string GetSsid() = 0;

  /// @brief Set station SSID
  /// @param ssid station SSID
  /// @return error code
  virtual esp_err_t SetSsid(const std::string& ssid) = 0;

  /// @brief Get station password
  /// @return station password
  virtual std::string GetPassword() = 0;

  /// @brief Set station password
  /// @param password station password
  /// @return error code
  virtual esp_err_t SetPassword(const std::string& password) = 0;
};

//==============================================================================

}