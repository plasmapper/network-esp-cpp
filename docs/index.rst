Network Component
=================

.. |COMPONENT| replace:: network

.. |ESP_IDF_VERSION| replace:: 5.3

.. |VERSION| replace:: 1.1.2

.. include:: ../../../installation.rst

.. include:: ../../../sdkconfig_network.rst

Features
--------

1. :cpp:struct:`PL::IpV4Address` and :cpp:struct:`PL::IpV6Address` - data types for IPv4 and IPv6 addresses with number and string initialization
   and ToString methods. :cpp:struct:`PL::NetworkEndpoint` - a data type for an IP address (v4 or v6) and a port.
2. :cpp:class:`PL::NetworkInterface` - a base class for any network interface.
3. :cpp:class:`PL::Ethernet` - a base class for any ethernet interface.
4. :cpp:class:`PL::WiFiStation` - a base class for any Wi-Fi station.
5. :cpp:class:`PL::EspNetworkInterface` - is a base class for the internal ESP Ethernet and WiFi station. It implements several :cpp:class:`PL::NetworkInterface`
   methods using ESP C api for the IP and DHCP configuration. 
6. :cpp:class:`PL::EspEthernet` - is a :cpp:class:`PL::Ethernet` implementation for the internal ESP Ethernet. It is initialized with the specific
   ``esp_eth_phy_new_xxx`` function for the PHY chip that is used. 
   :cpp:func:`PL::EspEthernet::Initialize` installs the Ethernet driver. :cpp:func:`PL::EspEthernet::Enable` and :cpp:func:`PL::EspEthernet::Disable` enable
   and disable the interface. :cpp:func:`PL::EspEthernet::IsConnected` checks the connection status.
7. :cpp:class:`PL::EspWiFiStation` - is a :cpp:class:`PL::WiFiStation` implementation for the internal ESP Wi-Fi station. 
   :cpp:func:`PL::EspWiFiStation::Initialize` installs the Wi-Fi driver. :cpp:func:`PL::EspWiFiStation::Enable` and :cpp:func:`PL::EspWiFiStation::Disable` enable
   and disable the interface. :cpp:func:`PL::EspWiFiStation::IsConnected` check the connection status.
   :cpp:func:`PL::EspWiFiStation::GetSsid`, :cpp:func:`PL::EspWiFiStation::SetSsid`, :cpp:func:`PL::EspWiFiStation::GetPassword`
   and :cpp:func:`PL::EspWiFiStation::SetPassword` get and set Wi-Fi station SSID and password.
8. :cpp:class:`PL::NetworkStream` - a base class for any network stream. In addition to :cpp:class:`PL::Stream` methods it provides Nagle algorithm
   enabling/disabling, keep-alive packet configuration and getting local/remote endpoint information.
9. :cpp:class:`PL::NetworkServer` - a base class for any network server. In addition to :cpp:class:`PL::Server` methods it provides port and maximum number
   of clients configuration.
10. :cpp:class:`PL::TcpClient` - a TCP client class. It is initialized with an IP address and a port, that can be changed later.
    :cpp:func:`PL::TcpClient::Connect` and :cpp:func:`PL::TcpClient::Disonnect` connect/disconenct the client from the server.
    :cpp:func:`PL::TcpClient::GetStream` returns a lockable :cpp:class:`PL::NetworkStream` for reading and writing.
11. :cpp:class:`PL::TcpServer` - a :cpp:class:`PL::NetworkServer` implementation for TCP connections. The descendant class should override
    :cpp:func:`PL::TcpServer::HandleRequest` to handle the client request. :cpp:func:`PL::TcpServer::HandleRequest` is only called for clients
    with the incoming data in the internal buffer.

Thread safety
-------------

Class method thread safety is implemented by having the :cpp:class:`PL::Lockable` as a base class and creating the class object lock guard at the beginning of the methods.

:cpp:class:`PL::TcpServer` task method locks both the :cpp:class:`PL::TcpServer` and the client :cpp:class:`PL::NetworkStream` objects for the duration of the transaction.

Examples
--------
| `Ethernet <https://components.espressif.com/components/plasmapper/pl_network/versions/1.1.2/examples/ethernet>`_
| `Wi-Fi station <https://components.espressif.com/components/plasmapper/pl_network/versions/1.1.2/examples/wifi_station>`_
| `TCP echo server <https://components.espressif.com/components/plasmapper/pl_network/versions/1.1.2/examples/tcp_echo_server>`_
| `TCP OTA server <https://components.espressif.com/components/plasmapper/pl_network/versions/1.1.2/examples/tcp_ota_server>`_
  
API reference
-------------

.. toctree::
  
  api/types      
  api/network_interface
  api/ethernet
  api/wifi_station
  api/esp_network_interface
  api/esp_ethernet
  api/esp_wifi_station
  api/network_stream
  api/network_server
  api/tcp_client
  api/tcp_server