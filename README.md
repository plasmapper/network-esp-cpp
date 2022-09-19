# Network C++ Class Component for ESP-IDF

## Requirements
ESP-IDF 4.4 and higher.

[pl_common](https://github.com/plasmapper/common-esp-cpp) component.

## Installation
Add this to `main/idf_component.yml`:
```yaml
dependencies:
  pl_network:
    path: component
    git: https://github.com/plasmapper/network-esp-cpp.git
```
Add this to the source code:
```C++
#include "pl_network.h"
```
Add `extern "C"` to `app_main` in `main.cpp`:
```C++
extern "C" void app_main(void) {...}
```

## Features
1. Base classes for network interface, stream and server.
2. Base classes for Ethernet and Wi-Fi interfaces.
3. Class wrappers for ESP internal Ethernet and Wi-Fi interfaces.
4. TCP client class.
5. TCP server class.

## Examples
[Ethernet](examples/ethernet)  
[Wi-Fi station](examples/wifi_station)  
[TCP echo server](examples/tcp_echo_server)

## Documentation
[Documentation](https://plasmapper.github.io/esp-cpp/network)
