#include "pl_esp_network_interface.h"
#include "esp_check.h"
#include "esp_event.h"

//==============================================================================

static const char* TAG = "pl_esp_network_interface";

//==============================================================================

namespace PL {

//==============================================================================

EspNetworkInterface::~EspNetworkInterface() {
  if (eventHandlerInstance) {
    ESP_LOGE(TAG, "UnregisterEventHandler was not called by the derived class destructor");
    abort();
  }
}

//==============================================================================

esp_err_t EspNetworkInterface::EnableIpV4DhcpClient() {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(netif, ESP_ERR_INVALID_STATE, TAG, "network interface is not initialized");
  esp_err_t error = esp_netif_dhcpc_start(netif);
  ESP_RETURN_ON_FALSE(error == ESP_OK || error == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED, error, TAG, "DHCP client start failed");
  ipV4DhcpClientEnabled = true;
  return ESP_OK;
}

//==============================================================================

esp_err_t EspNetworkInterface::DisableIpV4DhcpClient() {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(netif, ESP_ERR_INVALID_STATE, TAG, "network interface is not initialized");
  esp_err_t error = esp_netif_dhcpc_stop(netif);
  ESP_RETURN_ON_FALSE(error == ESP_OK || error == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED, error, TAG, "DHCP client stop failed");
  ipV4DhcpClientEnabled = false;
  return ESP_OK;
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
  LockGuard lg(*this);
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
  if (ipInfo.ip.addr == address.u32)
    return ESP_OK;
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
  if (ipInfo.netmask.addr == netmask.u32)
    return ESP_OK;
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
  if (ipInfo.gw.addr == gateway.u32)
    return ESP_OK;
  ipInfo.gw.addr = gateway.u32;
  ESP_RETURN_ON_ERROR(esp_netif_set_ip_info(netif, &ipInfo), TAG, "set IP info failed");
  return ESP_OK;
}

//==============================================================================

IpV6Address EspNetworkInterface::GetIpV6LinkLocalAddress() {
  LockGuard lg(*this);
  esp_ip6_addr_t ipInfo;
  if (esp_netif_get_ip6_linklocal(netif, &ipInfo) == ESP_OK)
    return IpV6Address(ipInfo.addr[0], ipInfo.addr[1], ipInfo.addr[2], ipInfo.addr[3], ipInfo.zone);
  return IpV6Address();
}

//==============================================================================

IpV6Address EspNetworkInterface::GetIpV6GlobalAddress() {
  LockGuard lg(*this);
  esp_ip6_addr_t ipInfo;
  if(esp_netif_get_ip6_global(netif, &ipInfo) == ESP_OK)
    return IpV6Address(ipInfo.addr[0], ipInfo.addr[1], ipInfo.addr[2], ipInfo.addr[3], ipInfo.zone);
  return IpV6Address();
}

//==============================================================================

esp_err_t EspNetworkInterface::SetIpV6GlobalAddress(IpV6Address address) {
  return ESP_ERR_NOT_SUPPORTED;
}

//==============================================================================

esp_err_t EspNetworkInterface::InitializeNetif(esp_netif_t* netif) {
  LockGuard lg(*this);
  ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, EventHandler, this, &eventHandlerInstance), TAG, "event handler instance register failed");
  this->netif = netif;
  return ESP_OK;
}

//==============================================================================

esp_err_t EspNetworkInterface::UnregisterEventHandler() {
  if (!eventHandlerInstance)
    return ESP_OK;
  ESP_RETURN_ON_ERROR(esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, eventHandlerInstance), TAG, "event handler instance unregister failed");
  eventHandlerInstance = NULL;
  return ESP_OK;
}

//==============================================================================

void EspNetworkInterface::EventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData) {
  EspNetworkInterface& espNetworkInterface = *(EspNetworkInterface*)arg;

  if (eventBase == IP_EVENT) {
    esp_netif_t* netif;
    {
      LockGuard lg(espNetworkInterface);
      netif = espNetworkInterface.netif;
    }

    if (eventID == IP_EVENT_STA_GOT_IP || eventID == IP_EVENT_ETH_GOT_IP) {
      if ((*(ip_event_got_ip_t*)eventData).esp_netif == netif)
        espNetworkInterface.gotIpV4AddressEvent.Generate();
    }
    if (eventID == IP_EVENT_GOT_IP6) {
      if ((*(ip_event_got_ip6_t*)eventData).esp_netif == netif)
        espNetworkInterface.gotIpV6AddressEvent.Generate();
    }
    if (eventID == IP_EVENT_STA_LOST_IP || eventID == IP_EVENT_ETH_LOST_IP) {
      if ((*(ip_event_got_ip_t*)eventData).esp_netif == netif) {
        espNetworkInterface.lostIpV4AddressEvent.Generate();
        espNetworkInterface.lostIpV6AddressEvent.Generate();
      }
    }
  }
}

//==============================================================================

}
