#include "pl_network_stream.h"

//==============================================================================

namespace PL {

//==============================================================================

NetworkStream::NetworkStream (int sock) : sock (sock) {
  SetReadTimeout (defaultReadTimeout);
}

//==============================================================================

NetworkStream::~NetworkStream() {
  if (sock >= 0)
    close (sock);
}

//==============================================================================

esp_err_t NetworkStream::Lock (TickType_t timeout) {
  return mutex.Lock(timeout);
}

//==============================================================================

esp_err_t NetworkStream::Unlock() {
  return mutex.Unlock();
}

//==============================================================================

esp_err_t NetworkStream::Read (void* dest, size_t size) {
  LockGuard lg (*this);
  if (sock < 0)
    return ESP_ERR_INVALID_STATE;
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

  if (!size)
    return ESP_OK;

  if (errno == EAGAIN)
    return ESP_ERR_TIMEOUT;

  Close();
  return ESP_FAIL;
}

//==============================================================================

esp_err_t NetworkStream::Write (const void* src, size_t size) {
  LockGuard lg (*this);
  if (sock < 0)
    return ESP_ERR_INVALID_STATE;
  if (!size)
    return ESP_OK;
  if (!src)
    return ESP_ERR_INVALID_ARG;
  
  if (send (sock, src, size, 0) == size)
    return ESP_OK;

  Close();
  return ESP_FAIL;
}

//==============================================================================

esp_err_t NetworkStream::Close() {
  LockGuard lg (*this);
  int s = sock;
  sock = -1;
  return (s >= 0)?((close (s) == 0)?(ESP_OK):(ESP_FAIL)):(ESP_OK);
}

//==============================================================================

esp_err_t NetworkStream::EnableNagleAlgorithm() {
  return SetSocketOption (IPPROTO_TCP, TCP_NODELAY, 0);
}

//==============================================================================

esp_err_t NetworkStream::DisableNagleAlgorithm() {
  return SetSocketOption (IPPROTO_TCP, TCP_NODELAY, 1);
}

//==============================================================================

esp_err_t NetworkStream::EnableKeepAlive() {
  return SetSocketOption (SOL_SOCKET, SO_KEEPALIVE, 1);
}

//==============================================================================

esp_err_t NetworkStream::DisableKeepAlive() {
  return SetSocketOption (SOL_SOCKET, SO_KEEPALIVE, 0);
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
  return (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) >= 0)?(ESP_OK):(ESP_FAIL);
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
  return SetSocketOption (IPPROTO_TCP, TCP_KEEPIDLE, seconds);
}

//==============================================================================

esp_err_t NetworkStream::SetKeepAliveInterval (int seconds) {
  return SetSocketOption (IPPROTO_TCP, TCP_KEEPINTVL, seconds);
}

//==============================================================================

esp_err_t NetworkStream::SetKeepAliveCount (int count) {
  return SetSocketOption (IPPROTO_TCP, TCP_KEEPCNT, count);
}

//==============================================================================

esp_err_t NetworkStream::SetSocketOption (int level, int option, int value) {
  LockGuard lg (*this);
  if (sock < 0)
    return ESP_OK;
  return (setsockopt (sock, level, option, (void*)&value, sizeof (value)) >= 0)?(ESP_OK):(ESP_FAIL);
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