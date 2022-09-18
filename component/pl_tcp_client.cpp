#include "pl_tcp_client.h"
#include "lwip/sockets.h"

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
  return mutex.Lock (timeout);
}

//==============================================================================

esp_err_t TcpClient::Unlock() {
  return mutex.Unlock();
}

//==============================================================================

esp_err_t TcpClient::Connect() {
  LockGuard lg (*this);
  if (stream->IsOpen())
    return ESP_OK;
  int sock;

  if (remoteEndpoint.address.family != NetworkAddressFamily::ipV4 && remoteEndpoint.address.family != NetworkAddressFamily::ipV6)
    return ESP_FAIL;

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
      PL_RETURN_ON_ERROR ((nagleAlgorithmEnabled ? stream->EnableNagleAlgorithm() : stream->DisableNagleAlgorithm()));
      return stream->SetReadTimeout (readTimeout);
    }
    close (sock);
  }
  return ESP_FAIL;
}

//==============================================================================

esp_err_t TcpClient::Disconnect() {
  LockGuard lg (*this);
  return stream->Close();
}

//==============================================================================

esp_err_t TcpClient::EnableNagleAlgorithm() {
  LockGuard lg (*this);
  this->nagleAlgorithmEnabled = true;
  return stream->EnableNagleAlgorithm();
}

//==============================================================================

esp_err_t TcpClient::DisableNagleAlgorithm() {
  LockGuard lg (*this);
  this->nagleAlgorithmEnabled = false;
  return stream->DisableNagleAlgorithm();
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
  return stream->SetReadTimeout(timeout);
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
  PL_RETURN_ON_ERROR (stream->Close());
  remoteEndpoint = NetworkEndpoint (address, port);
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::SetRemoteEndpoint (IpV6Address address, uint16_t port) {
  LockGuard lg (*this);
  PL_RETURN_ON_ERROR (stream->Close());
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