#pragma once

#ifdef __KERNEL__
# include <linux/types.h>
# include <linux/ioctl.h>

typedef __u8   fanctl_u8;
typedef __u16  fanctl_u16;
typedef __s16  fanctl_s16;
typedef __u32  fanctl_u32;

#else
# include <stdint.h>
# include <sys/ioctl.h>

typedef uint8_t  fanctl_u8;
typedef uint16_t fanctl_u16;
typedef int16_t  fanctl_s16;
typedef uint32_t fanctl_u32;

#endif

// ioctl magic
#define FANCTL_IOC_MAGIC  'F'

// status which will be handed over to the userspace
struct fanctl_status {
	fanctl_s16 temp_x100;      /* 0.01°C */
	fanctl_u16 humidity_x100;  /* 0.01% RH */
	fanctl_u8  fan_mode;       /* 0=AUTO, 1=MANUAL */
	fanctl_u8  fan_state;      /* 0=OFF, 1=ON */
	fanctl_u16 errors;         /* bitfield */
};

// ioctl cmds
#define FANCTL_IOC_PING          _IO(FANCTL_IOC_MAGIC, 0x01)
#define FANCTL_IOC_GET_STATUS    _IOR(FANCTL_IOC_MAGIC, 0x02, struct fanctl_status)
#define FANCTL_IOC_SET_FAN_MODE  _IOW(FANCTL_IOC_MAGIC, 0x03, fanctl_u8)
#define FANCTL_IOC_SET_FAN_STATE _IOW(FANCTL_IOC_MAGIC, 0x04, fanctl_u8)
#define FANCTL_IOC_SET_THRESHOLD _IOW(FANCTL_IOC_MAGIC, 0x05, fanctl_s16)
