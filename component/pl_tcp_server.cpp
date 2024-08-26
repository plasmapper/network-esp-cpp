#include "pl_tcp_server.h"
#include "lwip/sockets.h"
#include "esp_check.h"

//==============================================================================

static const char* TAG = "pl_tcp_server";

//==============================================================================

namespace PL {

//==============================================================================

const TaskParameters TcpServer::defaultTaskParameters = {4096, tskIDLE_PRIORITY + 5, 0};

//==============================================================================

TcpServer::TcpServer(uint16_t port) : clientConnectedEvent(*this), clientDisconnectedEvent(*this), port(port) {}

//==============================================================================

TcpServer::~TcpServer() {
  while (taskHandle) {
    disable = true;
    vTaskDelay(1);
  }
  for (auto& clientStream : clientStreams)
    clientStream->Close();
}

//==============================================================================

esp_err_t TcpServer::Lock(TickType_t timeout) {
  esp_err_t error = mutex.Lock(timeout);
  if (error == ESP_OK)
    return ESP_OK;
  if (error == ESP_ERR_TIMEOUT && timeout == 0)
    return ESP_ERR_TIMEOUT;
  ESP_RETURN_ON_ERROR(error, TAG, "mutex lock failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpServer::Unlock() {
  ESP_RETURN_ON_ERROR(mutex.Unlock(), TAG, "mutex unlock failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpServer::Enable() {
  LockGuard lg(*this);
  if (taskHandle == xTaskGetCurrentTaskHandle()) {
    enableFromRequest = true;
    return ESP_OK;
  }
  if (taskHandle)
    return ESP_OK;
  
  disable = false;
  if (xTaskCreatePinnedToCore(TaskCode, GetName().c_str(), taskParameters.stackDepth, this, taskParameters.priority, &taskHandle, taskParameters.coreId) != pdPASS) {
    taskHandle = NULL;
    ESP_RETURN_ON_ERROR(ESP_FAIL, TAG, "task create failed");
  }
  enabledEvent.Generate();
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpServer::Disable() {
  LockGuard lg(*this);
  if (taskHandle == xTaskGetCurrentTaskHandle()) {
    enableFromRequest = false;
    disableFromRequest = true;
    return ESP_OK;
  }
  if (!taskHandle)
    return ESP_OK;
  
  while (taskHandle) {
    disable = true;
    vTaskDelay(1);
  }

  for (auto& clientStream : clientStreams)
    clientStream->Close();
  clientStreams.clear();

  disabledEvent.Generate();
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpServer::EnableNagleAlgorithm() {
  LockGuard lg(*this);
  this->nagleAlgorithmEnabled = true;
  ESP_RETURN_ON_ERROR(SetStreamSocketOptions(), TAG, "stream socket options set failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpServer::DisableNagleAlgorithm() {
  LockGuard lg(*this);
  this->nagleAlgorithmEnabled = false;
  ESP_RETURN_ON_ERROR(SetStreamSocketOptions(), TAG, "stream socket options set failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpServer::EnableKeepAlive() {
  LockGuard lg(*this);
  this->keepAliveEnabled = true;
  ESP_RETURN_ON_ERROR(SetStreamSocketOptions(), TAG, "stream socket options set failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpServer::DisableKeepAlive() {
  LockGuard lg(*this);
  this->keepAliveEnabled = false;
  ESP_RETURN_ON_ERROR(SetStreamSocketOptions(), TAG, "stream socket options set failed");
  return ESP_OK;
}


//==============================================================================

bool TcpServer::IsEnabled() {
  LockGuard lg(*this);
  return taskHandle;
}

//==============================================================================

uint16_t TcpServer::GetPort() {
  LockGuard lg(*this);
  return port;
}

//==============================================================================

esp_err_t TcpServer::SetPort(uint16_t port) {
  LockGuard lg(*this);
  this->port = port;
  ESP_RETURN_ON_ERROR(RestartIfEnabled(), TAG, "restart failed");
  return ESP_OK;
}

//==============================================================================

size_t TcpServer::GetMaxNumberOfClients() {
  LockGuard lg(*this);
  return maxNumberOfClients;
}

//==============================================================================

esp_err_t TcpServer::SetMaxNumberOfClients(size_t maxNumberOfClients) {
  LockGuard lg(*this);
  this->maxNumberOfClients = maxNumberOfClients;
  ESP_RETURN_ON_ERROR(RestartIfEnabled(), TAG, "restart failed");
  return ESP_OK;
}

//==============================================================================

std::vector<std::shared_ptr<NetworkStream>> TcpServer::GetClientStreams() {
  LockGuard lg(*this);
  return clientStreams;  
}

//==============================================================================

esp_err_t TcpServer::SetTaskParameters(const TaskParameters& taskParameters) {
  LockGuard lg(*this);
  this->taskParameters = taskParameters;
  ESP_RETURN_ON_ERROR(RestartIfEnabled(), TAG, "restart failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpServer::SetKeepAliveIdleTime(int seconds) {
  LockGuard lg(*this);
  this->keepAliveIdleTime = seconds;
  ESP_RETURN_ON_ERROR(RestartIfEnabled(), TAG, "restart failed");
  return ESP_OK;
  return SetStreamSocketOptions();
}

//==============================================================================

esp_err_t TcpServer::SetKeepAliveInterval(int seconds) {
  LockGuard lg(*this);
  this->keepAliveInterval = seconds;
  ESP_RETURN_ON_ERROR(SetStreamSocketOptions(), TAG, "stream socket options set failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpServer::SetKeepAliveCount(int count) {
  LockGuard lg(*this);
  this->keepAliveCount = count;
  ESP_RETURN_ON_ERROR(SetStreamSocketOptions(), TAG, "stream socket options set failed");
  return ESP_OK;
}

//==============================================================================

esp_err_t TcpServer::SetStreamSocketOptions() {
  esp_err_t error = ESP_OK;
  for (auto& clientStream : clientStreams) {
    error = (nagleAlgorithmEnabled ? clientStream->EnableNagleAlgorithm() : clientStream->DisableNagleAlgorithm()) == ESP_OK ? error : ESP_FAIL;
    error = (keepAliveEnabled ? clientStream->EnableKeepAlive() : clientStream->DisableKeepAlive()) == ESP_OK ? error : ESP_FAIL;
    error = clientStream->SetKeepAliveIdleTime(keepAliveIdleTime) == ESP_OK ? error : ESP_FAIL;
    error = clientStream->SetKeepAliveInterval(keepAliveInterval) == ESP_OK ? error : ESP_FAIL;
    error = clientStream->SetKeepAliveCount(keepAliveCount) == ESP_OK ? error : ESP_FAIL;
  }
  ESP_RETURN_ON_ERROR(error, TAG, "stream socket options set failed");
  return ESP_OK;
}

//==============================================================================

void TcpServer::TaskCode(void* parameters) {
  TcpServer& server = *(TcpServer*)parameters;
  int sock = -1;

  while (!server.disable) {
    if (server.Lock(0) == ESP_OK) {
      if (sock >= 0 || (sock = server.Listen()) >= 0) {
        // Remove disconnected clients
        for (auto clientStream = server.clientStreams.begin(); clientStream != server.clientStreams.end();) {
          if ((*clientStream)->IsOpen())
            clientStream++;
          else {
            server.clientDisconnectedEvent.Generate(**clientStream);
            server.clientStreams.erase(clientStream);
          }
        }

        // Accept new clients
        fd_set set;
        timeval timeout = {};
        for (bool noPendingConnections = false; server.clientStreams.size() < server.maxNumberOfClients && !noPendingConnections;) {
          FD_ZERO(&set);
          FD_SET(sock, &set);
          if (select(sock + 1, &set, NULL, NULL, &timeout) > 0) {
            int newClientSock = accept(sock, NULL, NULL);
            if (newClientSock >= 0) {
              auto clientStream = std::make_shared<NetworkStream>(newClientSock);
              server.clientStreams.push_back(clientStream);
              server.SetStreamSocketOptions();
              server.clientConnectedEvent.Generate(*clientStream);
            }
          }
          else
            noPendingConnections = true;
        }

        // Handle requests
        for (auto& clientStream : server.clientStreams) {
          LockGuard lg(*clientStream);
          if (clientStream->GetReadableSize())
            server.HandleRequest(*clientStream);
        }

        if (server.disableFromRequest) {
          server.disableFromRequest = false;
          for (auto& clientStream : server.clientStreams)
            clientStream->Close();
          server.clientStreams.clear();
          close(sock);
          sock = -1;
          if (!server.enableFromRequest) {
            server.disabledEvent.Generate();
            server.taskHandle = NULL;
            server.Unlock();
            vTaskDelete(NULL);
            return;
          }
        }
        server.enableFromRequest = false;
      }
      else
        server.disable = true;

      server.Unlock();
    }
    vTaskDelay(1);
  }

  if (sock >= 0)
    close(sock);
  
  server.taskHandle = NULL;
  vTaskDelete(NULL);
}

//==============================================================================

int TcpServer::Listen() {
  int sock;
  if ((sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) >= 0) {
    int enableAddressReuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enableAddressReuse, sizeof(enableAddressReuse)) == 0) {
      sockaddr_in6 addr = {};
      addr.sin6_family = AF_INET6;
      addr.sin6_port = htons(port);
      if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == 0) {
        if (listen(sock, maxNumberOfClients) == 0)
          return sock;
        else
          ESP_LOGE(TAG, "socket listen failed (%d)", errno);
      }
      else
        ESP_LOGE(TAG, "socket bind failed (%d)", errno);
    }
    else
      ESP_LOGE(TAG, "socket set reuse address failed (%d)", errno);

    close(sock);
    sock = -1;
  }
  else
    ESP_LOGE(TAG, "socket create failed (%d)", errno);

  return sock;
}

//==============================================================================

esp_err_t TcpServer::RestartIfEnabled() {
  if (!taskHandle || disableFromRequest)
    return ESP_OK;
  ESP_RETURN_ON_ERROR(Disable(), TAG, "disable failed");
  ESP_RETURN_ON_ERROR(Enable(), TAG, "enable failed");
  return ESP_OK;
}

//==============================================================================

}