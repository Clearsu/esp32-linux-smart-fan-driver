import sys
import time
import struct
import serial

SYNC0 = 0xAA
SYNC1 = 0x55

PROTO_CMD_STATUS_REQ    = 0x01
PROTO_CMD_SET_FAN_MODE  = 0x02
PROTO_CMD_SET_FAN_STATE = 0x03
PROTO_CMD_SET_THRESHOLD = 0x04
PROTO_CMD_PING          = 0x05

PROTO_CMD_STATUS_RESP   = 0x81
PROTO_CMD_ACK           = 0x82
PROTO_CMD_PONG          = 0x83

PROTO_ERR_OK          = 0x00
PROTO_ERR_INVALID_ARG = 0x01
PROTO_ERR_STATE       = 0x02

PROTO_FAN_MODE_AUTO   = 0x00
PROTO_FAN_MODE_MANUAL = 0x01

PROTO_FAN_STATE_OFF   = 0x00
PROTO_FAN_STATE_ON    = 0x01

def crc16_ccitt_false(data: bytes, init = 0xFFFF) -> int:
    poly = 0x1021
    crc = init
    for b in data:
        crc ^= (b << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) & 0xFFFF) ^ poly
            else:
                crc = (crc << 1) & 0xFFFF
    return crc

class ProtoRx:
    RX_SYNC0 = 0
    RX_SYNC1 = 1
    RX_HEADER_CMD = 2
    RX_HEADER_SEQ = 3
    RX_HEADER_LEN = 4
    RX_PAYLOAD = 5
    RX_CRC_HI = 6
    RX_CRC_LO = 7
    
    def __init__(self):
        self.state = self.RX_SYNC0
        self.cmd = 0
        self.seq = 0
        self.len = 0
        self.payload = bytearray()
        self.crc = 0xFFFF
        self.crc_recv = 0

    def reset(self):
        self.__init__()

    def feed_byte(self, b: int):
        if self.state == self.RX_SYNC0:
            if b == SYNC0:
                self.state = self.RX_SYNC1

        elif self.state == self.RX_SYNC1:
            if b == SYNC1:
                self.state = self.RX_HEADER_CMD
                self.crc = 0xFFFF
            else:
                self.state = self.RX_SYNC0
        
        elif self.state == self.RX_HEADER_CMD:
            self.cmd = b
            self.crc = crc16_ccitt_false(bytes([b]), init=self.crc) 
            self.state = self.RX_HEADER_SEQ

        elif self.state == self.RX_HEADER_SEQ:
            self.seq = b
            self.crc = crc16_ccitt_false(bytes([b]), init=self.crc)
            self.state = self.RX_HEADER_LEN

        elif self.state == self.RX_HEADER_LEN:
            self.len = b
            if self.len > 32:
                self.reset()
                return None
            self.crc = crc16_ccitt_false(bytes([b]), init=self.crc)
            if self.len == 0:
                self.state = self.RX_CRC_HI
            else:
                self.payload = bytearray()
                self.state = self.RX_PAYLOAD

        elif self.state == self.RX_PAYLOAD:
            self.payload.append(b)
            self.crc = crc16_ccitt_false(bytes([b]), init=self.crc)
            if len(self.payload) == self.len:
                self.state = self.RX_CRC_HI

        elif self.state == self.RX_CRC_HI:
            self.crc_recv = b << 8
            self.state = self.RX_CRC_LO

        elif self.state == self.RX_CRC_LO:
            self.crc_recv |= b
            if self.crc == self.crc_recv:
                frame = {"cmd": self.cmd, "seq": self.seq, "len": self.len, "payload": bytes(self.payload)}
                self.reset()
                return frame
            print("Frame received but CRC does not match.")
            self.reset() 

        return None

def test(ser: serial.Serial, test_cmd: int, seq: int, payload: bytes, expected_cmd=None):
    header = bytes([test_cmd & 0xFF, seq & 0xFF, len(payload) & 0xFF])
    crc = crc16_ccitt_false(header + payload)
    frame = bytes([SYNC0, SYNC1, test_cmd & 0xFF, seq & 0xFF, len(payload) & 0xFF]) + payload + bytes([(crc >> 8) & 0xFF, crc & 0xFF])
    ser.write(frame)
    ser.flush()
    
    rx = ProtoRx()
    t0 = time.time()
    
    timeout = 2.0
    while time.time() - t0 < timeout:
        data = ser.read(64)
        if not data:
            continue
        for b in data:
            f = rx.feed_byte(b)
            if f is not None:
                print(f"<<< RX cmd=0x{f['cmd']:02X}, seq={f['seq']}, len={f['len']}")
                if expected_cmd == f["cmd"]:
                    return f
    raise TimeoutError("No expected response within timeout")

def decode_status_resp(payload):
    if len(payload) != 8:
        print("Length of STATUS_RESP payload not 8")
        return
    temp_x100, humid_x100, fan_mode, fan_state, errors = struct.unpack(">hHBBH", payload)
    temp = temp_x100 / 100.0
    humid = humid_x100 / 100.0
    print(f"  temp      = {temp:.2f} °C")
    print(f"  humid     = {humid:.2f} %")
    if fan_mode == PROTO_FAN_MODE_AUTO:
        print(f"  fan_mode  = AUTO")
    else:
        print(f"  fan_mode  = MANUAL")
    if fan_state == PROTO_FAN_STATE_ON:
        print(f"  fan_state = ON")
    else:
        print(f"  fan_state = OFF")
    print(f"  errors    = 0x{errors:04X}")

