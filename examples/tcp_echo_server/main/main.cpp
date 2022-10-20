#include "pl_network.h"

//==============================================================================

class EchoServer : public PL::TcpServer {
public:
  static const uint16_t defaultPort = 7;

  EchoServer();
  esp_err_t HandleRequest (PL::NetworkStream& clientStream) override;
};

//==============================================================================

class WiFiGotIpEventHandler {
public:
  void OnGotIpV4Address (PL::NetworkInterface& wifi);
  void OnGotIpV6Address (PL::NetworkInterface& wifi);
};

//==============================================================================

class ClientEventHandler {
public:
  void OnClientConnected (PL::TcpServer& server, PL::NetworkStream& client);
  void OnClientDisconnected (PL::TcpServer& server, PL::NetworkStream& client);
};

//==============================================================================

PL::EspWiFiStation wifi;
const std::string wifiSsid = CONFIG_EXAMPLE_WIFI_SSID;
const std::string wifiPassword = CONFIG_EXAMPLE_WIFI_PASSWORD;
auto wiFiGotIpEventHandler = std::make_shared<WiFiGotIpEventHandler>();

EchoServer server;
auto clientEventHandler = std::make_shared<ClientEventHandler>();

//==============================================================================

extern "C" void app_main(void) {
  esp_event_loop_create_default();
  esp_netif_init();

  wifi.Initialize();
  wifi.SetSsid (wifiSsid);
  wifi.SetPassword (wifiPassword);
  wifi.gotIpV4AddressEvent.AddHandler (wiFiGotIpEventHandler, &WiFiGotIpEventHandler::OnGotIpV4Address);
  wifi.gotIpV6AddressEvent.AddHandler (wiFiGotIpEventHandler, &WiFiGotIpEventHandler::OnGotIpV6Address);

  server.clientConnectedEvent.AddHandler (clientEventHandler, &ClientEventHandler::OnClientConnected);
  server.clientDisconnectedEvent.AddHandler (clientEventHandler, &ClientEventHandler::OnClientDisconnected);

  wifi.Enable();

  while (1) {
    vTaskDelay (1);
  }
}

//==============================================================================

EchoServer::EchoServer() : TcpServer (defaultPort) {}

//==============================================================================

esp_err_t EchoServer::HandleRequest (PL::NetworkStream& clientStream) {
  uint8_t dataByte;
  while (clientStream.GetReadableSize()) {
    if (clientStream.Read (&dataByte, 1) == ESP_OK)
      clientStream.Write(&dataByte, 1);
  }  
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

//==============================================================================

void ClientEventHandler::OnClientConnected (PL::TcpServer& server, PL::NetworkStream& client) {
  printf ("Client connected (address: %s)\n", client.GetRemoteEndpoint().address.ToString().c_str());
}

//==============================================================================

void ClientEventHandler::OnClientDisconnected (PL::TcpServer& server, PL::NetworkStream& client) {
  printf ("Client disconnected\n");
}

//==============================================================================
