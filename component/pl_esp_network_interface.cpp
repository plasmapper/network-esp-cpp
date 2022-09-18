#include "pl_esp_network_interface.h"
#include "esp_event.h"

//==============================================================================

namespace PL {

//==============================================================================

EspNetworkInterface::~EspNetworkInterface() {
  esp_event_handler_unregister (IP_EVENT, ESP_EVENT_ANY_ID, EventHandler);
}

//==============================================================================

esp_err_t EspNetworkInterface::EnableIpV4DhcpClient() {
  LockGuard lg (*this);
  if (!netif)
    return ESP_ERR_INVALID_STATE;
  return IsIpV4DhcpClientEnabled() ? ESP_OK : esp_netif_dhcpc_start (netif);
}

//==============================================================================

esp_err_t EspNetworkInterface::DisableIpV4DhcpClient() {
  LockGuard lg (*this);
  if (!netif)
    return ESP_ERR_INVALID_STATE;
  return IsIpV4DhcpClientEnabled() ? esp_netif_dhcpc_stop (netif) : ESP_OK;
}

//==============================================================================

esp_err_t EspNetworkInterface::EnableIpV6DhcpClient() {
  return ESP_ERR_NOT_SUPPORTED;
}

//==============================================================================

esp_err_t EspNetworkInterface::DisableIpV6DhcpClient() {
  return ESP_ERR_NOT_SUPPORTED;
}

//==============================================================================

bool EspNetworkInterface::IsIpV4DhcpClientEnabled() {
  LockGuard lg (*this);
  if (!netif)
    return false;
  esp_netif_dhcp_status_t status;
  return esp_netif_dhcpc_get_status (netif, &status) == ESP_OK ? (status == ESP_NETIF_DHCP_STARTED) : false;
}

//==============================================================================

bool EspNetworkInterface::IsIpV6DhcpClientEnabled() {
  LockGuard lg (*this);
  return false;  
}

//==============================================================================

IpV4Address EspNetworkInterface::GetIpV4Address() {
  LockGuard lg (*this);
  esp_netif_ip_info_t ipInfo;
  if (esp_netif_get_ip_info (netif, &ipInfo) == ESP_OK)
    return IpV4Address (ipInfo.ip.addr);
  return IpV4Address();  
}

//==============================================================================

esp_err_t EspNetworkInterface::SetIpV4Address (IpV4Address address) {
  LockGuard lg (*this);
  if (!netif)
    return ESP_ERR_INVALID_STATE;
  esp_netif_ip_info_t ipInfo;
  PL_RETURN_ON_ERROR (esp_netif_get_ip_info (netif, &ipInfo));  
  ipInfo.ip.addr = address.u32;
  return esp_netif_set_ip_info (netif, &ipInfo);
}

//==============================================================================

IpV4Address EspNetworkInterface::GetIpV4Netmask() {
  LockGuard lg (*this);
  esp_netif_ip_info_t ipInfo;
  if (esp_netif_get_ip_info (netif, &ipInfo) == ESP_OK)
    return IpV4Address (ipInfo.netmask.addr);
  return IpV4Address();  
}

//==============================================================================

esp_err_t EspNetworkInterface::SetIpV4Netmask (IpV4Address netmask) {
  LockGuard lg (*this);
  if (!netif)
    return ESP_ERR_INVALID_STATE;
  esp_netif_ip_info_t ipInfo;
  PL_RETURN_ON_ERROR (esp_netif_get_ip_info (netif, &ipInfo));  
  ipInfo.netmask.addr = netmask.u32;
  return esp_netif_set_ip_info (netif, &ipInfo);
}

//==============================================================================

IpV4Address EspNetworkInterface::GetIpV4Gateway() {
  LockGuard lg (*this);
  esp_netif_ip_info_t ipInfo;
  if (esp_netif_get_ip_info (netif, &ipInfo) == ESP_OK)
    return IpV4Address (ipInfo.gw.addr);
  return IpV4Address();  
}

//==============================================================================

esp_err_t EspNetworkInterface::SetIpV4Gateway (IpV4Address gateway) {
  LockGuard lg (*this);
  if (!netif)
    return ESP_ERR_INVALID_STATE;
  esp_netif_ip_info_t ipInfo;
  PL_RETURN_ON_ERROR (esp_netif_get_ip_info (netif, &ipInfo));  
  ipInfo.gw.addr = gateway.u32;
  return esp_netif_set_ip_info (netif, &ipInfo);
}

//==============================================================================

IpV6Address EspNetworkInterface::GetIpV6LinkLocalAddress() {
  LockGuard lg (*this);
  esp_ip6_addr_t ipInfo;
  if (esp_netif_get_ip6_linklocal (netif, &ipInfo) == ESP_OK)
    return IpV6Address (*(IpV6Address*)&ipInfo.addr);
  return IpV6Address();  
}

//==============================================================================

IpV6Address EspNetworkInterface::GetIpV6GlobalAddress() {
  LockGuard lg (*this);
  esp_ip6_addr_t ipInfo;
  if (esp_netif_get_ip6_global (netif, &ipInfo) == ESP_OK)
    return IpV6Address (*(IpV6Address*)&ipInfo.addr);
  return IpV6Address();  
}

//==============================================================================

esp_err_t EspNetworkInterface::SetIpV6GlobalAddress (IpV6Address address) {
  return ESP_ERR_NOT_SUPPORTED;
}

//==============================================================================

esp_err_t EspNetworkInterface::Initialize (esp_netif_t* netif) {
  LockGuard lg (*this);
  this->netif = netif;
  return esp_event_handler_instance_register (IP_EVENT, ESP_EVENT_ANY_ID, EventHandler, this, NULL);
}

//==============================================================================

void EspNetworkInterface::EventHandler (void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData) {
  EspNetworkInterface& espNetworkInterface = *(EspNetworkInterface*)arg;
  LockGuard lg (espNetworkInterface);

  if (eventBase == IP_EVENT) {
    if (eventID == IP_EVENT_STA_GOT_IP || eventID == IP_EVENT_ETH_GOT_IP)
      espNetworkInterface.gotIpV4AddressEvent.Generate();
    if (eventID == IP_EVENT_GOT_IP6 && (*(ip_event_got_ip6_t*)eventData).esp_netif == espNetworkInterface.netif)
      espNetworkInterface.gotIpV6AddressEvent.Generate ();
    if (eventID == IP_EVENT_STA_LOST_IP) {
      espNetworkInterface.lostIpV4AddressEvent.Generate();
      espNetworkInterface.lostIpV6AddressEvent.Generate();
    }    
  }
}

//==============================================================================

}
