# esp32-linux-smart-fan-driver
ESP32 smart fan node with Linux kernel driver

- ESP32 + DHT22 + SG90 servo "fan" node
- Custom UART binary protocol with CRC between ESP32 and embedded Linux
- Linux kernel character driver exposing `/dev/esp32_node`
- Userspace CLI tool `esp32ctl` for monitoring and control
