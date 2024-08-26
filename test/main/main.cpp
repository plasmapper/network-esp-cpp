#include "unity.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "ip_address.h"
#include "tcp.h"
#include "ethernet.h"
#include "wifi.h"

//==============================================================================

extern "C" void app_main(void) {
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_netif_init());

  UNITY_BEGIN();
  RUN_TEST(TestIpAddress);
  RUN_TEST(TestTcp);
  #if CONFIG_ETH_USE_ESP32_EMAC
  RUN_TEST(TestEthernet);
  #endif
  RUN_TEST(TestWiFi);
  UNITY_END();
}