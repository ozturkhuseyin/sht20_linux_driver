obj-m += sht20_sensor.o 

sht20_sensor-objs := src/sht20_driver.o src/sht20_driver_util.o 

BUILDROOT_DIR = /home/useyin/Desktop/embedded-linux-dev/buildroot
KERNEL_DIR = $(BUILDROOT_DIR)/output/build/linux-6.6.3
KERNEL_INC_DIR = $(BUILDROOT_DIR)/output/build/linux-headers-6.6.3

CC = $(BUILDROOT_DIR)/output/host/bin/arm-buildroot-linux-gnueabihf-gcc
LD = $(BUILDROOT_DIR)/output/host/bin/arm-buildroot-linux-gnueabihf-ld

CFLAGS += -I$(KERNEL_INC_DIR) # Add this line to include the kernel headers
EXTRA_CFLAGS= -I$(PWD)/inc

all:
	make -C $(KERNEL_DIR) M=$(PWD) CC=$(CC) LD=$(LD) ARCH=arm modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
