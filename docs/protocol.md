Frame:
+--------+-------+-----+-----+--------+-----------+-------+
| SYNC0  | SYNC1 | CMD | SEQ | LEN    | PAYLOAD   | CRC16 |
+--------+-------+-----+-----+--------+-----------+-------+
   1B       1B     1B    1B    1B       LEN byte     2B

  SYNC0 (1 byte)
    - frame start indicator
    - fixed value (eg. 0xAA)
  SYNC1 (1 byte)
    - second sync byte (eg. 0x55)
    - makes it easier to distinguish sync from garbage data
  CMD (1 byte)
    - command ID
  SEQ (1 byte)
    - 0 ~ 255
    - for request-reponse matching or retry
  LEN (1 byte)
    - payload length (0 ~ 255)
  PAYLOAD
    - it depends on commands
  CRC16 (2 bytes)
    - CRC for header + payload (except SYNC0, SYNC1)
    - big-endian

Commands:

  CMD   |  Name         |  Direction     |  Payload                 | Description
---------------------------------------------------------------------------------------------------------
  0x01    STATUS_REQ       Host -> ESP32    None                      Query current state
  0x02    SET_FAN_MODE     Host -> ESP32    1B mode                   AUTO/MANUAL
  0x03    SET_FAN_STATE    Host -> ESP32    1B state                  ON/OFF at MANUAL
  0x04    SET_THRESHOLD    Host -> ESP32    2B temp_x100              Threshold temperature (0.01°C unit)
  0x05    PING             Host -> ESP32    None                      Connection check
  0x81    STATUS_RESP      ESP32 -> Host    Struct                    Response to STATUS_REQ
  0x82    ACK              ESP32 -> Host    1B orig_cmd, 1B status    Success or failure of SET_*
  0x83    PONG             ESP32 -> HOST    None                      Response to PING

Command Payload:
  SET_FAN_MODE
    mode: 1 byte
      0x00 = AUTO
      0x01 = MANUAL
  SET_FAN_STATE
    state: 1 byte
      0x00 = OFF
      0x01 = ON
  SET_THRESHOLD
    temp_x100: 2 bytes, signed int16, big-endian
      e.g. 27.50°C -> 2750 (0X0ABE)
  STATUS_RESP
    struct {
        int16_t  temp_x100;
        uint16_t humidity_x100;
        uint8_t  fan_mode;
        uint8_t  fan_state;
        uint16_t errors;
    }
  ACK
    orig_cmd: 1 byte
    status: 1 byte
      0x00 = OK
      0x01 = ERROR_INVALID_ARG
      0x02 = ERROR_STATE

Receiving State Machine:
  typedef enum {
      RX_STATE_WAIT_SYNC0,
      RX_STATE_WAIT_SYNC1,
      RX_STATE_READ_HEADER,
      RX_STATE_READ_PAYLOAD,
      RX_STATE_READ_CRC
  } rx_state_t;

  1. WAIT_SYNC0
    - read byte
    - if 0xAA -> WAIT_SYNC1
    - else keep dumping
  2. WAIT_SYNC1
    - read byte
    - if 0x55: READ_HEADER
    - if 0xAA: restart from WAIT_SYNC0
    - else go to WAIT_SYNC0
  3. READ_HEADER
    - read CMD, SEQ, LEN (total 3 bytes) in order
    - if LEN > MAX_PAYLOAD: error -> WAIT_SYNC0
    - if ok, allocate buffer and READ_PAYLOAD
  4. READ_PAYLOAD
    - fill LEN bytes
  5. READ_CRC
    - read 2 bytes and compare it with calculated CRC16
    - if match: push to the queue
    - else: discard all, restart from WAIT_SYNC0

CRC16 Calculation:
  CRC-16/CCITT-FALSE
    - Poly: 0x1021
    - Init: 0xFFFF
    - Input/Output reflection: None
    - XOR out: 0x0000
  Calculation Range
    - CMD, SEQ, LEN, PAYLOAD (exclude SYNC0, SYNC1)
