#include "tcp.h"
#include "unity.h"
#include "esp_check.h"

//==============================================================================

static uint16_t port = 500;
const size_t maxNumberOfClients = 2;
const PL::IpV4Address ipV4Address(127, 0, 0, 1);
const PL::IpV6Address ipV6Address(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
const TickType_t readTimeout = 1000 / portTICK_PERIOD_MS;
const uint8_t dataToSend[] = {1, 2, 3, 4, 5};
const uint8_t disableDataToSend[] = {0xFE, 0, 0, 0, 0};
const uint8_t restartDataToSend[] = {0xFF, 0, 0, 0, 0};
static const char* TAG = "pl_tcp_server_test";

//==============================================================================

void TestTcp() {
  TcpServer server(port);

  TEST_ASSERT_EQUAL(port, server.GetPort());
  TEST_ASSERT(server.SetMaxNumberOfClients(maxNumberOfClients) == ESP_OK);
  TEST_ASSERT_EQUAL(maxNumberOfClients, server.GetMaxNumberOfClients());
  TEST_ASSERT(server.Enable() == ESP_OK);
  vTaskDelay(10);
  TEST_ASSERT(server.IsEnabled());

  PL::TcpClient ipV4Client(ipV4Address, port);
  TEST_ASSERT_EQUAL(PL::NetworkStream::defaultReadTimeout, ipV4Client.GetReadTimeout());
  TEST_ASSERT_EQUAL(PL::NetworkStream::defaultReadTimeout, ipV4Client.GetStream()->GetReadTimeout());
  TEST_ASSERT(ipV4Client.SetReadTimeout(readTimeout) == ESP_OK);
  TEST_ASSERT_EQUAL(readTimeout, ipV4Client.GetReadTimeout());

  TEST_ASSERT(ipV4Client.Connect() == ESP_OK);
  TEST_ASSERT(ipV4Client.IsConnected());
  TEST_ASSERT_EQUAL(readTimeout, ipV4Client.GetStream()->GetReadTimeout());
  vTaskDelay(10);
  TEST_ASSERT_EQUAL(1, server.GetClientStreams().size());

  PL::TcpClient ipV6Client(ipV6Address, port);
  TEST_ASSERT(ipV6Client.Connect() == ESP_OK);
  TEST_ASSERT(ipV6Client.IsConnected());
  vTaskDelay(10);
  auto serverStreams = server.GetClientStreams();
  TEST_ASSERT_EQUAL(2, serverStreams.size());

  TEST_ASSERT(CompareEndpoints(ipV4Client.GetLocalEndpoint(), serverStreams[0]->GetRemoteEndpoint()));
  TEST_ASSERT(CompareEndpoints(ipV4Client.GetRemoteEndpoint(), serverStreams[0]->GetLocalEndpoint()));
  TEST_ASSERT(CompareEndpoints(ipV6Client.GetLocalEndpoint(), serverStreams[1]->GetRemoteEndpoint()));
  TEST_ASSERT(CompareEndpoints(ipV6Client.GetRemoteEndpoint(), serverStreams[1]->GetLocalEndpoint()));

  uint8_t receivedData[sizeof(dataToSend)];
  TEST_ASSERT(ipV4Client.GetStream()->Write(dataToSend, sizeof(dataToSend)) == ESP_OK);
  TEST_ASSERT(ipV4Client.GetStream()->Read(receivedData, sizeof(dataToSend)) == ESP_OK);
  for (int i = 0; i < sizeof(dataToSend); i++)
    TEST_ASSERT_EQUAL(dataToSend[i], receivedData[i]);

  TEST_ASSERT(ipV6Client.GetStream()->Write(dataToSend, sizeof(dataToSend)) == ESP_OK);
  TEST_ASSERT(ipV6Client.GetStream()->Read(receivedData, sizeof(dataToSend)) == ESP_OK);
  for (int i = 0; i < sizeof(dataToSend); i++)
    TEST_ASSERT_EQUAL(dataToSend[i], receivedData[i]);

  port++;
  TEST_ASSERT(server.SetPort(port) == ESP_OK);
  TEST_ASSERT(server.IsEnabled());
  TEST_ASSERT(ipV4Client.SetRemoteEndpoint(ipV4Address, port) == ESP_OK);
  TEST_ASSERT(ipV6Client.SetRemoteEndpoint(ipV6Address, port) == ESP_OK);
  TEST_ASSERT(!ipV4Client.IsConnected());
  TEST_ASSERT(!ipV6Client.IsConnected());
  vTaskDelay(10);
  TEST_ASSERT_EQUAL(0, server.GetClientStreams().size());
  TEST_ASSERT(ipV4Client.Connect() == ESP_OK);
  TEST_ASSERT(ipV6Client.Connect() == ESP_OK);
  TEST_ASSERT(ipV4Client.IsConnected());
  TEST_ASSERT(ipV6Client.IsConnected());
  vTaskDelay(10);
  TEST_ASSERT_EQUAL(2, server.GetClientStreams().size());
  
  TEST_ASSERT(ipV4Client.Disconnect() == ESP_OK);
  TEST_ASSERT(!ipV4Client.IsConnected());
  vTaskDelay(10);
  TEST_ASSERT_EQUAL(1, server.GetClientStreams().size());
  TEST_ASSERT(ipV6Client.Disconnect() == ESP_OK);
  TEST_ASSERT(!ipV6Client.IsConnected());
  vTaskDelay(10);
  TEST_ASSERT_EQUAL(0, server.GetClientStreams().size());

  // Test server disable and restart from request
  TEST_ASSERT(ipV4Client.Connect() == ESP_OK);
  TEST_ASSERT(ipV4Client.IsConnected());
  TEST_ASSERT(ipV4Client.GetStream()->Write(disableDataToSend, sizeof(disableDataToSend)) == ESP_OK);
  vTaskDelay(10);
  TEST_ASSERT(!server.IsEnabled());

  TEST_ASSERT(server.Enable() == ESP_OK);
  TEST_ASSERT(server.IsEnabled());
  TEST_ASSERT(ipV4Client.Connect() == ESP_OK);
  TEST_ASSERT(ipV4Client.IsConnected());
  TEST_ASSERT(ipV4Client.GetStream()->Write(restartDataToSend, sizeof(restartDataToSend)) == ESP_OK);
  vTaskDelay(10);
  TEST_ASSERT(server.IsEnabled());

  TEST_ASSERT(server.Disable() == ESP_OK);
  TEST_ASSERT(!server.IsEnabled());
}

//==============================================================================

esp_err_t TcpServer::HandleRequest(PL::NetworkStream& stream) {
  uint8_t dataByte;
  while (stream.GetReadableSize()) {
    if (stream.Read(&dataByte, 1) == ESP_OK) {
      stream.Write(&dataByte, 1);
      if (dataByte == disableDataToSend[0])
        ESP_RETURN_ON_ERROR(Disable(), TAG, "server disable failed");
      if (dataByte == restartDataToSend[0]) {
        ESP_RETURN_ON_ERROR(Disable(), TAG, "server disable failed");
        ESP_RETURN_ON_ERROR(Enable(), TAG, "server enable failed");
      }
    }      
  }
  return ESP_OK;
}

//==============================================================================

bool CompareEndpoints(const PL::NetworkEndpoint& ep1, const PL::NetworkEndpoint& ep2) {
  if (ep1.address.family != ep2.address.family || ep1.port != ep2.port)
    return false;
  
  if (ep1.address.family == PL::NetworkAddressFamily::ipV4)
    return ep1.address.ipV4.u32 == ep2.address.ipV4.u32;

  if (ep1.address.family == PL::NetworkAddressFamily::ipV6)
    return ep1.address.ipV6.u32[0] == ep2.address.ipV6.u32[0] && ep1.address.ipV6.u32[1] == ep2.address.ipV6.u32[1] &&
           ep1.address.ipV6.u32[2] == ep2.address.ipV6.u32[2] && ep1.address.ipV6.u32[3] == ep2.address.ipV6.u32[3];

  return false;
}