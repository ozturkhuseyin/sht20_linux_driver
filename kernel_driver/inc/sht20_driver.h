#ifndef __SHT20_DRIVER_H__
#define __SHT20_DRIVER_H__

#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/version.h>
#include <asm/errno.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/mutex.h>
#include <linux/ioctl.h>
#include <linux/kthread.h>

#define DEVICE_NAME             "sht20"
#define DRIVER_NAME             "sht20_driver"
#define DEVICE_I2C_BUS_NUMBER   1

#define SHT20_I2C_ADDRESS       0x40

#define SHT20_TEMPERATURE_NO_HOLD_ADDRESS 0xF3
#define SHT20_HUMIDITY_NO_HOLD_ADDRESS 0xF5

#define SHT20_MODE_PERIODIC 0
#define SHT20_MODE_ONESHOT 1

#define SHT20_READ_TEMP 0
#define SHT20_READ_HUM 1
#define SHT20_READ_ALL 2

enum
{
    CDEV_NOT_USED = 0,
    CDEV_EXCLUSIVE_OPEN = 1,
};

int sht20_read_temperature(struct i2c_client *client, int *buff);
int sht20_read_hum(struct i2c_client *client, int *buff);

#endif /* __SHT20_DRIVER_H__ */