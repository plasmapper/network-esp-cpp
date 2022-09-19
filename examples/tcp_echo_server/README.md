# TCP echo server example

1. EchoServer class inherits TcpServer class and overrides HandleRequest method to implement echo functionality
2. Internal ESP Wi-Fi station interface is created and initialized.
3. Wi-Fi station SSID and password are set from project configuration (can be changed in `Example` menu of [Project Configuration](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html)).
4. Wi-Fi station is enabled.
5. The server is enabled when the Wi-Fi staion interface gets an IP address.
6. The server events are used to print the client connection/disconnection information.
