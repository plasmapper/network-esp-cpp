#include "pl_tcp_client.h"
#include "lwip/sockets.h"
#include "esp_check.h"
#include <fcntl.h>

//==============================================================================

static const char* TAG = "pl_tcp_client";

//==============================================================================

namespace PL {

//==============================================================================

TcpClient::TcpClient(IpV4Address address, uint16_t port) : remoteEndpoint(address, port), stream(std::make_shared<NetworkStream>()) {}

//==============================================================================

TcpClient::TcpClient(IpV6Address address, uint16_t port) : remoteEndpoint(address, port), stream(std::make_shared<NetworkStream>()) {}

//==============================================================================

TcpClient::~TcpClient() {
  stream->Close();
}

//==============================================================================

esp_err_t TcpClient::Lock(TickType_t timeout) {
  esp_err_t error = mutex.Lock(timeout);
  if (error != ESP_OK && (error != ESP_ERR_TIMEOUT || timeout != 0))
    ESP_LOGE(TAG, "mutex lock failed");
  return error;
}

//==============================================================================

esp_err_t TcpClient::Unlock() {
  ESP_RETURN_ON_ERROR(mutex.Unlock(), TAG, "mutex unlock failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::Connect() {
  LockGuard lg(*this);
  if (stream->IsOpen())
    return ESP_OK;

  int addressFamily = remoteEndpoint.address.family == NetworkAddressFamily::ipV4 ? AF_INET : AF_INET6;
  int sock = socket(addressFamily, SOCK_STREAM, IPPROTO_TCP);
  ESP_RETURN_ON_FALSE(sock >= 0, ESP_FAIL, TAG, "socket create failed (%d)", errno);

  int socketFlags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, socketFlags | O_NONBLOCK);

  bool connected = false;
  int connectErrno = 0;
  if (remoteEndpoint.address.family == NetworkAddressFamily::ipV4) {
    sockaddr_in sockAddr = {};
    sockAddr.sin_family = addressFamily;
    sockAddr.sin_addr.s_addr = remoteEndpoint.address.ipV4.u32;
    sockAddr.sin_port = htons(remoteEndpoint.port);
    connected = (connect(sock, (sockaddr*)&sockAddr, sizeof(sockAddr)) == 0);
    connectErrno = errno;
  }
  else {
    sockaddr_in6 sockAddr = {};
    sockAddr.sin6_family = addressFamily;
    ((uint32_t*)&sockAddr.sin6_addr)[0] = remoteEndpoint.address.ipV6.u32[0];
    ((uint32_t*)&sockAddr.sin6_addr)[1] = remoteEndpoint.address.ipV6.u32[1];
    ((uint32_t*)&sockAddr.sin6_addr)[2] = remoteEndpoint.address.ipV6.u32[2];
    ((uint32_t*)&sockAddr.sin6_addr)[3] = remoteEndpoint.address.ipV6.u32[3];
    sockAddr.sin6_scope_id = remoteEndpoint.address.ipV6.zoneId;
    sockAddr.sin6_port = htons(remoteEndpoint.port);
    connected = (connect(sock, (sockaddr*)&sockAddr, sizeof(sockAddr)) == 0);
    connectErrno = errno;
  }

  // A non-blocking connect that has not failed immediately is in progress; wait for the socket
  // to become writable (or for the connect timeout to expire) and read the real outcome from SO_ERROR.
  if (!connected && connectErrno == EINPROGRESS) {
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sock, &writeSet);

    timeval tv = {};
    timeval* tvPtr = &tv;
    if (connectTimeout == portMAX_DELAY)
      tvPtr = NULL;
    else {
      uint32_t timeoutMs = connectTimeout * portTICK_PERIOD_MS;
      tv.tv_sec = timeoutMs / 1000;
      tv.tv_usec = (timeoutMs % 1000) * 1000;
    }

    int selectResult = select(sock + 1, NULL, &writeSet, NULL, tvPtr);
    if (selectResult > 0) {
      int socketError = 0;
      socklen_t socketErrorSize = sizeof(socketError);
      if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &socketError, &socketErrorSize) != 0)
        connectErrno = errno;
      else if (socketError == 0)
        connected = true;
      else
        connectErrno = socketError;
    }
    else if (selectResult == 0)
      connectErrno = ETIMEDOUT;
    else
      connectErrno = errno;
  }

  fcntl(sock, F_SETFL, socketFlags);

  if (!connected) {
    close(sock);
    ESP_RETURN_ON_ERROR(ESP_FAIL, TAG, "socket connect failed (%d)", connectErrno);
  }

  stream = std::make_shared<NetworkStream>(sock);
  ESP_RETURN_ON_ERROR((nagleAlgorithmEnabled ? stream->EnableNagleAlgorithm() : stream->DisableNagleAlgorithm()), TAG, "set Nagle's algorithm failed");
  ESP_RETURN_ON_ERROR(stream->SetReadTimeout(readTimeout), TAG, "set read timeout failed");
  ESP_RETURN_ON_ERROR(stream->SetWriteTimeout(writeTimeout), TAG, "set write timeout failed");
  return ESP_OK;
}

