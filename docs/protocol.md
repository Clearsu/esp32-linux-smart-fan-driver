# UART Protocol Specification

This document describes the UART communication protocol used between the ESP32 fan controller node and the Linux host driver.

## 1. Frame Format

| SYNC0  | SYNC1  | CMD  | SEQ   | LEN    | PAYLOAD   | CRC16  |
|--------|--------|------|-------|--------|-----------|--------|
|1B      | 1B     | 1B   |  1B   |   1B   | LEN bytes |  2B    |


### Field Description

- **SYNC0 (1 byte)**  
  Frame start indicator. Fixed value `0xAA`.

- **SYNC1 (1 byte)**  
  Second sync byte. Fixed value `0x55`.

- **CMD (1 byte)**  
  Command ID.

- **SEQ (1 byte)**  
  Sequence number (0–255).  
  Used for request/response pairing or retransmission.

- **LEN (1 byte)**  
  Payload length in bytes (0–255).

- **PAYLOAD**  
  Command-specific data.

- **CRC16 (2 bytes)**  
  CRC computed over `CMD + SEQ + LEN + PAYLOAD`.  
  Big-endian order.  
  SYNC bytes are excluded.

## 2. Command List

| CMD   | Name          | Direction     | Payload             | Description                   |
|-------|---------------|---------------|---------------------|-------------------------------|
| 0x01  | STATUS_REQ    | Host → ESP32  | None                | Request current system status |
| 0x02  | SET_FAN_MODE  | Host → ESP32  | 1 byte (mode)       | AUTO / MANUAL                 |
| 0x03  | SET_FAN_STATE | Host → ESP32  | 1 byte (state)      | ON / OFF (manual only)        |
| 0x04  | SET_THRESHOLD | Host → ESP32  | 2 bytes temp_x100   | Threshold temperature         |
| 0x05  | PING          | Host → ESP32  | None                | Connectivity check            |
| 0x81  | STATUS_RESP   | ESP32 → Host  | struct              | Response to STATUS_REQ        |
| 0x82  | ACK           | ESP32 → Host  | orig_cmd + status   | Result of SET_*               |
| 0x83  | PONG          | ESP32 → Host  | None                | Response to PING              |


## 3. Payload Definitions

### 3.1 SET_FAN_MODE

mode (1 byte)
0x00 = AUTO
0x01 = MANUAL

### 3.2 SET_FAN_STATE

state (1 byte)
0x00 = OFF
0x01 = ON

### 3.3 SET_THRESHOLD

temp_x100 : int16 (big-endian)
Example:
27.50°C → 2750  (0x0ABE)

### 3.4 STATUS_RESP Structure

```c
typedef struct {
    int16_t  temp_x100;       // 0.01°C
    uint16_t humidity_x100;   // 0.01% RH
    uint8_t  fan_mode;        // 0=AUTO, 1=MANUAL
    uint8_t  fan_state;       // 0=OFF, 1=ON
    uint16_t errors;          // bitfield
} status_resp_t;
```

### 3.5 ACK

orig_cmd (1 byte)
status   (1 byte)
  0x00 = OK
  0x01 = ERROR_INVALID_ARG
  0x02 = ERROR_STATE


## 4. UART Receive State Machine
```c
typedef enum {
    RX_STATE_WAIT_SYNC0,
    RX_STATE_WAIT_SYNC1,
    RX_STATE_READ_HEADER,
    RX_STATE_READ_PAYLOAD,
    RX_STATE_READ_CRC
} rx_state_t;
```

### State Description
#### 1. WAIT_SYNC0
- Read one byte.
- If 0xAA → go to WAIT_SYNC1.
- Otherwise ignore.
#### 2. WAIT_SYNC1
- Read one byte.
- If 0x55 → go to READ_HEADER.
- If 0xAA → stay in WAIT_SYNC1.
- Else → return to WAIT_SYNC0.
#### 3.	READ_HEADER
- Read CMD, SEQ, LEN.
- If LEN > MAX_PAYLOAD → error → WAIT_SYNC0.
- Otherwise → READ_PAYLOAD.
#### 4.	READ_PAYLOAD
- Read LEN bytes.
- After receiving all → READ_CRC.
#### 5.	READ_CRC
- Read 2 bytes.
- Compare with computed CRC16.
- If match → packet complete.
- Else discard and go to WAIT_SYNC0.

## 5. CRC16 Specification

### Algorithm: CRC-16/CCITT-FALSE
- Polynomial: 0x1021
- Initial value: 0xFFFF
- Input reflection: None
- Output reflection: None
- XOR Out: 0x0000

### CRC Calculation Range
Included:
- CMD
- SEQ
- LEN
- PAYLOAD

Excluded:
- SYNC0
- SYNC1

## 6. Example Packet (Hex Dump)

More examples will be added later when implementing host & ESP32 libraries.

AA 55 01 02 00 B8 3F   ← STATUS_REQ (example)
