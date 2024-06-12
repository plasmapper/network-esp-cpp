# TCP OTA Server Example

1. OtaServer class inherits TcpServer class and overrides HandleRequest method to implement Over The Air Update functionality.
2. Internal ESP Wi-Fi station interface is created and initialized.
3. The Wi-Fi station SSID and password are set from project configuration (can be changed in `Example` menu of [Project Configuration](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html)).
4. The Wi-Fi station is enabled.
5. The server is enabled when the Wi-Fi station interface gets an IP address.
6. The new firmware (\<project\>.bin file from the build folder) can be uploaded by sending 4-byte file length (little-endian) followed by the file data via TCP.
7. The firmware compatibility test is simulated by the `firmwareIsCompatibleWithHardware` flag.
