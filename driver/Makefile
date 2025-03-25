# Kernel module Makefile for AES256GCM10G25GIP driver

# If KERNELRELEASE is defined, we've been invoked from the kernel build system
ifneq ($(KERNELRELEASE),)
    obj-m := aes256gcm10g25g.o
    aes256gcm10g25g-objs := aes-driver.o

# Otherwise we were called directly from the command line
else
    KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean

endif