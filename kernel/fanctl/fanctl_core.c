#include "fanctl.h"

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/delay.h>

bool	fanctl_match_resp(fanctl_ctx_t *ctx, const proto_frame_t *resp)
{
	if (resp->seq != ctx->pending_seq)
		return false;

	if (ctx->pending_cmd == PROTO_CMD_PING && resp->cmd == PROTO_CMD_PONG)
		return true;

	if (ctx->pending_cmd == PROTO_CMD_STATUS_REQ && resp->cmd == PROTO_CMD_STATUS_RESP)
		return true;

	if ((ctx->pending_cmd == PROTO_CMD_SET_FAN_MODE
		|| ctx->pending_cmd == PROTO_CMD_SET_FAN_STATE
		|| ctx->pending_cmd == PROTO_CMD_SET_THRESHOLD)
		&& resp->cmd == PROTO_CMD_ACK
		&& resp->len >= 2
		&& resp->payload[0] == ctx->pending_cmd)
		return true;

	return false;
}

int	fanctl_write_frame(fanctl_ctx_t *ctx, const proto_frame_t *req)
{
	u8		buf[2 + 3 + PROTO_MAX_PAYLOAD + 2];
	u16		out_len = 0;
	int		ret;
	int		offset = 0;
	int		room;
	unsigned long	deadline;

	if (!ctx || !req)
		return -EINVAL;
	if (!ctx->tty || !ctx->tty->ops || !ctx->tty->ops->write)
		return -ENODEV;

	if (!proto_build_frame(req->cmd, req->seq, req->payload, req->len, buf, &out_len))
		return -EINVAL;
	deadline = jiffies + msecs_to_jiffies(1000);
	while (offset < out_len)
	{
		room = tty_write_room(ctx->tty);
		if (room <= 0)
		{
			if (time_after(jiffies, deadline))
				return -EAGAIN;
			usleep_range(1000, 2000);
			continue;
		}
		ret = ctx->tty->ops->write(ctx->tty, buf + offset,
				min_t(int, room, out_len - offset));
		if (ret < 0)
			return ret;
		if (ret == 0) {
			if (time_after(jiffies, deadline))
				return -EIO;
			usleep_range(1000, 2000);
			continue;
		}
		offset += ret;
	}
	return 0;
}

int	fanctl_do_req_wait_resp(fanctl_ctx_t *ctx, u8 req_cmd, const u8 *payload,
			u8 len, proto_frame_t *out_resp, unsigned long timeout_jiffies)
{
	int		ret;
	unsigned long	flags;
	proto_frame_t	req;

	if (!ctx || !out_resp || len > PROTO_MAX_PAYLOAD || (len && !payload))
		return -EINVAL;

	mutex_lock(&ctx->req_lock);

	ctx->waiting = true;
	ctx->pending_cmd = req_cmd;
	ctx->pending_seq = (u8)(ctx->pending_seq + 1);
	reinit_completion(&ctx->resp_done);

	memset(&req, 0, sizeof(req));
	req.cmd = req_cmd;
	req.seq = ctx->pending_seq;
	req.len = len;
	if (len)
		memcpy(req.payload, payload, len);

	ret = fanctl_write_frame(ctx, &req);
	if (ret)
		goto out;

	if (!wait_for_completion_timeout(&ctx->resp_done, timeout_jiffies)) {
		ret = -ETIMEDOUT;
		goto out;
	}

	spin_lock_irqsave(&ctx->resp_lock, flags);
	*out_resp = ctx->last_resp;
	spin_unlock_irqrestore(&ctx->resp_lock, flags);

	ret = 0;

out:
	ctx->waiting = false;
	mutex_unlock(&ctx->req_lock);
	return ret;
}
