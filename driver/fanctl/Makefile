obj-m := fanctl.o

fanctl-objs := fanctl_main.o fanctl_core.o fanctl_ldisc.o fanctl_chardev.o proto.o

ccflags-y += -I$(src)/../../common

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

re: clean all

.PHONY: all clean re