def decode_ack(payload, expected_status=None):
    if len(payload) != 2:
        print("Length of ACK payload not 2")
        return
    orig_cmd = payload[0]
    status = payload[1]
    print(f"  ACK for cmd=0x{orig_cmd:02X}, status=0x{status:02X}", end="")
    if status == PROTO_ERR_OK:
        print(" (OK)")
    elif status == PROTO_ERR_INVALID_ARG:
        print(" (INVALID_ARG)")
    elif status == PROTO_ERR_STATE:
        print(" (STATE)")
    else:
        print(" (UNKNOWN)")

def main():
    if len(sys.argv) != 2:
        print("Usage: {sys.argv[0]} /dev/cu.usbserial-XX")
        sys.exit(1)

    port = sys.argv[1]
    ser = serial.Serial(port, 115200, timeout=0.1)

    try:
        # 1) PING -> PONG
        seq = 1
        print(f"\n[seq: {seq}] Testing PING ...")
        f = test(ser, PROTO_CMD_PING, seq, b"", PROTO_CMD_PONG)
        print("Got PONG")
        time.sleep(1)

        # 2) STATUS_REQ -> STATUS_RESP
        seq += 1
        print(f"\n[seq: {seq}] Testing STATUS_REQ ...")
        f = test(ser, PROTO_CMD_STATUS_REQ, seq, b"", PROTO_CMD_STATUS_RESP)
        print("Got STATUS_RESP")
        decode_status_resp(f["payload"])
        time.sleep(1)

        # 3) SET_FAN_STATE (ON) -> ACK
        seq += 1
        print(f"\n[seq: {seq}] Testing SET_FAN_STATE to ON ... This should not work - the fan must remain OFF).")
        f = test(ser, PROTO_CMD_SET_FAN_STATE, seq, bytes([PROTO_FAN_STATE_ON]), PROTO_CMD_ACK)
        print("Got ACK")
        decode_ack(f["payload"])
        time.sleep(3)

        # 3) SET_FAN_MODE (MANUAL) -> ACK
        seq += 1
        print(f"\n[seq: {seq}] Testing SET_FAN_MODE to MANUAL ...")
        f = test(ser, PROTO_CMD_SET_FAN_MODE, seq, bytes([PROTO_FAN_MODE_MANUAL]), PROTO_CMD_ACK)
        print("Got ACK")
        decode_ack(f["payload"])
        time.sleep(1)

        # 4) SET_FAN_STATE (ON) -> ACK
        seq += 1
        print(f"\n[seq: {seq}] Testing SET_FAN_STATE to ON ...")
        f = test(ser, PROTO_CMD_SET_FAN_STATE, seq, bytes([PROTO_FAN_STATE_ON]), PROTO_CMD_ACK)
        print("Got ACK")
        decode_ack(f["payload"])
        time.sleep(5)

        # 5) SET_FAN_STATE (OFF) -> ACK
        seq += 1
        print(f"\n[seq: {seq}] Testing SET_FAN_STATE to OFF ... The fan will stop.")
        f = test(ser, PROTO_CMD_SET_FAN_STATE, seq, bytes([PROTO_FAN_STATE_OFF]), PROTO_CMD_ACK)
        print("Got ACK")
        decode_ack(f["payload"])
        time.sleep(2)

        # 6) SET_FAN_MODE (AUTO) -> ACK
        seq += 1
        print(f"\n[seq: {seq}] Testing SET_FAN_MODE to AUTO ...")
        f = test(ser, PROTO_CMD_SET_FAN_MODE, seq, bytes([PROTO_FAN_MODE_AUTO]), PROTO_CMD_ACK)
        print("Got ACK")
        decode_ack(f["payload"])
        time.sleep(2)

        # 7) SET_THRESHOLD (4°C) -> ACK
        seq += 1
        print(f"\n[seq: {seq}] Testing SET_THRESHOLD to 4°C ... The fan will be ON if it is room temperature.")
        temp_c = 4.0
        temp_x100 = int(temp_c * 100)
        payload = temp_x100.to_bytes(2, byteorder="big", signed=True)
        f = test(ser, PROTO_CMD_SET_THRESHOLD, seq, payload, PROTO_CMD_ACK)
        print("Got ACK")
        decode_ack(f["payload"])
        time.sleep(5)

        # 8) SET_THRESHOLD (30°C) -> ACK
        seq += 1
        print(f"\n[seq: {seq}] Testing SET_THRESHOLD to 30°C ... The fan will be OFF.")
        temp_c = 30.0
        temp_x100 = int(temp_c * 100)
        payload = temp_x100.to_bytes(2, byteorder="big", signed=True)
        f = test(ser, PROTO_CMD_SET_THRESHOLD, seq, payload, PROTO_CMD_ACK)
        print("Got ACK")
        decode_ack(f["payload"])
        time.sleep(3)

        # 9) STATUS_REQ -> STATUS_RESP
        seq += 1
        print(f"\n[seq: {seq}] Testing STATUS_REQ ...")
        f = test(ser, PROTO_CMD_STATUS_REQ, seq, b"", PROTO_CMD_STATUS_RESP)
        print("Got STATUS_RESP")
        decode_status_resp(f["payload"])

        print("\nTest successful.")

    except TimeoutError as e:
        print("Test failed: Error:", e)


if __name__ == "__main__":
    main()
