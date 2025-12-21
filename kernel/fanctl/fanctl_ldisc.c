#include "fanctl.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/termios.h>

static int	fanctl_open(struct tty_struct *tty)
{
	fanctl_ctx_t	*ctx;
	int		ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->tty = tty;
	proto_rx_init(&ctx->rx);
	mutex_init(&ctx->req_lock);
	spin_lock_init(&ctx->resp_lock);
	init_completion(&ctx->resp_done);

	ctx->pending_seq = 0;
	ctx->waiting = false;

	tty->disc_data = ctx;

	// register active ctx for /dev/fanctl ioctl
	ret = fanctl_set_active_ctx(ctx);
	if (ret) {
		tty->disc_data = NULL;
		kfree(ctx);
		return ret;
	}
	pr_info("fanctl: ldisc attached\n");
	return 0;
}

static void	fanctl_close(struct tty_struct *tty)
{
	fanctl_ctx_t	*ctx = tty->disc_data;

	if (!ctx)
		return;
	// wake up ioctl
	mutex_lock(&ctx->req_lock);
	ctx->waiting = false;
	complete_all(&ctx->resp_done);
	mutex_unlock(&ctx->req_lock);
	fanctl_clear_active_ctx(ctx);
	tty->disc_data = NULL;
	kfree(ctx);
	pr_info("fanctl: ldisc detached\n");
}

static int	fanctl_receive_buf(struct tty_struct *tty, const unsigned char *cp,
		const char *fp, int count)
{
	fanctl_ctx_t	*ctx = tty->disc_data;
	unsigned long	flags;
	proto_frame_t	f;
	int		i;

	if (!ctx)
		return 0;
	for (i = 0; i < count; i++)
	{
		if (proto_rx_feed(&ctx->rx, (u8)cp[i], &f))
		{
			ctx->rx_frames++;
			if (ctx->waiting && fanctl_match_resp(ctx, &f))
			{
				spin_lock_irqsave(&ctx->resp_lock, flags);
				ctx->last_resp = f;
				spin_unlock_irqrestore(&ctx->resp_lock, flags);
				complete(&ctx->resp_done);
			}
			else
				ctx->rx_dropped++;
		}
	}
	return count;
}

static struct tty_ldisc_ops	fanctl_ldisc_ops = {
	.owner       = THIS_MODULE,
	.num         = N_FANCTL,
	.name        = "fanctl",
	.open        = fanctl_open,
	.close       = fanctl_close,
	.receive_buf2 = fanctl_receive_buf,
};

int fanctl_ldisc_register(void)
{
	int ret = tty_register_ldisc(&fanctl_ldisc_ops);
	if (ret)
		pr_err("fanctl: tty_register_ldisc failed: %d\n", ret);
	else
		pr_info("fanctl: ldisc registered (N=%d)\n", N_FANCTL);
	return ret;
}

void fanctl_ldisc_unregister(void)
{
	tty_unregister_ldisc(&fanctl_ldisc_ops);
	pr_info("fanctl: ldisc unregistered\n");
}
