#include "ethernet.h"
#include "unity.h"

#if CONFIG_ETH_USE_ESP32_EMAC

//==============================================================================

const PL::IpV4Address staticIpV4Address(192, 168, 0, 10);
const PL::IpV4Address staticIpV4Netmask(255, 255, 255, 0);
const PL::IpV4Address staticIpV4Gateway(192, 168, 0, 1);
const TickType_t connectionTimeout = 3000 / portTICK_PERIOD_MS;

//==============================================================================

void TestEthernet() {
  // Ethernet with KSZ8081 chip. Phy address: 0. Reset pin: 5. MDC pin: 23. MDIO pin: 18.
  PL::EspEthernet ethernet(esp_eth_phy_new_ksz80xx, 0, 5, 23, 18);

  TEST_ASSERT(ethernet.Initialize() == ESP_OK);
  
  TEST_ASSERT(ethernet.Enable() == ESP_OK);
  TEST_ASSERT(ethernet.IsEnabled());
  TEST_ASSERT(ethernet.EnableIpV4DhcpClient() == ESP_OK);
  TEST_ASSERT(ethernet.IsIpV4DhcpClientEnabled());
  vTaskDelay(connectionTimeout);
  TEST_ASSERT(ethernet.IsConnected());
  
  TEST_ASSERT(ethernet.GetIpV4Address().u32);
  TEST_ASSERT(ethernet.GetIpV4Netmask().u32);
  TEST_ASSERT(ethernet.GetIpV4Gateway().u32);

  PL::IpV6Address ipV6LinkLocalAddress = ethernet.GetIpV6LinkLocalAddress();
  TEST_ASSERT(ipV6LinkLocalAddress.u32[0] || ipV6LinkLocalAddress.u32[1] || ipV6LinkLocalAddress.u32[2] || ipV6LinkLocalAddress.u32[3]);

  TEST_ASSERT(ethernet.DisableIpV4DhcpClient() == ESP_OK);
  TEST_ASSERT(!ethernet.IsIpV4DhcpClientEnabled());
  TEST_ASSERT(ethernet.SetIpV4Address(staticIpV4Address) == ESP_OK);
  TEST_ASSERT(ethernet.SetIpV4Netmask(staticIpV4Netmask) == ESP_OK);
  TEST_ASSERT(ethernet.SetIpV4Gateway(staticIpV4Gateway) == ESP_OK);

  TEST_ASSERT(!ethernet.IsIpV4DhcpClientEnabled());
  TEST_ASSERT_EQUAL(staticIpV4Address.u32, ethernet.GetIpV4Address().u32);
  TEST_ASSERT_EQUAL(staticIpV4Netmask.u32, ethernet.GetIpV4Netmask().u32);
  TEST_ASSERT_EQUAL(staticIpV4Gateway.u32, ethernet.GetIpV4Gateway().u32);

  TEST_ASSERT(ethernet.Disable() == ESP_OK);
  TEST_ASSERT(!ethernet.IsEnabled());
  TEST_ASSERT(!ethernet.IsConnected());
}

#endif