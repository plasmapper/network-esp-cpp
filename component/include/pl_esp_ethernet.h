#pragma once
#include "pl_ethernet.h"
#include "pl_esp_network_interface.h"
#include "esp_eth.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief Internal ESP Ethernet interface class
class EspEthernet : public EspNetworkInterface, public Ethernet {
public:
  /// @brief PHY generator function
  typedef esp_eth_phy_t* (*PhyNewFunction)(const eth_phy_config_t*);
  
  /// @brief Default hardware interface name
  static const std::string defaultName;

  /// @brief Create an ESP Ethernet interface
  /// @param phyNewFunction PHY generator function
  /// @param phyAddress PHY address
  /// @param resetPin reset pin
  /// @param mdcPin MDC pin
  /// @param mdioPin MDIO pin
  EspEthernet (PhyNewFunction phyNewFunction, int32_t phyAddress, int resetPin, int mdcPin, int mdioPin);
  ~EspEthernet();
  EspEthernet (const EspEthernet&) = delete;
  EspEthernet& operator= (const EspEthernet&) = delete;

  esp_err_t Lock (TickType_t timeout = portMAX_DELAY) override;
  esp_err_t Unlock() override;

  esp_err_t Initialize() override;

  esp_err_t Enable() override;
  esp_err_t Disable() override;

  bool IsEnabled() override;
  bool IsConnected() override;
  
private:
  Mutex mutex;
  esp_netif_t* netif = NULL;
  bool enabled = false, connected = false;
  PhyNewFunction phyNewFunction;
  eth_phy_config_t phyConfig = ETH_PHY_DEFAULT_CONFIG();
  eth_mac_config_t macConfig = ETH_MAC_DEFAULT_CONFIG();
  eth_esp32_emac_config_t esp32EmacConfig = ETH_ESP32_EMAC_DEFAULT_CONFIG();
  esp_eth_handle_t handle = NULL;
  esp_eth_netif_glue_handle_t netifGlueHandle = NULL;

  static void EventHandler (void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData);
};

//==============================================================================

}