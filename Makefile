obj-m := vcmdas1.o


KERNELDIR ?= /lib/modules/$(shell uname -r)/build


all: modules


modules:
	$(MAKE) -C $(KERNELDIR) M=$$PWD modules


modules_install:
	$(MAKE) -C $(KERNELDIR) M=$$PWD modules_install

clean:
	rm -rf *.o .depend .*.cmd *.ko *.mod.c
	rm -rf .tmp_versions modules.order Module.symvers *.tmp *.log
