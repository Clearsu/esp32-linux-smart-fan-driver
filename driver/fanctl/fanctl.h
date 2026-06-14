#pragma once

#include <linux/types.h>
#include <linux/tty.h>
#include <linux/tty_ldisc.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

#include "proto.h"

#define N_FANCTL 27

/**
 * fanctl_ctx
 * ----------
 * Per-tty context for the fanctl line discipline.
 *
 * It holds all state required to communicate with a ESP32 fan node
 * over a tty device. It is shared between:
 * 
 * - the tty line discipline RX path
 * - the userspace ioctl handler
 * 
 * The driver follows a single synchronous request-response model:
 * only single command is allowed at a time.
 */
typedef struct fanctl_ctx
{
	/* Associated TTY device */
	struct tty_struct	*tty;

	/* Protocol RX state machine */
	proto_rx_t		rx;

	/* Locks */
	struct mutex		req_lock; // serialize ioctl requests
	spinlock_t		resp_lock; // protect last_resp (RX context)
	struct completion	resp_done; // wait for matching response

	/* Single synchronous request-response state */
	bool			waiting; // true when waiting for a response
	u8			pending_cmd; // command being waited for
	u8			pending_seq; // sequence number of pending request
	proto_frame_t		last_resp; // last received matching response

	/* RX statistics (for future extension) */
	u32			rx_frames; // successfully parsed frames
	u32			rx_crc_err; // frames dropped due to CRC error
	u32			rx_dropped; // frames dropped due to state(waiting)/sequence mismatch
}	fanctl_ctx_t;

int		fanctl_set_active_ctx(fanctl_ctx_t *ctx);
void		fanctl_clear_active_ctx(fanctl_ctx_t *ctx);
fanctl_ctx_t	*fanctl_get_active_ctx(void);

int		fanctl_do_req_wait_resp(fanctl_ctx_t *ctx,
			u8 req_cmd, const u8 *payload, u8 len,
			proto_frame_t *out_resp,
			unsigned long timeout_jiffies);
int		fanctl_write_frame(fanctl_ctx_t *ctx, const proto_frame_t *req);
bool		fanctl_match_resp(fanctl_ctx_t *ctx, const proto_frame_t *resp);

int		fanctl_ldisc_register(void);
void		fanctl_ldisc_unregister(void);
int		fanctl_chardev_register(void);
void		fanctl_chardev_unregister(void);
