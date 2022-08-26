
obj-m:=backtrace.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)
modules:
	make -C $(KDIR) M=$(PWD) modules
	objdump -ds backtrace.ko > txt
	objdump -dS backtrace.ko > txt1
	lsmod | grep backtrace || insmod backtrace.ko
	dmesg | tail -20 
clean:
	rm -f *.o *.symvers *.cmd *.cmd.o .*.o .*.cmd *.ko
