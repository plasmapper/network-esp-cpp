#pragma once
#include "pl_wifi_station.h"
#include "pl_esp_network_interface.h"
#include "esp_wifi.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief Internal ESP Wi-Fi station class
class EspWiFiStation : public EspNetworkInterface, public WiFiStation {
public:
  /// @brief Default hardware interface name
  static const std::string defaultName;

  /// @brief Creates an ESP Wi-Fi station
  EspWiFiStation();
  ~EspWiFiStation();
  EspWiFiStation(const EspWiFiStation&) = delete;
  EspWiFiStation& operator=(const EspWiFiStation&) = delete;

  esp_err_t Lock(TickType_t timeout = portMAX_DELAY) override;
  esp_err_t Unlock() override;

  esp_err_t Initialize() override;

  esp_err_t Enable() override;
  esp_err_t Disable() override;

  bool IsEnabled() override;
  bool IsConnected() override;
  
  std::string GetSsid() override;
  esp_err_t SetSsid(const std::string& ssid) override;

  std::string GetPassword() override;
  esp_err_t SetPassword(const std::string& password) override;

private:
  Mutex mutex;
  esp_netif_t* netif = NULL;
  bool enabled = false, connected = false;
  std::string ssid, password;

  static void EventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData);
};

//==============================================================================

}