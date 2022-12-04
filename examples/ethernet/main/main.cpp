#include "pl_network.h"

//==============================================================================

class Logger {
public:
  void OnEnabled (PL::HardwareInterface& hardwareInterface);
  void OnDisabled (PL::HardwareInterface& hardwareInterface);
  void OnConnected (PL::NetworkInterface& networkInterface);
  void OnDisconnected (PL::NetworkInterface& networkInterface);
  void OnGotIpV4Address (PL::NetworkInterface& networkInterface);
  void OnGotIpV6Address (PL::NetworkInterface& networkInterface);
  void OnLostIpV4Address (PL::NetworkInterface& networkInterface);
  void OnLostIpV6Address (PL::NetworkInterface& networkInterface);
};

//==============================================================================

// Ethernet with KSZ8081 chip. Phy address: 0. Reset pin: 5. MDC pin: 23. MDIO pin: 18.
PL::EspEthernet ethernet (esp_eth_phy_new_ksz80xx, 0, 5, 23, 18);
auto logger = std::make_shared<Logger>();

//==============================================================================

extern "C" void app_main(void) {
  esp_event_loop_create_default();
  esp_netif_init();
  
  ethernet.enabledEvent.AddHandler (logger, &Logger::OnEnabled);
  ethernet.disabledEvent.AddHandler (logger, &Logger::OnDisabled);
  ethernet.connectedEvent.AddHandler (logger, &Logger::OnConnected);
  ethernet.disconnectedEvent.AddHandler (logger, &Logger::OnDisconnected);
  ethernet.gotIpV4AddressEvent.AddHandler (logger, &Logger::OnGotIpV4Address);
  ethernet.gotIpV6AddressEvent.AddHandler (logger, &Logger::OnGotIpV6Address);
  ethernet.lostIpV4AddressEvent.AddHandler (logger, &Logger::OnLostIpV4Address);
  ethernet.lostIpV6AddressEvent.AddHandler (logger, &Logger::OnLostIpV6Address);

  ethernet.Initialize();
  ethernet.EnableIpV4DhcpClient();
  ethernet.Enable();

  while (1) {
    vTaskDelay (1);
  }
}

//==============================================================================

void Logger::OnEnabled (PL::HardwareInterface& hardwareInterface) {
  printf ("%s enabled\n", hardwareInterface.GetName().c_str());
}

//==============================================================================

void Logger::OnDisabled (PL::HardwareInterface& hardwareInterface) {
  printf ("%s disabled\n", hardwareInterface.GetName().c_str());
}

//==============================================================================

void Logger::OnConnected (PL::NetworkInterface& networkInterface) {
  printf ("%s connected\n", networkInterface.GetName().c_str());
}

//==============================================================================

void Logger::OnDisconnected (PL::NetworkInterface& networkInterface) {
  printf ("%s disconnected\n", networkInterface.GetName().c_str());
}

//==============================================================================

void Logger::OnGotIpV4Address (PL::NetworkInterface& networkInterface) {
  printf ("%s got IPv4 (address: %s, netwask: %s, gateway: %s)\n",
    networkInterface.GetName().c_str(),
    networkInterface.GetIpV4Address().ToString().c_str(),
    networkInterface.GetIpV4Netmask().ToString().c_str(),
    networkInterface.GetIpV4Gateway().ToString().c_str());
}

//==============================================================================

void Logger::OnGotIpV6Address (PL::NetworkInterface& networkInterface) {
  printf ("%s got IPv6 (link-local address: %s, global address: %s)\n",
    networkInterface.GetName().c_str(), networkInterface.GetIpV6LinkLocalAddress().ToString().c_str(), networkInterface.GetIpV6GlobalAddress().ToString().c_str());
}

//==============================================================================

void Logger::OnLostIpV4Address (PL::NetworkInterface& networkInterface) {
  printf ("%s lost IPv4\n", networkInterface.GetName().c_str());
}

//==============================================================================

void Logger::OnLostIpV6Address (PL::NetworkInterface& networkInterface) {
  printf ("%s lost IPv6\n", networkInterface.GetName().c_str());
}