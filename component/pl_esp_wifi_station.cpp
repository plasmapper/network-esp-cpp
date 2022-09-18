#include "pl_esp_wifi_station.h"
#include "esp_event.h"

//==============================================================================

namespace PL {

//==============================================================================

const std::string EspWiFiStation::defaultName = "Wi-Fi";

//==============================================================================

EspWiFiStation::EspWiFiStation() {
  SetName (defaultName);
}

//==============================================================================

EspWiFiStation::~EspWiFiStation() {
  esp_event_handler_unregister (WIFI_EVENT, ESP_EVENT_ANY_ID, EventHandler);

  if (netif)
    esp_netif_destroy_default_wifi (netif);
}

//==============================================================================

esp_err_t EspWiFiStation::Lock (TickType_t timeout) {
  return mutex.Lock (timeout);
}

//==============================================================================

esp_err_t EspWiFiStation::Unlock() {
  return mutex.Unlock();
}

//==============================================================================

esp_err_t EspWiFiStation::Initialize() {
  LockGuard lg (*this);
  if (netif)
    return ESP_OK;

  wifi_init_config_t wifiInitCfg = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t config = {};
  config.sta.pmf_cfg.capable = true;

  netif = esp_netif_create_default_wifi_sta();
  PL_RETURN_ON_ERROR (esp_wifi_init (&wifiInitCfg));
  PL_RETURN_ON_ERROR (esp_wifi_set_mode (WIFI_MODE_STA));
  PL_RETURN_ON_ERROR (esp_wifi_set_ps (WIFI_PS_NONE));
  PL_RETURN_ON_ERROR (esp_wifi_set_config (WIFI_IF_STA, &config));

  PL_RETURN_ON_ERROR (esp_event_handler_instance_register (WIFI_EVENT, ESP_EVENT_ANY_ID, EventHandler, this, NULL));
  PL_RETURN_ON_ERROR (EspNetworkInterface::Initialize (netif));
  return ESP_OK;
}

//==============================================================================

esp_err_t EspWiFiStation::Enable() {
  LockGuard lg (*this);
  if (!netif)
    return ESP_ERR_INVALID_STATE;
  if (enabled)
    return ESP_OK;

  wifi_config_t config;
  PL_RETURN_ON_ERROR (esp_wifi_get_config (WIFI_IF_STA, &config));
  snprintf ((char*)config.sta.ssid, sizeof (config.sta.ssid), ssid.c_str());
  snprintf ((char*)config.sta.password, sizeof (config.sta.password), password.c_str());
  PL_RETURN_ON_ERROR (esp_wifi_set_config (WIFI_IF_STA, &config));

  PL_RETURN_ON_ERROR (esp_wifi_start());
  enabled = true;
  enabledEvent.Generate();

  return ESP_OK;
}

//==============================================================================

esp_err_t EspWiFiStation::Disable() {
  LockGuard lg (*this);
  if (!netif)
    return ESP_ERR_INVALID_STATE;
  if (!enabled)
    return ESP_OK;
  PL_RETURN_ON_ERROR (esp_wifi_stop());
  enabled = false;
  disabledEvent.Generate();

  return ESP_OK;
}

//==============================================================================

bool EspWiFiStation::IsEnabled() {
  LockGuard lg (*this);
  return enabled;
}

//==============================================================================

bool EspWiFiStation::IsConnected() {
  LockGuard lg (*this);
  return enabled && connected;
}

//==============================================================================

std::string EspWiFiStation::GetSsid() {
  LockGuard lg (*this);
  return ssid;
}

//==============================================================================

esp_err_t EspWiFiStation::SetSsid (const std::string& ssid) {
  LockGuard lg (*this);
  this->ssid = ssid;

  if (enabled) {
    Disable();
    return Enable();
  }
  return ESP_OK;
}

//==============================================================================

std::string EspWiFiStation::GetPassword() {
  LockGuard lg (*this);
  return password;
}

//==============================================================================

esp_err_t EspWiFiStation::SetPassword (const std::string& password) {
  LockGuard lg (*this);
  this->password = password;

  if (enabled) {
    Disable();
    return Enable();
  }
  return ESP_OK;
}

//==============================================================================

void EspWiFiStation::EventHandler (void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData) {
  EspWiFiStation& wifiStation = *(EspWiFiStation*)arg;
  LockGuard lg (wifiStation);

  if (eventBase == WIFI_EVENT) {
    if (eventID == WIFI_EVENT_STA_START) {
      esp_wifi_connect();
    }
    if (eventID == WIFI_EVENT_STA_CONNECTED) {
      esp_netif_create_ip6_linklocal (wifiStation.netif);
      if (!wifiStation.connected) {
        wifiStation.connected = true;
        wifiStation.connectedEvent.Generate();
      }
    }
    if (eventID == WIFI_EVENT_STA_DISCONNECTED) {
      if (wifiStation.connected) {
        wifiStation.connected = false;
        wifiStation.disconnectedEvent.Generate();
      }
      if (wifiStation.enabled)
        esp_wifi_connect();
    }  
  }
}

//==============================================================================

}