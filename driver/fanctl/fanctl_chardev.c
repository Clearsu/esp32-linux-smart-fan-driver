#include "fanctl.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#include "fanctl_uapi.h"

static DEFINE_MUTEX(g_ctx_lock);
static fanctl_ctx_t *g_active_ctx;

static u16	fanctl_parse_be16(const u8 *p)
{
	return ((u16)p[0] << 8) | p[1];
}

static long	fanctl_decode_ack_status(proto_frame_t *resp)
{
	u8	status;

	if (!resp)
		return -EINVAL;
	if (resp->cmd != PROTO_CMD_ACK || resp->len < 2)
		return -EPROTO;
	status = resp->payload[1];
	if (status == PROTO_ERR_OK)
		return 0;
	if (status == PROTO_ERR_INVALID_ARG)
		return -EOPNOTSUPP;
	if (status == PROTO_ERR_STATE)
		return -EBUSY;
	return -EPROTO;
}

int	fanctl_set_active_ctx(fanctl_ctx_t *ctx)
{
	int	ret;

	ret = 0;
	mutex_lock(&g_ctx_lock);
	if (g_active_ctx && g_active_ctx != ctx)
		ret = -EBUSY;
	else
		g_active_ctx = ctx;
	mutex_unlock(&g_ctx_lock);
	return ret;
}

void	fanctl_clear_active_ctx(fanctl_ctx_t *ctx)
{
	mutex_lock(&g_ctx_lock);
	if (g_active_ctx == ctx)
		g_active_ctx = NULL;
	mutex_unlock(&g_ctx_lock);
}

fanctl_ctx_t	*fanctl_get_active_ctx(void)
{
	fanctl_ctx_t	*ctx;

	mutex_lock(&g_ctx_lock);
	ctx = g_active_ctx;
	mutex_unlock(&g_ctx_lock);
	return ctx;
}

static long	fanctl_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int		ret;
	u8		payload[2];
	proto_frame_t	resp;

	fanctl_ctx_t *ctx = fanctl_get_active_ctx();
	if (!ctx) // when fanctl_open() is not called yet
		return -ENODEV;
	switch (cmd)
	{
	case FANCTL_IOC_PING:
		return fanctl_do_req_wait_resp(ctx, PROTO_CMD_PING,
						NULL, 0, &resp,
						msecs_to_jiffies(1000));

	case FANCTL_IOC_GET_STATUS:
		{
			struct fanctl_status	st;
			const u8		*p;

			ret = fanctl_do_req_wait_resp(ctx, PROTO_CMD_STATUS_REQ,
							NULL, 0, &resp,
							msecs_to_jiffies(1000));
			if (ret)
				return ret;
			if (resp.cmd != PROTO_CMD_STATUS_RESP || resp.len < sizeof(status_resp_t))
				return -EPROTO;
			p = resp.payload;
			st.temp_x100 = (u16)fanctl_parse_be16(p);
			st.humidity_x100 = (u16)fanctl_parse_be16(p + 2);
			st.fan_mode = p[4];
			st.fan_state = p[5];
			st.errors = (u16)fanctl_parse_be16(p + 6);
			if (copy_to_user((void __user *)arg, &st, sizeof(st)))
				return -EFAULT;
			return 0;
		}

	case FANCTL_IOC_SET_FAN_MODE:
		{
			u8	mode;

			if (copy_from_user(&mode, (void __user *)arg, sizeof(mode)))
				return -EFAULT;
			payload[0] = mode;
			ret = fanctl_do_req_wait_resp(ctx, PROTO_CMD_SET_FAN_MODE,
							payload, 1, &resp,
							msecs_to_jiffies(1000));
			if (ret)
				return ret;
			return fanctl_decode_ack_status(&resp);
		}

	case FANCTL_IOC_SET_FAN_STATE:
		{
			u8	state;

			if (copy_from_user(&state, (void __user *)arg, sizeof(state)))
				return -EFAULT;
			payload[0] = state;
			ret = fanctl_do_req_wait_resp(ctx, PROTO_CMD_SET_FAN_STATE,
							payload, 1, &resp, 
							msecs_to_jiffies(1000));
			if (ret)
				return ret;
			return fanctl_decode_ack_status(&resp);
		}

	case FANCTL_IOC_SET_THRESHOLD:
		{
			int16_t	temp_x100;

			if (copy_from_user(&temp_x100, (void __user *)arg, sizeof(temp_x100)))
				return -EFAULT;
			payload[0] = (u8)((temp_x100 >> 8) & 0xFF);
			payload[1] = (u8)(temp_x100 & 0xFF);
			ret = fanctl_do_req_wait_resp(ctx, PROTO_CMD_SET_THRESHOLD,
						payload, 2, &resp,
						msecs_to_jiffies(1000));
			if (ret)
				return ret;
			return fanctl_decode_ack_status(&resp);
		}

	default:
		return -ENOIOCTLCMD;
	}
}

/*
 * .owner = THIS_MODULE
 * --------------------
 * Prevents the module from being unloaded while
 * this file (/dev/fanctl) is opened (module ref counting).
 *
 * .unlocked_ioctl
 * ---------------
 *  Entry point for ioctl() system calls from userspace.
 *  "unlocked" means the kernel does not hold the BKL (Big Kernel Lock)
 *  when calling this function - synchronization must be handled 
 *  by the driver implementation.
 *  - BKL is removed since Linux 4.0
 */
static const struct file_operations fanctl_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = fanctl_unlocked_ioctl,
};

/*
 * miscdevice (miscellaneous device)
 * ---------------------------------
 * A shortcut for creating a simple character device.
 *
 * Major number for miscdevice: 10 (fixed)
 * Do not fix `minor` - might cause conflict (e.g. duplication).
 */
static struct miscdevice fanctl_miscdev = {
	.minor = MISC_DYNAMIC_MINOR, // kernel will dynamically allocate a minor number
	.name  = "fanctl",
	.fops  = &fanctl_fops,
	.mode  = 0666,
};

int fanctl_chardev_register(void)
{
	int	ret;

	ret = misc_register(&fanctl_miscdev);
	if (ret)
		pr_err("fanctl: misc_register failed: %d\n", ret);
	else
		pr_info("fanctl: /dev/%s created\n", fanctl_miscdev.name);
	return ret;
}

void fanctl_chardev_unregister(void)
{
	misc_deregister(&fanctl_miscdev);
	pr_info("fanctl: /dev/%s removed\n", fanctl_miscdev.name);
}
