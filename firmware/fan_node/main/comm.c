#include "driver/uart.h"

#include "comm.h"

static const char	*TAG = "COMM";

void	comm_init(void)
{
	uart_config_t	uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	};
	ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 2048, 0, 0, NULL, 0));
}

bool	comm_send_frame(const proto_frame_t *frame)
{
	uint8_t		buf[2 + 3 + PROTO_MAX_PAYLOAD + 1]; // SYNC(2) + CMD/SEQ/LEN(3) + PAYLOAD + CRC(1)
	uint16_t	out_len;
	uint16_t	written_len;

	if (!frame || frame->len > PROTO_MAX_PAYLOAD)
		return false;
	if (!proto_build_frame(frame->cmd, frame->seq, frame->payload,
							frame->len, buf, &out_len))
	{
		ESP_LOGE(TAG, "Failed to build frame (cmd=0x%02X)", frame->cmd);
		return false;
	}
	written_len = uart_write_bytes(UART_NUM_0, (const char*)buf, out_len);
	if (written_len != out_len)
	{
		ESP_LOGE(TAG, "UART write failed: written=%u expected=%u", written_len, out_len);
		return false;
	}
	return true;
}
