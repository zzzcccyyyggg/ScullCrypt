CONFIG_MODULE_SIG=n

obj-m := scull.o
KERNELDIR := /home/zzzccc/Desktop/kernel/linux-6.2
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers

.PHONY: all clean