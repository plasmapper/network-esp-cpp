#pragma once
#include "pl_network_interface.h"
#include "esp_netif.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief Internal ESP network interface class
class EspNetworkInterface : public virtual NetworkInterface {
public:
  /// @note Every derived class must call UnregisterEventHandler as the first statement of its own
  /// destructor, before any of its own members are destroyed, so that the IP_EVENT handler cannot
  /// be invoked on an object whose virtual Lock() no longer dispatches to the derived override.
  /// The base destructor aborts if this was not done.
  ~EspNetworkInterface();
  EspNetworkInterface(const EspNetworkInterface&) = delete;
  EspNetworkInterface& operator=(const EspNetworkInterface&) = delete;

  esp_err_t EnableIpV4DhcpClient() override;
  esp_err_t DisableIpV4DhcpClient() override;
  esp_err_t EnableIpV6DhcpClient() override;
  esp_err_t DisableIpV6DhcpClient() override;

  bool IsIpV4DhcpClientEnabled() override;
  bool IsIpV6DhcpClientEnabled() override;

  IpV4Address GetIpV4Address() override;
  esp_err_t SetIpV4Address(IpV4Address address) override;
  IpV4Address GetIpV4Netmask() override;
  esp_err_t SetIpV4Netmask(IpV4Address netmask) override;
  IpV4Address GetIpV4Gateway() override;
  esp_err_t SetIpV4Gateway(IpV4Address gateway) override;

  IpV6Address GetIpV6LinkLocalAddress() override;
  IpV6Address GetIpV6GlobalAddress() override;
  esp_err_t SetIpV6GlobalAddress(IpV6Address address) override;

protected:
  /// @brief Creates an internal ESP network interface
  EspNetworkInterface() {}

  /// @brief Initializes the interface
  /// @param netif ESP netif
  /// @return error code
  esp_err_t InitializeNetif(esp_netif_t* netif);

  /// @brief Unregisters the IP event handler
  /// @note Must be called as the first statement of the destructor of every derived class so
  /// that no event can be dispatched to this object after its own members start being destroyed.
  /// @return error code
  esp_err_t UnregisterEventHandler();

private:
  esp_netif_t* netif = NULL;
  esp_event_handler_instance_t eventHandlerInstance = NULL;
  bool ipV4DhcpClientEnabled = false;

  static void EventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData);
};

//==============================================================================

}