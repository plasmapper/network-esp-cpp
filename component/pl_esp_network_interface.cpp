#include "pl_esp_network_interface.h"
#include "esp_check.h"
#include "esp_event.h"

//==============================================================================

static const char* TAG = "pl_esp_network_interface";

//==============================================================================

namespace PL {

//==============================================================================

EspNetworkInterface::~EspNetworkInterface() {
  esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, EventHandler);
}

//==============================================================================

esp_err_t EspNetworkInterface::EnableIpV4DhcpClient() {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(netif, ESP_ERR_INVALID_STATE, TAG, "network interface is not initialized");
  ipV4DhcpClientEnabled = true;
  esp_netif_dhcpc_start(netif);
  return ESP_OK;
}

//==============================================================================

esp_err_t EspNetworkInterface::DisableIpV4DhcpClient() {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(netif, ESP_ERR_INVALID_STATE, TAG, "network interface is not initialized");
  ipV4DhcpClientEnabled = false;
  esp_netif_dhcpc_stop(netif);
  return ESP_OK;
}

//==============================================================================

esp_err_t EspNetworkInterface::EnableIpV6DhcpClient() {
  ESP_RETURN_ON_ERROR(ESP_ERR_NOT_SUPPORTED, TAG, "IPv6 DHCP client is not supported");
  return ESP_OK;
}

//==============================================================================

esp_err_t EspNetworkInterface::DisableIpV6DhcpClient() {
  ESP_RETURN_ON_ERROR(ESP_ERR_NOT_SUPPORTED, TAG, "IPv6 DHCP client is not supported");
  return ESP_OK;
}

//==============================================================================

bool EspNetworkInterface::IsIpV4DhcpClientEnabled() {
  return ipV4DhcpClientEnabled;
}

//==============================================================================

bool EspNetworkInterface::IsIpV6DhcpClientEnabled() {
  return false;  
}

//==============================================================================

IpV4Address EspNetworkInterface::GetIpV4Address() {
  LockGuard lg(*this);
  esp_netif_ip_info_t ipInfo;
  if (esp_netif_get_ip_info(netif, &ipInfo) == ESP_OK)
    return IpV4Address(ipInfo.ip.addr);
  return IpV4Address();  
}

//==============================================================================

esp_err_t EspNetworkInterface::SetIpV4Address(IpV4Address address) {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(netif, ESP_ERR_INVALID_STATE, TAG, "network interface is not initialized");
  esp_netif_ip_info_t ipInfo;
  ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(netif, &ipInfo), TAG, "get IP info failed");  
  ipInfo.ip.addr = address.u32;
  ESP_RETURN_ON_ERROR(esp_netif_set_ip_info(netif, &ipInfo), TAG, "set IP info failed");
  return ESP_OK;
}

//==============================================================================

IpV4Address EspNetworkInterface::GetIpV4Netmask() {
  LockGuard lg(*this);
  esp_netif_ip_info_t ipInfo;
  if (esp_netif_get_ip_info(netif, &ipInfo) == ESP_OK)
    return IpV4Address(ipInfo.netmask.addr);
  return IpV4Address();  
}

//==============================================================================

esp_err_t EspNetworkInterface::SetIpV4Netmask(IpV4Address netmask) {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(netif, ESP_ERR_INVALID_STATE, TAG, "network interface is not initialized");
  esp_netif_ip_info_t ipInfo;
  ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(netif, &ipInfo), TAG, "get IP info failed");  
  ipInfo.netmask.addr = netmask.u32;
  ESP_RETURN_ON_ERROR(esp_netif_set_ip_info(netif, &ipInfo), TAG, "set IP info failed");
  return ESP_OK;
}

//==============================================================================

IpV4Address EspNetworkInterface::GetIpV4Gateway() {
  LockGuard lg(*this);
  esp_netif_ip_info_t ipInfo;
  if (esp_netif_get_ip_info(netif, &ipInfo) == ESP_OK)
    return IpV4Address(ipInfo.gw.addr);
  return IpV4Address();  
}

//==============================================================================

esp_err_t EspNetworkInterface::SetIpV4Gateway(IpV4Address gateway) {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(netif, ESP_ERR_INVALID_STATE, TAG, "network interface is not initialized");
  esp_netif_ip_info_t ipInfo;
  ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(netif, &ipInfo), TAG, "get IP info failed");  
  ipInfo.gw.addr = gateway.u32;
  ESP_RETURN_ON_ERROR(esp_netif_set_ip_info(netif, &ipInfo), TAG, "set IP info failed");
  return ESP_OK;
}

//==============================================================================

IpV6Address EspNetworkInterface::GetIpV6LinkLocalAddress() {
  LockGuard lg(*this);
  esp_ip6_addr_t ipInfo;
  if (esp_netif_get_ip6_linklocal(netif, &ipInfo) == ESP_OK)
    return IpV6Address(*(IpV6Address*)&ipInfo.addr);
  return IpV6Address();  
}

//==============================================================================

IpV6Address EspNetworkInterface::GetIpV6GlobalAddress() {
  LockGuard lg(*this);
  esp_ip6_addr_t ipInfo;
  if(esp_netif_get_ip6_global(netif, &ipInfo) == ESP_OK)
    return IpV6Address(*(IpV6Address*)&ipInfo.addr);
  return IpV6Address();  
}

//==============================================================================

esp_err_t EspNetworkInterface::SetIpV6GlobalAddress(IpV6Address address) {
  ESP_RETURN_ON_ERROR(ESP_ERR_NOT_SUPPORTED, TAG, "set IPv6 global address is not supported");
  return ESP_OK;
}

//==============================================================================

esp_err_t EspNetworkInterface::InitializeNetif(esp_netif_t* netif) {
  LockGuard lg(*this);
  this->netif = netif;
  ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, EventHandler, this, NULL), TAG, "event handler instance register failed");
  return ESP_OK;
}

//==============================================================================

void EspNetworkInterface::EventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData) {
  EspNetworkInterface& espNetworkInterface = *(EspNetworkInterface*)arg;

  if (eventBase == IP_EVENT) {
    if (eventID == IP_EVENT_STA_GOT_IP || eventID == IP_EVENT_ETH_GOT_IP)
      espNetworkInterface.gotIpV4AddressEvent.Generate();
    if (eventID == IP_EVENT_GOT_IP6 &&(*(ip_event_got_ip6_t*)eventData).esp_netif == espNetworkInterface.netif)
      espNetworkInterface.gotIpV6AddressEvent.Generate();
    if (eventID == IP_EVENT_STA_LOST_IP) {
      espNetworkInterface.lostIpV4AddressEvent.Generate();
      espNetworkInterface.lostIpV6AddressEvent.Generate();
    }    
  }
}

//==============================================================================

}
