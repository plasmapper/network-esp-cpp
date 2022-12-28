#include "pl_network_stream.h"
#include "esp_check.h"

//==============================================================================

static const char* TAG = "pl_network_stream";

//==============================================================================

namespace PL {

//==============================================================================

NetworkStream::NetworkStream (int sock) : sock (sock) {
  SetReadTimeout (defaultReadTimeout);
}

//==============================================================================

esp_err_t NetworkStream::Lock (TickType_t timeout) {
  esp_err_t error = mutex.Lock (timeout);
  if (error == ESP_OK)
    return ESP_OK;
  if (error == ESP_ERR_TIMEOUT && timeout == 0)
    return ESP_ERR_TIMEOUT;
  ESP_RETURN_ON_ERROR (error, TAG, "mutex lock failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::Unlock() {
  ESP_RETURN_ON_ERROR (mutex.Unlock(), TAG, "mutex unlock failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::Read (void* dest, size_t size) {
  LockGuard lg (*this);
  ESP_RETURN_ON_FALSE (sock >= 0, ESP_ERR_INVALID_STATE, TAG, "network stream is closed");
  if (!size)
    return ESP_OK;
 
  int res;
  if (dest) {
    for (; size && (res = recv (sock, (uint8_t*)dest, size, 0)) > 0; size -= res, dest = (uint8_t*)dest + res);
  }
  else {
    uint8_t data;
    for (; size && recv (sock, &data, 1, 0) == 1; size--);
  }

  if (!size || (errno == EAGAIN && size == SIZE_MAX))
    return ESP_OK;

  ESP_RETURN_ON_FALSE (errno != EAGAIN, ESP_ERR_TIMEOUT, TAG, "timeout");

  Close();
  ESP_RETURN_ON_ERROR (ESP_FAIL, TAG, "read failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::Write (const void* src, size_t size) {
  LockGuard lg (*this);
  ESP_RETURN_ON_FALSE (sock >= 0, ESP_ERR_INVALID_STATE, TAG, "network stream is closed");
  if (!size)
    return ESP_OK;
  ESP_RETURN_ON_FALSE (src, ESP_ERR_INVALID_ARG, TAG, "src is null");
  
  if (send (sock, src, size, 0) == size)
    return ESP_OK;

  Close();
  ESP_RETURN_ON_ERROR (ESP_FAIL, TAG, "write failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::Close() {
  LockGuard lg (*this);
  if (sock < 0)
    return ESP_OK;
  int s = sock;
  sock = -1;
  ESP_RETURN_ON_FALSE (close (s) == 0, ESP_FAIL, TAG, "socket close failed (%d)", errno);
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::EnableNagleAlgorithm() {
  ESP_RETURN_ON_ERROR (SetSocketOption (IPPROTO_TCP, TCP_NODELAY, 0), TAG, "Nagle's algorithm enable failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::DisableNagleAlgorithm() {
  ESP_RETURN_ON_ERROR (SetSocketOption (IPPROTO_TCP, TCP_NODELAY, 1), TAG, "Nagle's algorithm disable failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::EnableKeepAlive() {
  ESP_RETURN_ON_ERROR (SetSocketOption (SOL_SOCKET, SO_KEEPALIVE, 1), TAG, "keep-alive enable failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::DisableKeepAlive() {
  ESP_RETURN_ON_ERROR (SetSocketOption (SOL_SOCKET, SO_KEEPALIVE, 0), TAG, "keep-alive disable failed");
  return ESP_OK;
}

//==============================================================================

bool NetworkStream::IsOpen() {
  LockGuard lg (*this);
  return (sock >= 0);
}

//==============================================================================

size_t NetworkStream::GetReadableSize() {
  LockGuard lg (*this);
  if (sock < 0)
    return 0;
  fd_set set;
  timeval timeout = {};
  FD_ZERO (&set);
  FD_SET (sock, &set);
  bool readyForRead = select (sock + 1, &set, NULL, NULL, &timeout);
  if (readyForRead) {
    size_t dataSize = 0;
    ioctl (sock, FIONREAD, &dataSize);
    if (dataSize)
      return dataSize;
    
    Close();
  }
  return 0;
}

//==============================================================================

TickType_t NetworkStream::GetReadTimeout() {
  LockGuard lg (*this);
  return readTimeout;
}

//==============================================================================

esp_err_t NetworkStream::SetReadTimeout (TickType_t timeout) {
  LockGuard lg (*this);
  this->readTimeout = timeout;
  if (sock < 0)
    return ESP_OK;

  timeval tv = {};
  if (timeout != portMAX_DELAY) {
    uint32_t timeoutMs = timeout * portTICK_PERIOD_MS;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
  }
  ESP_RETURN_ON_FALSE (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) >= 0, ESP_FAIL, TAG, "socket option set failed (%d)", errno);
  return ESP_OK;
}

//==============================================================================

NetworkEndpoint NetworkStream::GetLocalEndpoint() {
  sockaddr_storage sockAddr;
  socklen_t sockAddrSize = sizeof (sockAddr);
  
  if (sock < 0 || getsockname (sock, (sockaddr*)&sockAddr, &sockAddrSize) != 0)
    return NetworkEndpoint();
  else
    return SockAddrToEndpoint (sockAddr);
}

//==============================================================================

NetworkEndpoint NetworkStream::GetRemoteEndpoint() {
  sockaddr_storage sockAddr;
  socklen_t sockAddrSize = sizeof (sockAddr);
  
  if (sock < 0 || getpeername (sock, (sockaddr*)&sockAddr, &sockAddrSize) != 0)
    return NetworkEndpoint();
  else
    return SockAddrToEndpoint (sockAddr);
}

//==============================================================================

esp_err_t NetworkStream::SetKeepAliveIdleTime (int seconds) {
  ESP_RETURN_ON_ERROR (SetSocketOption (IPPROTO_TCP, TCP_KEEPIDLE, seconds), TAG, "keep-alive idle time set failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::SetKeepAliveInterval (int seconds) {
  ESP_RETURN_ON_ERROR (SetSocketOption (IPPROTO_TCP, TCP_KEEPINTVL, seconds), TAG, "keep-alive interval set failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::SetKeepAliveCount (int count) {
  ESP_RETURN_ON_ERROR (SetSocketOption (IPPROTO_TCP, TCP_KEEPCNT, count), TAG, "keep-alive count set failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::SetSocketOption (int level, int option, int value) {
  LockGuard lg (*this);
  if (sock < 0)
    return ESP_OK;
  ESP_RETURN_ON_FALSE (setsockopt (sock, level, option, (void*)&value, sizeof (value)) >= 0, ESP_FAIL, TAG, "socket option set failed (%d)", errno);
  return ESP_OK;
}

//==============================================================================

NetworkEndpoint NetworkStream::SockAddrToEndpoint (sockaddr_storage& sockAddr) {
  switch (((sockaddr*)&sockAddr)->sa_family) {
    case AF_INET:
      return NetworkEndpoint (IpV4Address (((sockaddr_in*)&sockAddr)->sin_addr.s_addr), ntohs (((sockaddr_in*)&sockAddr)->sin_port));
    case AF_INET6:
      uint32_t* u32 = ((sockaddr_in6*)&sockAddr)->sin6_addr.un.u32_addr;
      if (u32[0] == 0 && u32[1] == 0 && u32[2] == 0xFFFF0000)
        return NetworkEndpoint (IpV4Address (u32[3]), ntohs (((sockaddr_in*)&sockAddr)->sin_port));
      else
        return NetworkEndpoint (IpV6Address (
          ((sockaddr_in6*)&sockAddr)->sin6_addr.un.u32_addr[0],
          ((sockaddr_in6*)&sockAddr)->sin6_addr.un.u32_addr[1],
          ((sockaddr_in6*)&sockAddr)->sin6_addr.un.u32_addr[2],
          ((sockaddr_in6*)&sockAddr)->sin6_addr.un.u32_addr[3],
          ((sockaddr_in6*)&sockAddr)->sin6_scope_id), ntohs (((sockaddr_in6*)&sockAddr)->sin6_port));
  }
  return NetworkEndpoint();
}

//==============================================================================

}