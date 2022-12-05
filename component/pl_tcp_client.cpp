#include "pl_tcp_client.h"
#include "lwip/sockets.h"
#include "esp_check.h"

//==============================================================================

static const char* TAG = "pl_tcp_client";

//==============================================================================

namespace PL {

//==============================================================================

TcpClient::TcpClient (IpV4Address address, uint16_t port) : remoteEndpoint (address, port), stream (std::make_shared<NetworkStream>()) {}

//==============================================================================

TcpClient::TcpClient (IpV6Address address, uint16_t port) : remoteEndpoint (address, port), stream (std::make_shared<NetworkStream>()) {}

//==============================================================================

TcpClient::~TcpClient() {
  stream->Close();
}

//==============================================================================

esp_err_t TcpClient::Lock (TickType_t timeout) {
  esp_err_t error = mutex.Lock (timeout);
  if (error == ESP_OK)
    return ESP_OK;
  if (error == ESP_ERR_TIMEOUT && timeout == 0)
    return ESP_ERR_TIMEOUT;
  ESP_RETURN_ON_ERROR (error, TAG, "mutex lock failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::Unlock() {
  ESP_RETURN_ON_ERROR (mutex.Unlock(), TAG, "mutex unlock failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::Connect() {
  LockGuard lg (*this);
  if (stream->IsOpen())
    return ESP_OK;
  int sock;

  int addressFamily = remoteEndpoint.address.family == NetworkAddressFamily::ipV4 ? AF_INET : AF_INET6;
  if ((sock = socket (addressFamily, SOCK_STREAM, IPPROTO_TCP)) >= 0) {
    bool connected = false;

    if (remoteEndpoint.address.family == NetworkAddressFamily::ipV4) {
      sockaddr_in sockAddr = {};
      sockAddr.sin_family = addressFamily;
      sockAddr.sin_addr.s_addr = remoteEndpoint.address.ipV4.u32;
      sockAddr.sin_port = htons (remoteEndpoint.port);
      connected = (connect (sock, (sockaddr*)&sockAddr, sizeof (sockAddr)) == 0);
    }
    else {
      sockaddr_in6 sockAddr;
      sockAddr.sin6_family = addressFamily;
      ((uint32_t*)&sockAddr.sin6_addr)[0] = remoteEndpoint.address.ipV6.u32[0];
      ((uint32_t*)&sockAddr.sin6_addr)[1] = remoteEndpoint.address.ipV6.u32[1];
      ((uint32_t*)&sockAddr.sin6_addr)[2] = remoteEndpoint.address.ipV6.u32[2];
      ((uint32_t*)&sockAddr.sin6_addr)[3] = remoteEndpoint.address.ipV6.u32[3];
      sockAddr.sin6_scope_id = remoteEndpoint.address.ipV6.zoneId;
      sockAddr.sin6_port = htons (remoteEndpoint.port);
      connected = (connect (sock, (sockaddr*)&sockAddr, sizeof (sockAddr)) == 0);
    }

    if (connected) {
      stream = std::make_shared<NetworkStream>(sock);
      ESP_RETURN_ON_ERROR ((nagleAlgorithmEnabled ? stream->EnableNagleAlgorithm() : stream->DisableNagleAlgorithm()), TAG, "Nagle's algorithm set failed");
      ESP_RETURN_ON_ERROR (stream->SetReadTimeout (readTimeout), TAG, "read timeout set failed");
      return ESP_OK;
    }
    close (sock);
  }
  ESP_RETURN_ON_ERROR (ESP_FAIL, TAG, "socket create failed (%d)", errno);
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::Disconnect() {
  LockGuard lg (*this);
  ESP_RETURN_ON_ERROR (stream->Close(), TAG, "stream close failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::EnableNagleAlgorithm() {
  LockGuard lg (*this);
  this->nagleAlgorithmEnabled = true;
  ESP_RETURN_ON_ERROR (stream->EnableNagleAlgorithm(), TAG, "Nagle's algorithm enable failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::DisableNagleAlgorithm() {
  LockGuard lg (*this);
  this->nagleAlgorithmEnabled = false;
  ESP_RETURN_ON_ERROR (stream->DisableNagleAlgorithm(), TAG, "Nagle's algorithm disable failed");
  return ESP_OK;
}

//==============================================================================

bool TcpClient::IsConnected() {
  LockGuard lg (*this);
  return stream->IsOpen();
}

//==============================================================================

TickType_t TcpClient::GetReadTimeout() {
  LockGuard lg (*this);
  return readTimeout;
}

//==============================================================================

esp_err_t TcpClient::SetReadTimeout (TickType_t timeout) {
  LockGuard lg (*this);
  this->readTimeout = timeout;
  ESP_RETURN_ON_ERROR (stream->SetReadTimeout(timeout), TAG, "stream read timeout set failed");
  return ESP_OK;
}

//==============================================================================

NetworkEndpoint TcpClient::GetLocalEndpoint() {
  LockGuard lg (*this);
  return stream->GetLocalEndpoint();
}

//==============================================================================

NetworkEndpoint TcpClient::GetRemoteEndpoint() {
  LockGuard lg (*this);
  return remoteEndpoint;
}

//==============================================================================

esp_err_t TcpClient::SetRemoteEndpoint (IpV4Address address, uint16_t port) {
  LockGuard lg (*this);
  ESP_RETURN_ON_ERROR (stream->Close(), TAG, "stream close failed");
  remoteEndpoint = NetworkEndpoint (address, port);
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::SetRemoteEndpoint (IpV6Address address, uint16_t port) {
  LockGuard lg (*this);
  ESP_RETURN_ON_ERROR (stream->Close(), TAG, "stream close failed");
  remoteEndpoint = NetworkEndpoint (address, port);
  return ESP_OK;
}

//==============================================================================

std::shared_ptr<NetworkStream> TcpClient::GetStream() {
  LockGuard lg (*this);
  return stream;
}

//==============================================================================

}