# esp32-linux-smart-fan-driver

## Summary

ESP32-based smart fan node controlled from Linux through a custom kernel driver.

This project implements an end-to-end embedded system consisting of:

- An ESP32 firmware running multiple FreeRTOS tasks
- A custom binary UART protocol with CRC
- A Linux kernel module using a tty line discipline
- A character device driver exposing a synchronous ioctl interface
- A userspace CLI tool for monitoring and control

The goal of this project is to explore low-level Linux–MCU communication, kernel-space design, and real-time firmware architecture in a single coherent system.

## Tech Stack
1. **Language**: C, Python (test script)
2. **Embedded / Firmware**: ESP-IDF, FreeRTOS, UART, GPIO, PWM
3. **Linux Kernel**: Linux Kernel Module, tty line discipline
4. **Userspace**: POSIX
5. **Build**: GNU Make, ESP-IDF build system

## Demo

## System Architecture

### Design Rationale

#### UART protocol
A custom binary protocol is used instead of text-based commands to ensure:
- deterministic parsing
- compact messages
- CRC-based integrity checking

#### tty line discipline
The line discipline allows raw byte-stream processing directly in kernel space,
avoiding userspace buffering and framing issues.

#### Character device + ioctl
The kernel module exposes a synchronous request/response interface using ioctl,
making the userspace API simple and explicit.


## Repository Structure


## Components

### Hardware
- ESP32 DevKitC V4
- DHT22 temperature & humidity sensor
- SG90 servo motor (fan actuator)
- CP2102 USB-to-UART bridge

### Software

#### ESP32 Firmware
- Built with ESP-IDF
- Uses FreeRTOS with multiple tasks:
- `uart_read_task`: receives raw UART bytes and parses frames
- `cmd_handler_task`: processes protocol commands
- `sensor_task`: reads DHT22 sensor periodically
- `fan_control_task`: controls servo based on system state
- A central system state (`sys_state_t`) stored in a shared structure
- UART communication uses the same protocol definition as the Linux side

#### Linux Kernel Module
- Custom tty line discipline
- Parses incoming UART frames in kernel space
- Implements a character device `/dev/fanctl`
- Supports synchronous command/response using:
  - A mutex for request serialization
  - Completion for blocking wait
- Exposes control operations via `ioctl`

#### Userspace CLI
- Communicates only through `/dev/fanctl`
- No direct UART access from userspace
- Supports:
  - `ping`: connection check
  - `status`: fan node status query
  - `auto`, `manual`: fan mode control
  - `on`, `off`: fan power control
  - `threshold [temp (°C)]`: temperature threshold configuration


## How to Run

### 1. Install Ubuntu Virtual Machine
It is strongly recommended to run this project inside a virtual machine for safety, as it involves building and loading a custom Linux kernel module.

- This project was developed and tested on `Ubuntu 5.15.0-164-generic` using `UTM`.

### 2. Flash ESP32 Firmware

Build and flash the firmware using ESP-IDF:

```bash
cd firmware/fan_node
idf.py build
idf.py -p [PORT] flash monitor
```

### 3. udev Setup

To simplify device handling, it is recommended to create a persistent symlink for the CP2102 USB-to-UART adapter.
Without this, the device name may vary (`/dev/ttyUSB0`, `/dev/ttyUSB1`, ...) depending on connection order or number of USB peripherals.

Install the udev rule:

```bash
cd tools/scripts/udev
chmod +x install_udev.sh
sudo ./install_udev.sh
```

Reconnect the USB device and verify:

```bash
ls -l /dev/ttyFAN
```

Example output:

```
lrwxrwxrwx 1 root root 7 Dec 18 13:13 /dev/ttyFAN -> ttyUSB1
```

### 4. tty Configuration

The tty device must be configured for raw binary transmission:

```bash
stty -F /dev/ttyFAN 115200 raw -echo -ixon -ixoff -crtscts
```

This disables:
	•	line buffering
	•	echo
	•	software/hardware flow control

### 5. Build and Load Kernel Module

```bash
cd kernel/fanctl
make
sudo insmod fanctl.ko
```

Attach the line discipline:

```bash
sudo ldattach 27 /dev/ttyFAN
```
- 27: line discpline ID (`N_FANCTL`, defined in `kernel/fanctl/fanctl.h`)


### 6. Build and Run Userspace CLI

```bash
cd userspace/fanctl
make
./fanctl status
```

## License
MIT
