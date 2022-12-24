#include "pl_esp_wifi_station.h"
#include "esp_check.h"
#include "esp_event.h"

//==============================================================================

static const char* TAG = "pl_esp_wifi_station";

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
  esp_err_t error = mutex.Lock (timeout);
  if (error == ESP_OK)
    return ESP_OK;
  if (error == ESP_ERR_TIMEOUT && timeout == 0)
    return ESP_ERR_TIMEOUT;
  ESP_RETURN_ON_ERROR (error, TAG, "mutex lock failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t EspWiFiStation::Unlock() {
  ESP_RETURN_ON_ERROR (mutex.Unlock(), TAG, "mutex unlock failed");
  return ESP_OK;
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
  ESP_RETURN_ON_ERROR (esp_wifi_init (&wifiInitCfg), TAG, "init failed");
  ESP_RETURN_ON_ERROR (esp_wifi_set_mode (WIFI_MODE_STA), TAG, "set mode failed");
  ESP_RETURN_ON_ERROR (esp_wifi_set_ps (WIFI_PS_NONE), TAG, "set power save type failed");
  ESP_RETURN_ON_ERROR (esp_wifi_set_config (WIFI_IF_STA, &config), TAG, "set config failed");

  ESP_RETURN_ON_ERROR (esp_event_handler_instance_register (WIFI_EVENT, ESP_EVENT_ANY_ID, EventHandler, this, NULL), TAG, "event handler instance register failed");
  ESP_RETURN_ON_ERROR (EspNetworkInterface::Initialize (netif), TAG, "network interface initialize failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t EspWiFiStation::Enable() {
  LockGuard lg (*this);
  ESP_RETURN_ON_FALSE (netif, ESP_ERR_INVALID_STATE, TAG, "WiFi is not initialized");
  if (enabled)
    return ESP_OK;

  wifi_config_t config;
  ESP_RETURN_ON_ERROR (esp_wifi_get_config (WIFI_IF_STA, &config), TAG, "get config failed");
  snprintf ((char*)config.sta.ssid, sizeof (config.sta.ssid), ssid.c_str());
  snprintf ((char*)config.sta.password, sizeof (config.sta.password), password.c_str());
  ESP_RETURN_ON_ERROR (esp_wifi_set_config (WIFI_IF_STA, &config), TAG, "set config failed");

  ESP_RETURN_ON_ERROR (esp_wifi_start(), TAG, "start failed");
  enabled = true;
  enabledEvent.Generate();

  return ESP_OK;
}

//==============================================================================

esp_err_t EspWiFiStation::Disable() {
  LockGuard lg (*this);
  ESP_RETURN_ON_FALSE (netif, ESP_ERR_INVALID_STATE, TAG, "WiFi is not initialized");
  if (!enabled)
    return ESP_OK;
  ESP_RETURN_ON_ERROR (esp_wifi_stop(), TAG, "stop failed");
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

  if (eventBase == WIFI_EVENT) {
    if (eventID == WIFI_EVENT_STA_START) {
      esp_wifi_connect();
    }
    if (eventID == WIFI_EVENT_STA_CONNECTED) {
      wifiStation.connected = true;
      esp_netif_create_ip6_linklocal (wifiStation.netif);
      
      if (wifiStation.IsIpV4DhcpClientEnabled())
        esp_netif_dhcpc_start (wifiStation.netif);
      else
        esp_netif_dhcpc_stop (wifiStation.netif);
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