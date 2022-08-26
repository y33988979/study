
obj-m:=lockup.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)
modules:
	make -C $(KDIR) M=$(PWD) modules
clean:
	rm -f *.o *.symvers *.cmd *.cmd.o .*.o .*.cmd *.ko
