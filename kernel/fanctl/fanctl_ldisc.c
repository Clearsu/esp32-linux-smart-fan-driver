#include "fanctl.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/termios.h>

/* Runs when line discipline is attached. */
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

	// register active ctx - shared with the ioctl context
	ret = fanctl_set_active_ctx(ctx);
	if (ret) {
		tty->disc_data = NULL;
		kfree(ctx);
		return ret;
	}
	pr_info("fanctl: ldisc attached\n");
	return 0;
}

/* Runs when line discipline is detached */
static void	fanctl_close(struct tty_struct *tty)
{
	fanctl_ctx_t	*ctx;

	ctx = tty->disc_data;
	if (!ctx)
		return;
	mutex_lock(&ctx->req_lock);
	ctx->waiting = false;
	complete_all(&ctx->resp_done); // wakeup ioctl
	mutex_unlock(&ctx->req_lock);
	fanctl_clear_active_ctx(ctx);
	tty->disc_data = NULL;
	kfree(ctx);
	pr_info("fanctl: ldisc detached\n");
}

/*
 * ldisc RX callback
 * -----------------
 * Runs in process context as part of deferred tty RX handling,
 * triggered by interrupt-driven USB serial RX activity.
 *
 * Execution context (typical path for USB-serial):
 *
 * 1. USB interrupt (hard IRQ)
 *    - URB(USB Request Block) completion is recorded
 *    - USB softirq is raised
 *
 * 2. USB softirq
 *    - The kernel first tries to run pending softirqs on the interrupt-return path.
 *      If softirq processing runs too long or is heavily loaded, it is deferred to
 *      the ksoftirqd/N kernel thread.
 *    - Runs URB completion callback:
 *      - Received bytes are handed to the tty layer (e.g., via tty_insert_flip_string()).
 *      - tty_flip_buffer_push() defers "flip buffer" processing.
 *
 * 3. Deferred tty processing (process context)
 *    - flush_to_ldisc() delivers bytes to the line discipline
 *    - ldisc->receive_buf2() is invoked
 *
 * This callback feeds incoming bytes into the protocol parser.
 * When a complete response frame matching the current outstanding request is detected,
 * it wakes up the sleeping ioctl handler via completion.
 *
 * Constraints:
 * - Must be non-blocking: do not sleep (mutex_lock, msleep, wait_for_completion, etc.)
 * - Keep work minimal (parsing + signaling only)
 */
static int	fanctl_receive_buf(struct tty_struct *tty, const unsigned char *cp,
		const char *fp, int count)
{
	fanctl_ctx_t	*ctx;
	unsigned long	flags;
	proto_frame_t	f;
	int		i;

	ctx = tty->disc_data;
	if (!ctx)
		return 0;
	for (i = 0; i < count; i++)
	{
		if (proto_rx_feed(&ctx->rx, (u8)cp[i], &f)) // parse
		{
			ctx->rx_frames++;
			if (ctx->waiting && fanctl_match_resp(ctx, &f))
			{
				spin_lock_irqsave(&ctx->resp_lock, flags); // busy wait
				ctx->last_resp = f;
				spin_unlock_irqrestore(&ctx->resp_lock, flags);
				complete(&ctx->resp_done); // wakeup ioctl context
			}
			else
				ctx->rx_dropped++;
		}
	}
	return count;
}

/*
 * tty line discipline operations
 * ------------------------------
 * This line discipline binds a tty device to the fanctl protocol handler.
 *
 * - open/close: allocate and release per-tty context
 * - receive_buf2: process raw RX bytes and assemble protocol frames
 *   - receive_buf (legacy) is not working
 */
static struct tty_ldisc_ops	fanctl_ldisc_ops = {
	.owner = THIS_MODULE,
	.num = N_FANCTL, // line discipline unique ID, here we set 27
	.name = "fanctl",
	.open = fanctl_open,
	.close = fanctl_close,
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
