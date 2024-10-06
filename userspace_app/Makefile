BUILDROOT_PATH := /home/useyin/Desktop/embedded-linux-dev/buildroot

# Define the cross-compiler and paths
CROSS_COMPILE := $(BUILDROOT_PATH)/output/host/bin/arm-linux-
CC := $(CROSS_COMPILE)gcc
CFLAGS := -I$(BUILDROOT_PATH)/output/host/arm-buildroot-linux-gnueabihf/sysroot/usr/include
LDFLAGS := -L$(BUILDROOT_PATH)/output/host/arm-buildroot-linux-gnueabihf/sysroot/usr/lib

# Define the source files and target
SRCS := main.c

INC_DIRS := -I.
OBJS := $(SRCS:.c=.o)
TARGET := sht20_user_driver

# Default target
all: $(TARGET)

# Link the target
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile the source files
%.o: %.c
	$(CC) $(CFLAGS) $(INC_DIRS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
