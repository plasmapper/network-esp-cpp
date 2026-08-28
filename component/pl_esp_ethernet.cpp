#include "pl_esp_ethernet.h"
#include "esp_check.h"
#include "esp_event.h"

#if CONFIG_ETH_USE_ESP32_EMAC

//==============================================================================

static const char* TAG = "pl_esp_ethernet";

//==============================================================================

namespace PL {

//==============================================================================

EspEthernet::EspEthernet(PhyNewFunction phyNewFunction, int32_t phyAddress, int resetPin, int mdcPin, int mdioPin) : phyNewFunction(phyNewFunction) {
  phyConfig.phy_addr = phyAddress;
  phyConfig.reset_gpio_num = resetPin;
  esp32EmacConfig.smi_gpio.mdc_num = mdcPin;
  esp32EmacConfig.smi_gpio.mdio_num = mdioPin;

  SetName(defaultName);
}

//==============================================================================

EspEthernet::~EspEthernet() {
  UnregisterEventHandler();
  if (netif) {
    Disable();
    esp_event_handler_instance_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, eventHandlerInstance);
    esp_eth_del_netif_glue(netifGlueHandle);
    esp_netif_destroy(netif);
    if (esp_eth_driver_uninstall(handle) == ESP_OK) {
      mac->del(mac);
      phy->del(phy);
    }
  }
}

//==============================================================================

esp_err_t EspEthernet::Lock(TickType_t timeout) {
  esp_err_t error = mutex.Lock(timeout);
  if (error != ESP_OK && (error != ESP_ERR_TIMEOUT || timeout != 0))
    ESP_LOGE(TAG, "mutex lock failed");
  return error;
}

//==============================================================================

esp_err_t EspEthernet::Unlock() {
  ESP_RETURN_ON_ERROR(mutex.Unlock(), TAG, "mutex unlock failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t EspEthernet::Initialize() {
  LockGuard lg(*this);
  if (netif)
    return ESP_OK;

  esp_eth_phy_t* newPhy = phyNewFunction(&phyConfig);
  ESP_RETURN_ON_FALSE(newPhy, ESP_ERR_NO_MEM, TAG, "phy new failed");

  esp_eth_mac_t* newMac = esp_eth_mac_new_esp32(&esp32EmacConfig, &macConfig);
  if (!newMac) {
    newPhy->del(newPhy);
    ESP_RETURN_ON_ERROR(ESP_ERR_NO_MEM, TAG, "mac new failed");
  }

  esp_eth_config_t ethernetConfig = ETH_DEFAULT_CONFIG(newMac, newPhy);

  esp_eth_handle_t newHandle = NULL;
  esp_err_t error = esp_eth_driver_install(&ethernetConfig, &newHandle);
  if (error != ESP_OK) {
    newMac->del(newMac);
    newPhy->del(newPhy);
    ESP_RETURN_ON_ERROR(error, TAG, "driver install failed");
  }

  esp_netif_config_t netifConfig = ESP_NETIF_DEFAULT_ETH();
  esp_netif_t* newNetif = esp_netif_new(&netifConfig);
  esp_eth_netif_glue_handle_t newNetifGlueHandle = esp_eth_new_netif_glue(newHandle);
  if (!newNetif || !newNetifGlueHandle) {
    if (newNetifGlueHandle)
      esp_eth_del_netif_glue(newNetifGlueHandle);
    if (newNetif)
      esp_netif_destroy(newNetif);
    esp_eth_driver_uninstall(newHandle);
    newMac->del(newMac);
    newPhy->del(newPhy);
    ESP_RETURN_ON_ERROR(ESP_ERR_NO_MEM, TAG, "netif or netif glue create failed");
  }

  if ((error = esp_netif_attach(newNetif, newNetifGlueHandle)) != ESP_OK) {
    esp_eth_del_netif_glue(newNetifGlueHandle);
    esp_netif_destroy(newNetif);
    esp_eth_driver_uninstall(newHandle);
    newMac->del(newMac);
    newPhy->del(newPhy);
    ESP_RETURN_ON_ERROR(error, TAG, "netif attach failed");
  }

  if ((error = esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, EventHandler, this, &eventHandlerInstance)) != ESP_OK) {
    esp_eth_del_netif_glue(newNetifGlueHandle);
    esp_netif_destroy(newNetif);
    esp_eth_driver_uninstall(newHandle);
    newMac->del(newMac);
    newPhy->del(newPhy);
    ESP_RETURN_ON_ERROR(error, TAG, "event handler instance register failed");
  }

  if ((error = InitializeNetif(newNetif)) != ESP_OK) {
    esp_event_handler_instance_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, eventHandlerInstance);
    esp_eth_del_netif_glue(newNetifGlueHandle);
    esp_netif_destroy(newNetif);
    esp_eth_driver_uninstall(newHandle);
    newMac->del(newMac);
    newPhy->del(newPhy);
    ESP_RETURN_ON_ERROR(error, TAG, "network interface initialize failed");
  }

  handle = newHandle;
  netif = newNetif;
  netifGlueHandle = newNetifGlueHandle;
  mac = newMac;
  phy = newPhy;
  return ESP_OK;
}

//==============================================================================

esp_err_t EspEthernet::Enable() {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(netif, ESP_ERR_INVALID_STATE, TAG, "ethernet is not initialized");
  if (enabled)
    return ESP_OK;

  ESP_RETURN_ON_ERROR(esp_eth_start(handle), TAG, "start failed");
  enabled = true;
  enabledEvent.Generate();

  return ESP_OK;
}

//==============================================================================

esp_err_t EspEthernet::Disable() {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(netif, ESP_ERR_INVALID_STATE, TAG, "ethernet is not initialized");
  if (!enabled)
    return ESP_OK;

  ESP_RETURN_ON_ERROR(esp_eth_stop(handle), TAG, "stop failed");
  enabled = false;
  disabledEvent.Generate();

  return ESP_OK;
}

//==============================================================================

bool EspEthernet::IsEnabled() {
  LockGuard lg(*this);
  return enabled;
}

//==============================================================================

bool EspEthernet::IsConnected() {
  LockGuard lg(*this);
  return enabled && connected;
}

//==============================================================================

void EspEthernet::EventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData) {
  EspEthernet& ethernet = *(EspEthernet*)arg;

  if (eventBase == ETH_EVENT) {
    if (eventID == ETHERNET_EVENT_CONNECTED) {
      {
        LockGuard lg(ethernet);
        ethernet.connected = true;
        esp_netif_create_ip6_linklocal(ethernet.netif);
        if (ethernet.IsIpV4DhcpClientEnabled())
          esp_netif_dhcpc_start(ethernet.netif);
        else
          esp_netif_dhcpc_stop(ethernet.netif);
      }
      ethernet.connectedEvent.Generate();
    }
    if (eventID == ETHERNET_EVENT_DISCONNECTED) {
      bool wasConnected;
      {
        LockGuard lg(ethernet);
        wasConnected = ethernet.connected;
        ethernet.connected = false;
      }
      if (wasConnected)
        ethernet.disconnectedEvent.Generate();
    }
  }
}

//==============================================================================

}

#endif