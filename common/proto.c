#include "proto.h"

static proto_u16	crc16_step(proto_u16 crc, proto_u8 data)
{
	crc ^= (proto_u16)data << 8;
	for (int i = 0; i < 8; i++)
	{
		if (crc & 0x8000)
			crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
	}
	return crc;
}

proto_u16	proto_crc16(const proto_u8 *data, proto_u16 len)
{
	proto_u16	crc;

	crc = 0xFFFF;
	for (int i = 0; i < len; i++)
		crc = crc16_step(crc, data[i]);
	return crc;
}

bool	proto_build_frame(proto_u8 cmd, proto_u8 seq, const proto_u8 *payload,
						proto_u8 len, proto_u8 *out, proto_u16 *out_len)
{
	proto_u16	pos;
	proto_u16	crc;

	if (len > PROTO_MAX_PAYLOAD || !out || !out_len)
		return false;
	if (len > 0 && !payload)
		return false;
	pos = 0;
	out[pos++] = PROTO_SYNC0;
	out[pos++] = PROTO_SYNC1;
	out[pos++] = cmd;
	out[pos++] = seq;
	out[pos++] = len;
	for (int i = 0; i < len; i++)
		out[pos++] = payload[i];
	crc = proto_crc16(&out[2], 3 + len); // 3 bytes (cmd, seq, len) + payload length
	out[pos++] = crc >> 8;
	out[pos++] = crc & 0xFF;
	*out_len = pos;
	return true;
}

void	proto_rx_init(proto_rx_t *r)
{
	r->st = RX_SYNC0;
	r->pos = 0;
}

bool	proto_rx_feed(proto_rx_t *r, proto_u8 b, proto_frame_t *out)
{
	if (!r || !out)
		return false;
	switch (r->st)
	{
		case RX_SYNC0:
			if (b == PROTO_SYNC0)
				r->st = RX_SYNC1;
			break;
		case RX_SYNC1:
			if (b == PROTO_SYNC1)
			{
				r->st = RX_HEADER_CMD;
				r->crc = 0xFFFF;
			}
			else
				r->st = RX_SYNC0;
			break;
		case RX_HEADER_CMD:
			r->cmd = b;
			r->crc = crc16_step(r->crc, b);
			r->st = RX_HEADER_SEQ;
			break;
		case RX_HEADER_SEQ:
			r->seq = b;
			r->crc = crc16_step(r->crc, b);
			r->st = RX_HEADER_LEN;
			break;
		case RX_HEADER_LEN:
			r->len = b;
			r->crc = crc16_step(r->crc, b);
			if (r->len > PROTO_MAX_PAYLOAD)
			{
				r->st = RX_SYNC0;
				break;
			}
			if (r->len == 0)
				r->st = RX_CRC_HI;
			else
			{
				r->pos = 0;
				r->st = RX_PAYLOAD;
			}
			break;
		case RX_PAYLOAD:
			r->payload[r->pos++] = b;
			r->crc = crc16_step(r->crc, b);
			if (r->pos == r->len)
				r->st = RX_CRC_HI;
			break;
		case RX_CRC_HI:
			r->crc_recv = (proto_u16)b << 8;
			r->st = RX_CRC_LO;
			break;
		case RX_CRC_LO:
			r->crc_recv |= b;
			if (r->crc == r->crc_recv)
			{
				out->cmd = r->cmd;
				out->seq = r->seq;
				out->len = r->len;
				for (int i = 0; i < r->len; i++)
					out->payload[i] = r->payload[i];
				proto_rx_init(r);
				return true;
			}
			proto_rx_init(r);
			break;
	}
	return false;
}
