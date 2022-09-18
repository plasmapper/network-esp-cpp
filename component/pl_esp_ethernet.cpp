#include "pl_esp_ethernet.h"
#include "esp_event.h"

//==============================================================================

namespace PL {

//==============================================================================

const std::string EspEthernet::defaultName = "Ethernet";

//==============================================================================

EspEthernet::EspEthernet (PhyNewFunction phyNewFunction, int32_t phyAddress, int resetPin, int mdcPin, int mdioPin) : phyNewFunction (phyNewFunction) {
  phyConfig.phy_addr = phyAddress;
  phyConfig.reset_gpio_num = resetPin;
  macConfig.smi_mdc_gpio_num = mdcPin;
  macConfig.smi_mdio_gpio_num = mdioPin;

  SetName (defaultName);
}

//==============================================================================

EspEthernet::~EspEthernet() {
  esp_event_handler_unregister (ETH_EVENT, ESP_EVENT_ANY_ID, EventHandler);

  if (netifGlueHandle)
    esp_eth_del_netif_glue (netifGlueHandle);

  if (netif)
    esp_netif_destroy (netif);

  if (handle) {
    esp_eth_stop (handle);
    esp_eth_driver_uninstall (handle);
  }
}

//==============================================================================

esp_err_t EspEthernet::Lock (TickType_t timeout) {
  return mutex.Lock (timeout);
}

//==============================================================================

esp_err_t EspEthernet::Unlock() {
  return mutex.Unlock();
}

//==============================================================================

esp_err_t EspEthernet::Initialize() {
  LockGuard lg (*this);
  if (netif)
    return ESP_OK;

  esp_eth_phy_t* phy = phyNewFunction (&phyConfig);
  esp_eth_mac_t* mac = esp_eth_mac_new_esp32 (&macConfig);
  esp_eth_config_t ethernetConfig = ETH_DEFAULT_CONFIG (mac, phy);
  PL_RETURN_ON_ERROR (esp_eth_driver_install (&ethernetConfig, &handle));
  esp_netif_config_t netifConfig = ESP_NETIF_DEFAULT_ETH();
  netif = esp_netif_new (&netifConfig);
  netifGlueHandle = esp_eth_new_netif_glue (handle);
  PL_RETURN_ON_ERROR (esp_netif_attach (netif, netifGlueHandle));

  PL_RETURN_ON_ERROR (esp_event_handler_instance_register (ETH_EVENT, ESP_EVENT_ANY_ID, EventHandler, this, NULL));
  PL_RETURN_ON_ERROR (EspNetworkInterface::Initialize (netif));
  return ESP_OK;
}

//==============================================================================

esp_err_t EspEthernet::Enable() {
  LockGuard lg (*this);
  if (!netif)
    return ESP_ERR_INVALID_STATE;
  if (enabled)
    return ESP_OK;

  PL_RETURN_ON_ERROR (esp_eth_start (handle));
  enabled = true;
  enabledEvent.Generate();

  return ESP_OK;
}

//==============================================================================

esp_err_t EspEthernet::Disable() {
  LockGuard lg (*this);
  if (!netif)
    return ESP_ERR_INVALID_STATE;
  if (!enabled)
    return ESP_OK;

  PL_RETURN_ON_ERROR (esp_eth_stop (handle));
  enabled = false;
  disabledEvent.Generate();

  return ESP_OK;
}

//==============================================================================

bool EspEthernet::IsEnabled() {
  LockGuard lg (*this);
  return enabled;
}

//==============================================================================

bool EspEthernet::IsConnected() {
  LockGuard lg (*this);
  return enabled && connected;
}

//==============================================================================

void EspEthernet::EventHandler (void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData) {
  EspEthernet& ethernet = *(EspEthernet*)arg;
  LockGuard lg (ethernet);

  if (eventBase == ETH_EVENT) {
    if (eventID == ETHERNET_EVENT_CONNECTED) {
      esp_netif_create_ip6_linklocal (ethernet.netif);
      if (!ethernet.connected) {
        ethernet.connected = true;
        ethernet.connectedEvent.Generate();
      }
    }
    if (eventID == ETHERNET_EVENT_DISCONNECTED) {
      if (ethernet.connected) {
        ethernet.connected = false;
        ethernet.disconnectedEvent.Generate();
      }
    }  
  }
}

//==============================================================================

}
