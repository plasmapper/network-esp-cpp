#include "pl_network_stream.h"
#include "esp_check.h"

//==============================================================================

static const char* TAG = "pl_network_stream";

//==============================================================================

namespace PL {

//==============================================================================

NetworkStream::NetworkStream(int sock) : sock(sock) {
  SetReadTimeout(defaultReadTimeout);
  SetWriteTimeout(defaultWriteTimeout);
}

//==============================================================================

esp_err_t NetworkStream::Lock(TickType_t timeout) {
  esp_err_t error = mutex.Lock(timeout);
  if (error != ESP_OK && (error != ESP_ERR_TIMEOUT || timeout != 0))
    ESP_LOGE(TAG, "mutex lock failed");
  return error;
}

//==============================================================================

esp_err_t NetworkStream::Unlock() {
  ESP_RETURN_ON_ERROR(mutex.Unlock(), TAG, "mutex unlock failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::Read(void* dest, size_t size) {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(sock >= 0, ESP_ERR_INVALID_STATE, TAG, "network stream is closed");
  if (!size)
    return ESP_OK;
 
  TimeOut_t xTimeOut;
  vTaskSetTimeOutState(&xTimeOut);
  TickType_t remainingTimeout = readTimeout;

  int res = 0;
  bool timedOut = false;
  do {
    if (dest) {
      res = recv(sock, (uint8_t*)dest, size, 0);
      if (res > 0) {
        size -= res;
        dest = (uint8_t*)dest + res;
      }
    }
    else {
      constexpr size_t discardBufferSize = 64;
      uint8_t discardBuffer[discardBufferSize];
      res = recv(sock, discardBuffer, std::min(size, discardBufferSize), 0);
      if (res > 0)
        size -= res;
    }

    // res = 0 means the peer closed the connection
    if (size && (res > 0 || (res < 0 && errno == EAGAIN)))
      timedOut = xTaskCheckForTimeOut(&xTimeOut, &remainingTimeout) != pdFALSE;
  } while (size && (res > 0 || (res < 0 && errno == EAGAIN)) && !timedOut);

  if (!size)
    return ESP_OK;

  ESP_RETURN_ON_FALSE(!timedOut, ESP_ERR_TIMEOUT, TAG, "timeout");

  Close();
  ESP_RETURN_ON_ERROR(ESP_FAIL, TAG, "read failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::Write(const void* src, size_t size) {
  LockGuard lg(*this);
  ESP_RETURN_ON_FALSE(sock >= 0, ESP_ERR_INVALID_STATE, TAG, "network stream is closed");
  if (!size)
    return ESP_OK;
  ESP_RETURN_ON_FALSE(src, ESP_ERR_INVALID_ARG, TAG, "src is null");

  TimeOut_t xTimeOut;
  vTaskSetTimeOutState(&xTimeOut);
  TickType_t remainingTimeout = writeTimeout;

  int res;
  bool timedOut = false;
  do {
    res = send(sock, src, size, 0);
    if (res > 0) {
      size -= res;
      src = (const uint8_t*)src + res;
    }

    if (size && (res > 0 || (res < 0 && errno == EAGAIN)))
      timedOut = xTaskCheckForTimeOut(&xTimeOut, &remainingTimeout) != pdFALSE;
  } while (size && (res > 0 || (res < 0 && errno == EAGAIN)) && !timedOut);

  if (!size)
    return ESP_OK;

  ESP_RETURN_ON_FALSE(!timedOut, ESP_ERR_TIMEOUT, TAG, "timeout");

  Close();
  ESP_RETURN_ON_ERROR(ESP_FAIL, TAG, "write failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::Close() {
  LockGuard lg(*this);
  if (sock < 0)
    return ESP_OK;
  int s = sock;
  sock = -1;
  ESP_RETURN_ON_FALSE(close(s) == 0, ESP_FAIL, TAG, "socket close failed (%d)", errno);
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::EnableNagleAlgorithm() {
  ESP_RETURN_ON_ERROR(SetSocketOption(IPPROTO_TCP, TCP_NODELAY, 0), TAG, "Nagle's algorithm enable failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::DisableNagleAlgorithm() {
  ESP_RETURN_ON_ERROR(SetSocketOption(IPPROTO_TCP, TCP_NODELAY, 1), TAG, "Nagle's algorithm disable failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::EnableKeepAlive() {
  ESP_RETURN_ON_ERROR(SetSocketOption(SOL_SOCKET, SO_KEEPALIVE, 1), TAG, "keep-alive enable failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::DisableKeepAlive() {
  ESP_RETURN_ON_ERROR(SetSocketOption(SOL_SOCKET, SO_KEEPALIVE, 0), TAG, "keep-alive disable failed");
  return ESP_OK;
}

//==============================================================================

bool NetworkStream::IsOpen() {
  LockGuard lg(*this);
  return (sock >= 0);
}

//==============================================================================

size_t NetworkStream::GetReadableSize() {
  LockGuard lg(*this);
  if (sock < 0)
    return 0;
  fd_set set;
  timeval timeout = {};
  FD_ZERO(&set);
  FD_SET(sock, &set);
  bool readyForRead = select(sock + 1, &set, NULL, NULL, &timeout) > 0;
  if (readyForRead) {
    size_t dataSize = 0;
    ioctl(sock, FIONREAD, &dataSize);
    if (dataSize)
      return dataSize;
    
    Close();
  }
  return 0;
}

//==============================================================================

TickType_t NetworkStream::GetReadTimeout() {
  LockGuard lg(*this);
  return readTimeout;
}

//==============================================================================

esp_err_t NetworkStream::SetReadTimeout(TickType_t timeout) {
  LockGuard lg(*this);
  this->readTimeout = timeout;
  if (sock < 0)
    return ESP_OK;

  // lwIP treats a zero timeval as "no timeout" (block forever)
  // 1 ms is the smallest value that survives lwIP's tv_sec*1000 + tv_usec/1000 conversion
  timeval tv = {};
  if (timeout == 0)
    tv.tv_usec = 1000;
  else if (timeout != portMAX_DELAY) {
    uint32_t timeoutMs = timeout * portTICK_PERIOD_MS;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
  }
  ESP_RETURN_ON_FALSE(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) >= 0, ESP_FAIL, TAG, "set socket option failed (%d)", errno);
  return ESP_OK;
}

//==============================================================================

TickType_t NetworkStream::GetWriteTimeout() {
  LockGuard lg(*this);
  return writeTimeout;
}

//==============================================================================

esp_err_t NetworkStream::SetWriteTimeout(TickType_t timeout) {
  LockGuard lg(*this);
  this->writeTimeout = timeout;
  if (sock < 0)
    return ESP_OK;

  // lwIP treats a zero timeval as "no timeout" (block forever)
  // 1 ms is the smallest value that survives lwIP's tv_sec*1000 + tv_usec/1000 conversion
  timeval tv = {};
  if (timeout == 0)
    tv.tv_usec = 1000;
  else if (timeout != portMAX_DELAY) {
    uint32_t timeoutMs = timeout * portTICK_PERIOD_MS;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
  }
  ESP_RETURN_ON_FALSE(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) >= 0, ESP_FAIL, TAG, "set socket option failed (%d)", errno);
  return ESP_OK;
}

//==============================================================================

NetworkEndpoint NetworkStream::GetLocalEndpoint() {
  LockGuard lg(*this);
  sockaddr_storage sockAddr;
  socklen_t sockAddrSize = sizeof(sockAddr);
  
  if (sock < 0 || getsockname(sock, (sockaddr*)&sockAddr, &sockAddrSize) != 0)
    return NetworkEndpoint();
  else
    return SockAddrToEndpoint(sockAddr);
}

//==============================================================================

NetworkEndpoint NetworkStream::GetRemoteEndpoint() {
  LockGuard lg(*this);
  sockaddr_storage sockAddr;
  socklen_t sockAddrSize = sizeof(sockAddr);
  
  if (sock < 0 || getpeername(sock, (sockaddr*)&sockAddr, &sockAddrSize) != 0)
    return NetworkEndpoint();
  else
    return SockAddrToEndpoint(sockAddr);
}

//==============================================================================

esp_err_t NetworkStream::SetKeepAliveIdleTime(int seconds) {
  ESP_RETURN_ON_ERROR(SetSocketOption(IPPROTO_TCP, TCP_KEEPIDLE, seconds), TAG, "set keep-alive idle time failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::SetKeepAliveInterval(int seconds) {
  ESP_RETURN_ON_ERROR(SetSocketOption(IPPROTO_TCP, TCP_KEEPINTVL, seconds), TAG, "set keep-alive interval failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::SetKeepAliveCount(int count) {
  ESP_RETURN_ON_ERROR(SetSocketOption(IPPROTO_TCP, TCP_KEEPCNT, count), TAG, "set keep-alive count failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t NetworkStream::SetSocketOption(int level, int option, int value) {
  LockGuard lg(*this);
  if (sock < 0)
    return ESP_OK;
  ESP_RETURN_ON_FALSE(setsockopt(sock, level, option, (void*)&value, sizeof(value)) >= 0, ESP_FAIL, TAG, "set socket option failed (%d)", errno);
  return ESP_OK;
}

//==============================================================================

NetworkEndpoint NetworkStream::SockAddrToEndpoint(sockaddr_storage& sockAddr) {
  switch (((sockaddr*)&sockAddr)->sa_family) {
    case AF_INET:
      return NetworkEndpoint(IpV4Address(((sockaddr_in*)&sockAddr)->sin_addr.s_addr), ntohs(((sockaddr_in*)&sockAddr)->sin_port));
    case AF_INET6:
      uint32_t* u32 = ((sockaddr_in6*)&sockAddr)->sin6_addr.un.u32_addr;
      if (u32[0] == 0 && u32[1] == 0 && u32[2] == htonl(0x0000FFFF))
        return NetworkEndpoint(IpV4Address(u32[3]), ntohs(((sockaddr_in6*)&sockAddr)->sin6_port));
      else
        // sin6_scope_id (uint32_t) is narrowed to IpV6Address::zoneId (uint8_t)
        // should be safe on ESP-IDF/lwIP where the scope ID is a netif index
        return NetworkEndpoint(IpV6Address(
          ((sockaddr_in6*)&sockAddr)->sin6_addr.un.u32_addr[0],
          ((sockaddr_in6*)&sockAddr)->sin6_addr.un.u32_addr[1],
          ((sockaddr_in6*)&sockAddr)->sin6_addr.un.u32_addr[2],
          ((sockaddr_in6*)&sockAddr)->sin6_addr.un.u32_addr[3],
          ((sockaddr_in6*)&sockAddr)->sin6_scope_id), ntohs(((sockaddr_in6*)&sockAddr)->sin6_port));
  }
  return NetworkEndpoint();
}

//==============================================================================

}