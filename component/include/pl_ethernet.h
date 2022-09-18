#pragma once
#include "pl_network_interface.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief Ethernet interface class
class Ethernet : public virtual NetworkInterface {
public:
  /// @brief Create an Ethernet interface
  Ethernet() {}
};

//==============================================================================

}