BASEINCLUDE 	?= /usr/src/kernels/$(shell  uname -r)/
proctest-objs 	:= kprobe_test.o
obj-m			:= kprobe_test.o

all:
	$(MAKE) -C $(BASEINCLUDE) M=$(PWD) modules;

clean:
	$(MAKE) -C $(BASEINCLUDE) M=$(PWD) clean;
	rm -rf *.ko
