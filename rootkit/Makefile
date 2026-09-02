KERNELDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

obj-m = wlkom.o
wlkom-objs = src/wlkom.o src/connection.o src/hide-rootkit/hide_rootkit.o src/hide-rootkit/syscall_hook.o src/persistence.o

.PHONY: all module clean

all: module

module:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
