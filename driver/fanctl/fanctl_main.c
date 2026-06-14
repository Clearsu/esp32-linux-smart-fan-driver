#include "fanctl.h"

#include <linux/module.h>
#include <linux/kernel.h>

static int __init fanctl_init(void)
{
	int	ret;

	ret = fanctl_chardev_register();
	if (ret) {
		pr_err("fanctl: chardev register failed: %d\n", ret);
		return ret;
	}

	ret = fanctl_ldisc_register();
	if (ret) {
		pr_err("fanctl: ldisc register failed: %d\n", ret);
		fanctl_chardev_unregister();
		return ret;
	}

	pr_info("fanctl: module loaded\n");
	return 0;
}

static void __exit fanctl_exit(void)
{
	fanctl_ldisc_unregister();
	fanctl_chardev_unregister();
	pr_info("fanctl: module unloaded\n");
}

module_init(fanctl_init);
module_exit(fanctl_exit);

MODULE_LICENSE("GPL");
