
ifneq ($(KERNELRELEASE),)
	obj-m := newsyscall.o 
else
	include ../Makefile
	KERNELDIR := ../../$(KERNEL_VERSION)
	PWD := $(shell pwd)

all: test5 modules

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

test5: test5.c
	gcc -static -D__KERNEL__ -I../../$(KERNEL_VERSION)/include test5.c -o test5
	
clean:
	rm -rf *.[oas] .*.flags *.ko .*.cmd .*.d .*.tmp *.mod.c .tmp_versions Module.symvers test5 *.order
endif


