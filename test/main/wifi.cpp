#include "wifi.h"
#include "unity.h"

//==============================================================================

const std::string ssid = CONFIG_TEST_WIFI_SSID;
const std::string password = CONFIG_TEST_WIFI_PASSWORD;
const PL::IpV4Address staticIpV4Address (192, 168, 0, 10);
const PL::IpV4Address staticIpV4Netmask (255, 255, 255, 0);
const PL::IpV4Address staticIpV4Gateway (192, 168, 0, 1);
const TickType_t connectionTimeout = 5000 / portTICK_PERIOD_MS;

//==============================================================================

void TestWiFi() {
  PL::EspWiFiStation wifi;

  TEST_ASSERT (wifi.Initialize() == ESP_OK);
  TEST_ASSERT (wifi.SetSsid (ssid) == ESP_OK);
  TEST_ASSERT (wifi.GetSsid() == ssid);
  TEST_ASSERT (wifi.SetPassword (password) == ESP_OK);
  TEST_ASSERT (wifi.GetPassword() == password);
  
  TEST_ASSERT (wifi.Enable() == ESP_OK);
  TEST_ASSERT (wifi.IsEnabled());
  vTaskDelay (connectionTimeout);
  TEST_ASSERT (wifi.IsConnected());
  
  TEST_ASSERT (wifi.EnableIpV4DhcpClient() == ESP_OK);
  TEST_ASSERT (wifi.IsIpV4DhcpClientEnabled());
  TEST_ASSERT (wifi.GetIpV4Address().u32);
  TEST_ASSERT (wifi.GetIpV4Netmask().u32);
  TEST_ASSERT (wifi.GetIpV4Gateway().u32);

  PL::IpV6Address ipV6LinkLocalAddress = wifi.GetIpV6LinkLocalAddress();
  TEST_ASSERT (ipV6LinkLocalAddress.u32[0] || ipV6LinkLocalAddress.u32[1] || ipV6LinkLocalAddress.u32[2] || ipV6LinkLocalAddress.u32[3]);

  TEST_ASSERT (wifi.DisableIpV4DhcpClient() == ESP_OK);
  TEST_ASSERT (!wifi.IsIpV4DhcpClientEnabled());
  TEST_ASSERT (wifi.SetIpV4Address (staticIpV4Address) == ESP_OK);
  TEST_ASSERT (wifi.SetIpV4Netmask (staticIpV4Netmask) == ESP_OK);
  TEST_ASSERT (wifi.SetIpV4Gateway (staticIpV4Gateway) == ESP_OK);

  TEST_ASSERT (!wifi.IsIpV4DhcpClientEnabled());
  TEST_ASSERT_EQUAL (staticIpV4Address.u32, wifi.GetIpV4Address().u32);
  TEST_ASSERT_EQUAL (staticIpV4Netmask.u32, wifi.GetIpV4Netmask().u32);
  TEST_ASSERT_EQUAL (staticIpV4Gateway.u32, wifi.GetIpV4Gateway().u32);

  TEST_ASSERT (wifi.Disable() == ESP_OK);
  TEST_ASSERT (!wifi.IsEnabled());
  TEST_ASSERT (!wifi.IsConnected());
}