

ifneq ($(KERNELRELEASE),)

obj-m := abc.o 
 
else
	include ../Makefile
	KERNELDIR := ../../$(KERNEL_VERSION)
	PWD := $(shell pwd)

all: modules test6

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

test6: test6.c
	gcc -static -D__KERNEL__ -I../../$(KERNEL_VERSION)/include test6.c -o test6

clean:
	rm -rf *.[oas] .*.flags *.ko .*.cmd .*.d .*.tmp *.mod.c .tmp_versions test6 Module.symvers *.order
endif

