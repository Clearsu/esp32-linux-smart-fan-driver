# esp32-linux-smart-fan-driver
ESP32 smart fan node with Linux kernel driver

- ESP32 + DHT22 + SG90 servo "fan" node
- Custom UART binary protocol with CRC between ESP32 and embedded Linux
- Linux kernel character driver exposing `/dev/esp32_node`
- Userspace CLI tool `esp32ctl` for monitoring and control

### udev setup (required)

This project expects the CP2102 USB-to-UART adapter to appear as:

    /dev/ttyFAN

Install the udev rule:

```bash
cd tools/scripts/udev
chmod +x ./install_udev.sh
sudo ./install_udev.sh
```

Then, unplug and replug the USB device. When you run `ls -l /dev/ttyFAN`, you should see something like:
```
lrwxrwxrwx 1 root root 7 Dec 18 13:13 /dev/ttyFAN -> ttyUSB1
```
