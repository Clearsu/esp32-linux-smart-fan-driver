## `fanctl_serial` (legacy)

This userspace tool communicates directly with the ESP32 fan node via a raw serial interface.

It was used during early development for protocol bring-up and firmware debugging.

Due to inherent race conditions and lack of serialization when multiple userspace processes concurrently access the same tty device, this approach was replaced by a kernel driver exposing a single ioctl-based interface (`/dev/fanctl`).


### Build

```bash
make all
```

#### Debug build

```bash
make debug
```

#### Clean

```bash
clean
```

### Usage
```bash
./fanctl /dev/ttyXXX <cmd>
```
Where `/dev/ttyXXX` is the USB-to-UART device connected to the ESP32.

**cmd**:
1. `ping`: connection check - expects `PONG`
2. `status`: request current temperature, humidity, fan mode, fan state, and errors
3. `auto`: set fan mode automatic (fan will be on when the temperature reaches the threshold)
4. `manual`: set fan mode manual
5. `on`: set fan state on (when the mode is manual)
6. `off`: set fan state off (when the mode is manual)
7. `threshold <tempC>`: set threshold 
