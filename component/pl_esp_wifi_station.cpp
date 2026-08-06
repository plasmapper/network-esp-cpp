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
  SetName(defaultName);
}

//==============================================================================

EspWiFiStation::~EspWiFiStation() {
  if (netif) {
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, EventHandler);
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(netif);
  }
}

//==============================================================================

esp_err_t EspWiFiStation::Lock(TickType_t timeout) {
  esp_err_t error = mutex.Lock(timeout);
  if (error != ESP_OK && (error != ESP_ERR_TIMEOUT || timeout != 0))
    ESP_LOGE(TAG, "mutex lock failed");
  return error;
}

//==============================================================================

esp_err_t EspWiFiStation::Unlock() {
  ESP_RETURN_ON_ERROR(mutex.Unlock(), TAG, "mutex unlock failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t EspWiFiStation::Initialize() {
  LockGuard lg(*this);
  if (netif)
    return ESP_OK;

  wifi_init_config_t wifiInitCfg = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t config = {};
  config.sta.pmf_cfg.capable = true;

  esp_netif_t* newNetif = esp_netif_create_default_wifi_sta();
  
  esp_err_t error = esp_wifi_init(&wifiInitCfg);
  if (error != ESP_OK) {
    esp_netif_destroy_default_wifi(newNetif);
    ESP_RETURN_ON_ERROR(error, TAG, "init failed");
  }

  if ((error = esp_wifi_set_mode(WIFI_MODE_STA)) != ESP_OK) {
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(newNetif);
    ESP_RETURN_ON_ERROR(error, TAG, "set mode failed");
  }

  if ((error = esp_wifi_set_ps(WIFI_PS_NONE)) != ESP_OK) {
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(newNetif);
    ESP_RETURN_ON_ERROR(error, TAG, "set power save type failed");
  }

  if ((error = esp_wifi_set_config(WIFI_IF_STA, &config)) != ESP_OK) {
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(newNetif);
    ESP_RETURN_ON_ERROR(error, TAG, "set config failed");
  }

  if ((error = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, EventHandler, this, NULL)) != ESP_OK) {
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(newNetif);
    ESP_RETURN_ON_ERROR(error, TAG, "event handler instance register failed");
  }

  if ((error = InitializeNetif(newNetif)) != ESP_OK) {
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, EventHandler);
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(newNetif);
    ESP_RETURN_ON_ERROR(error, TAG, "network interface initialize failed");
  }

  netif = newNetif;
  return ESP_OK;
}

//==============================================================================

esp_err_t EspWiFiStation::Enable() {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(netif, ESP_ERR_INVALID_STATE, TAG, "WiFi is not initialized");
  if (enabled)
    return ESP_OK;

  wifi_config_t config;
  ESP_RETURN_ON_ERROR(esp_wifi_get_config(WIFI_IF_STA, &config), TAG, "get config failed");
  snprintf((char*)config.sta.ssid, sizeof(config.sta.ssid), "%s", ssid.c_str());
  snprintf((char*)config.sta.password, sizeof(config.sta.password), "%s", password.c_str());
  ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &config), TAG, "set config failed");

  ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "start failed");
  enabled = true;
  enabledEvent.Generate();

  return ESP_OK;
}

//==============================================================================

esp_err_t EspWiFiStation::Disable() {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(netif, ESP_ERR_INVALID_STATE, TAG, "WiFi is not initialized");
  if (!enabled)
    return ESP_OK;
  ESP_RETURN_ON_ERROR(esp_wifi_stop(), TAG, "stop failed");
  enabled = false;
  disabledEvent.Generate();

  return ESP_OK;
}

//==============================================================================

bool EspWiFiStation::IsEnabled() {
  LockGuard lg(*this);
  return enabled;
}

//==============================================================================

bool EspWiFiStation::IsConnected() {
  LockGuard lg(*this);
  return enabled && connected;
}

//==============================================================================

std::string EspWiFiStation::GetSsid() {
  LockGuard lg(*this);
  return ssid;
}

//==============================================================================

esp_err_t EspWiFiStation::SetSsid(const std::string& ssid) {
  LockGuard lg(*this);
  this->ssid = ssid;

  if (enabled) {
    ESP_RETURN_ON_ERROR(Disable(), TAG, "disable failed");
    ESP_RETURN_ON_ERROR(Enable(), TAG, "enable failed");
  }
  return ESP_OK;
}

//==============================================================================

std::string EspWiFiStation::GetPassword() {
  LockGuard lg(*this);
  return password;
}

//==============================================================================

esp_err_t EspWiFiStation::SetPassword(const std::string& password) {
  LockGuard lg(*this);
  this->password = password;

  if (enabled) {
    ESP_RETURN_ON_ERROR(Disable(), TAG, "disable failed");
    ESP_RETURN_ON_ERROR(Enable(), TAG, "enable failed");
  }
  return ESP_OK;
}

//==============================================================================

void EspWiFiStation::EventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData) {
  EspWiFiStation& wifiStation = *(EspWiFiStation*)arg;

  if (eventBase == WIFI_EVENT) {
    if (eventID == WIFI_EVENT_STA_START) {
      esp_wifi_connect();
    }
    if (eventID == WIFI_EVENT_STA_CONNECTED) {
      {
        LockGuard lg(wifiStation);
        wifiStation.connected = true;
        esp_netif_create_ip6_linklocal(wifiStation.netif);
        if (wifiStation.IsIpV4DhcpClientEnabled())
          esp_netif_dhcpc_start(wifiStation.netif);
        else
          esp_netif_dhcpc_stop(wifiStation.netif);
      }
      wifiStation.connectedEvent.Generate();
    }
    if (eventID == WIFI_EVENT_STA_DISCONNECTED) {
      bool wasConnected, isEnabled;
      {
        LockGuard lg(wifiStation);
        wasConnected = wifiStation.connected;
        wifiStation.connected = false;
        isEnabled = wifiStation.enabled;
      }
      if (wasConnected)
        wifiStation.disconnectedEvent.Generate();
      if (isEnabled)
        esp_wifi_connect();
    }
  }
}

//==============================================================================

}