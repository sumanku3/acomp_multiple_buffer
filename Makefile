CONFIG_MODULE_SIG=n#Suppress module signing for test module
ifneq ($(KERNELRELEASE),)
obj-m := acomp_test.o
else
KERNELDIR ?= /home/sumanku3/qat_repo/qat_next/
PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif

clean:
	/bin/rm -rf *.mod.c *.ko *.o *.a .*.cmd *.mod Module.symvers modules.order