//==============================================================================

TickType_t TcpClient::GetConnectTimeout() {
  LockGuard lg(*this);
  return connectTimeout;
}

//==============================================================================

esp_err_t TcpClient::SetConnectTimeout(TickType_t timeout) {
  LockGuard lg(*this);
  connectTimeout = timeout;
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::Disconnect() {
  LockGuard lg(*this);
  ESP_RETURN_ON_ERROR(stream->Close(), TAG, "stream close failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::EnableNagleAlgorithm() {
  LockGuard lg(*this);
  this->nagleAlgorithmEnabled = true;
  ESP_RETURN_ON_ERROR(stream->EnableNagleAlgorithm(), TAG, "Nagle's algorithm enable failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::DisableNagleAlgorithm() {
  LockGuard lg(*this);
  this->nagleAlgorithmEnabled = false;
  ESP_RETURN_ON_ERROR(stream->DisableNagleAlgorithm(), TAG, "Nagle's algorithm disable failed");
  return ESP_OK;
}

//==============================================================================

bool TcpClient::IsConnected() {
  LockGuard lg(*this);
  return stream->IsOpen();
}

//==============================================================================

TickType_t TcpClient::GetReadTimeout() {
  LockGuard lg(*this);
  return readTimeout;
}

//==============================================================================

esp_err_t TcpClient::SetReadTimeout(TickType_t timeout) {
  LockGuard lg(*this);
  this->readTimeout = timeout;
  ESP_RETURN_ON_ERROR(stream->SetReadTimeout(timeout), TAG, "stream set read timeout failed");
  return ESP_OK;
}

//==============================================================================

TickType_t TcpClient::GetWriteTimeout() {
  LockGuard lg(*this);
  return writeTimeout;
}

//==============================================================================

esp_err_t TcpClient::SetWriteTimeout(TickType_t timeout) {
  LockGuard lg(*this);
  this->writeTimeout = timeout;
  ESP_RETURN_ON_ERROR(stream->SetWriteTimeout(timeout), TAG, "stream set write timeout failed");
  return ESP_OK;
}

//==============================================================================

NetworkEndpoint TcpClient::GetLocalEndpoint() {
  LockGuard lg(*this);
  return stream->GetLocalEndpoint();
}

//==============================================================================

NetworkEndpoint TcpClient::GetRemoteEndpoint() {
  LockGuard lg(*this);
  return remoteEndpoint;
}

//==============================================================================

esp_err_t TcpClient::SetRemoteEndpoint(IpV4Address address, uint16_t port) {
  LockGuard lg(*this);
  ESP_RETURN_ON_ERROR(stream->Close(), TAG, "stream close failed");
  remoteEndpoint = NetworkEndpoint(address, port);
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpClient::SetRemoteEndpoint(IpV6Address address, uint16_t port) {
  LockGuard lg(*this);
  ESP_RETURN_ON_ERROR(stream->Close(), TAG, "stream close failed");
  remoteEndpoint = NetworkEndpoint(address, port);
  return ESP_OK;
}

//==============================================================================

std::shared_ptr<NetworkStream> TcpClient::GetStream() {
  LockGuard lg(*this);
  return stream;
}

//==============================================================================

}