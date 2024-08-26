#include "pl_network.h"

//==============================================================================

class TcpServer : public PL::TcpServer {
public:
  using PL::TcpServer::TcpServer;

protected:
  esp_err_t HandleRequest(PL::NetworkStream& clientStream) override;
};

//==============================================================================

void TestTcp();
bool CompareEndpoints(const PL::NetworkEndpoint& ep1, const PL::NetworkEndpoint& ep2);