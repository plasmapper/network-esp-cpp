#include "pl_network.h"
#include "esp_ota_ops.h"
#include "nvs_flash.h"
#include "esp_check.h"

//==============================================================================

const char * const TAG = "tcp_ota_server";

const int firmwareVersion = 1;
const int firmwareIsCompatibleWithHardware = true;

//==============================================================================

class OtaServer : public PL::TcpServer {
public:
  static const uint16_t defaultPort = 3232;

  OtaServer();

protected:
  esp_err_t HandleRequest (PL::NetworkStream& clientStream) override;

private:
  char firmwareData[1024];

  esp_err_t UpdateFirmware(PL::NetworkStream& clientStream, uint32_t* dataSizeToRead, const esp_partition_t* partition, esp_ota_handle_t* handle);
};

//==============================================================================

class WiFiGotIpEventHandler {
public:
  void OnGotIpV4Address (PL::NetworkInterface& wifi);
  void OnGotIpV6Address (PL::NetworkInterface& wifi);
};

//==============================================================================

PL::EspWiFiStation wifi;
const std::string wifiSsid = CONFIG_EXAMPLE_WIFI_SSID;
const std::string wifiPassword = CONFIG_EXAMPLE_WIFI_PASSWORD;
auto wiFiGotIpEventHandler = std::make_shared<WiFiGotIpEventHandler>();

OtaServer server;

//==============================================================================

esp_err_t Initialize() {
  ESP_RETURN_ON_ERROR (esp_event_loop_create_default(), TAG, "create default event loop failed");
  ESP_RETURN_ON_ERROR (esp_netif_init(), TAG, "netif init failed");
  
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_RETURN_ON_ERROR (nvs_flash_erase(), TAG, "nvs flash erase failed");
      err = nvs_flash_init();
  }
  ESP_RETURN_ON_ERROR (err, TAG, "nvs flash init failed");

  ESP_RETURN_ON_FALSE (firmwareIsCompatibleWithHardware, ESP_ERR_INVALID_VERSION, TAG, "firmware is not compatible with hardware");

  ESP_RETURN_ON_ERROR (wifi.Initialize(), TAG, "Wi-Fi initialize failed");
  ESP_RETURN_ON_ERROR (wifi.SetSsid (wifiSsid), TAG, "Wi-Fi set SSID failed");
  ESP_RETURN_ON_ERROR (wifi.SetPassword (wifiPassword), TAG, "Wi-Fi set password failed");
  wifi.gotIpV4AddressEvent.AddHandler (wiFiGotIpEventHandler, &WiFiGotIpEventHandler::OnGotIpV4Address);
  wifi.gotIpV6AddressEvent.AddHandler (wiFiGotIpEventHandler, &WiFiGotIpEventHandler::OnGotIpV6Address);

  ESP_RETURN_ON_ERROR (wifi.EnableIpV4DhcpClient(), TAG, "Wi-Fi enable DHCP client failed");
  ESP_RETURN_ON_ERROR (wifi.Enable(), TAG, "Wi-Fi enable failed");

  return ESP_OK;
}

//==============================================================================

extern "C" void app_main(void) {
  esp_err_t initializeResult = Initialize();
  const esp_partition_t* runningPartition = esp_ota_get_running_partition();
  esp_ota_img_states_t otaState;
  if (esp_ota_get_state_partition(runningPartition, &otaState) == ESP_OK && otaState == ESP_OTA_IMG_PENDING_VERIFY) {
    if (initializeResult == ESP_OK)
      esp_ota_mark_app_valid_cancel_rollback();
    else
      esp_ota_mark_app_invalid_rollback_and_reboot();
  }

  printf ("Firmware v%d\n", firmwareVersion);
  while (1) {
    vTaskDelay (1);
  }
}

//==============================================================================

OtaServer::OtaServer() : TcpServer (defaultPort) {}

//==============================================================================

esp_err_t OtaServer::HandleRequest (PL::NetworkStream& clientStream) {
  uint32_t firmwareSize = 0;
  esp_ota_handle_t updateHandle = 0;
  
  if (clientStream.GetReadableSize() < sizeof(firmwareSize))
    return ESP_OK;
  
  ESP_RETURN_ON_ERROR (clientStream.Read (&firmwareSize, sizeof(firmwareSize)), TAG, "firmware size read failed");
  printf ("Receiving new firmware (size: %lu)\n", firmwareSize);

  esp_err_t result = UpdateFirmware(clientStream, &firmwareSize, esp_ota_get_next_update_partition(NULL), &updateHandle);
  if (result != ESP_OK && result != ESP_ERR_TIMEOUT)
    clientStream.Read (NULL, firmwareSize);
  ESP_RETURN_ON_ERROR (result, TAG, "firmware update failed");

  if (updateHandle != 0)
    esp_ota_abort(updateHandle);

  return ESP_OK; 
}

//==============================================================================

esp_err_t OtaServer::UpdateFirmware(PL::NetworkStream& clientStream, uint32_t* dataSizeToRead, const esp_partition_t* partition, esp_ota_handle_t* handle) {
  ESP_RETURN_ON_ERROR (esp_ota_begin(partition, OTA_WITH_SEQUENTIAL_WRITES, handle), TAG, "OTA begin failed");
  
  uint32_t initialDataSizeToRead = *dataSizeToRead;
  int lastProgress = 0;

  while (*dataSizeToRead) {
    size_t readSize = std::min((size_t)*dataSizeToRead, sizeof(firmwareData));
    ESP_RETURN_ON_ERROR (clientStream.Read (firmwareData, readSize), TAG, "firmware data read failed");
    ESP_RETURN_ON_ERROR (esp_ota_write(*handle, (const void*)firmwareData, readSize), TAG, "OTA write failed");
    *dataSizeToRead -= readSize;
    int progress = (initialDataSizeToRead - *dataSizeToRead) * 100 / initialDataSizeToRead;
    if (progress != lastProgress && progress % 10 == 0)
      printf("%d%%\n", progress);
    lastProgress = progress;
    vTaskDelay (1);
  }

  ESP_RETURN_ON_ERROR (esp_ota_end(*handle), TAG, "OTA end failed");
  ESP_RETURN_ON_ERROR (esp_ota_set_boot_partition(partition), TAG, "OTA set partition failed");
  printf("Firmware updated.\n");
  esp_restart();
  return ESP_OK;
}

//==============================================================================

void WiFiGotIpEventHandler::OnGotIpV4Address (PL::NetworkInterface& wifi) {
  if (server.Enable() == ESP_OK)
    printf ("Listening (address: %s, port: %d)\n", wifi.GetIpV4Address().ToString().c_str(), server.GetPort());
}

//==============================================================================

void WiFiGotIpEventHandler::OnGotIpV6Address (PL::NetworkInterface& wifi) {
  if (server.Enable() == ESP_OK)
    printf ("Listening (address: %s, port: %d)\n", wifi.GetIpV6LinkLocalAddress().ToString().c_str(), server.GetPort());
}