#include "pl_network_interface.h"

//==============================================================================

namespace PL {

//==============================================================================

NetworkInterface::NetworkInterface() :
  connectedEvent(*this), disconnectedEvent(*this),
  gotIpV4AddressEvent(*this), lostIpV4AddressEvent(*this),
  gotIpV6AddressEvent(*this), lostIpV6AddressEvent(*this) {}

//==============================================================================

}