#pragma once

#include <linux/types.h>
#include <linux/tty.h>
#include <linux/tty_ldisc.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

#include "proto.h"

#define N_FANCTL 27

typedef struct fanctl_ctx
{
	struct tty_struct	*tty;
	proto_rx_t		rx;
	struct mutex		req_lock;
	spinlock_t		resp_lock;
	struct completion	resp_done;

	bool			waiting;
	u8			pending_cmd;
	u8			pending_seq;
	proto_frame_t		last_resp;

	u32			rx_frames;
	u32			rx_crc_err;
	u32			rx_dropped;
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
